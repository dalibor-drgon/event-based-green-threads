#ifndef QSPIRAM_H
#define QSPIRAM_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "green-thread.h"

class QSPIRAM : protected GreenThread {
protected:
    class Input {
        Input(bool enable, bool disable, bool is_char, bool has_char, uint8_t
        ch) : is_enable(enable), is_disable(disable), is_char(is_char),
        has_char(has_char), ch(ch) {}
    public:
        bool is_enable;
        bool is_disable;
        bool is_char;
        bool has_char;
        uint8_t ch;

        Input() {}

        static Input Enable() {
            return Input(true, false, false, false, 0);
        }

        static Input Disable() {
            return Input(false, true, false, false, 0);
        }

        static Input Char(uint8_t ch) {
            return Input(false, false, true, true, ch);
        }

        static Input CharHighImpedance() {
            return Input(false, false, true, false, 0);
        }
    };


    class Output {
        Output(bool has_char, uint8_t ch)
        : has_char(has_char), ch(ch) {}
    public:
        bool has_char;
        uint8_t ch;

        Output() {}

        static Output HighImpedance() {
            return Output(false, 0);
        }

        static Output Char(uint8_t ch) {
            return Output(true, ch);
        }
    };

    class ProtocolError {
    public:
        ProtocolError() {}
        virtual ~ProtocolError() {}
    };

    Output last_output;
    Input last_input;
    bool is_quad;
    bool has_read_delay;

    unsigned size; // in bytes
    uint8_t *data;

    void SetOutput(const Output &output) {
        last_output = output;
    }

    Input & WaitInput() {
        LeaveThread();
        return last_input;
    }

    Output & Converse(const Input &input) {
        last_input = input;
        EnterThread();
        return last_output;
    }

public:
    QSPIRAM(unsigned size, bool hasReadDelay=true, bool forceQuad=false) : 
        GreenThread(16*1024),
        is_quad(forceQuad),
        has_read_delay(hasReadDelay),
        size(size),
        data((uint8_t *) ::calloc(16, size/16))
    {
        //Init();
    }

    ~QSPIRAM() {
        ::free(data);
        data = nullptr;
    }

    void Init() {
        GreenThread::Init();
    }

    unsigned GetSize() {
        return size;
    }

    uint8_t *GetContent() {
        return data;
    }

    void OnCSEnable() {
        Converse(Input::Enable());
    }

    void OnCSDisable() {
        Converse(Input::Disable());
    }

    uint8_t OnChar(uint8_t ch) {
        return Converse(Input::Char(ch)).ch;
    }

protected:

    void WaitForEnable() {
        while(WaitInput().is_enable == false) {
            SetOutput(Output::HighImpedance());
        }
        SetOutput(Output::HighImpedance());
    }

    void WaitDisable() {
        if(WaitInput().is_disable == false) {
            throw ProtocolError();
        }
        SetOutput(Output::HighImpedance());
    }

    uint8_t WaitCharRaw1x4(bool strong = false) {
        Input & in = WaitInput();
        if(in.is_char == false || (strong && in.has_char == false)) {
            throw ProtocolError();
        }
        return in.ch;
    }

    uint8_t WaitCharRaw4x1(bool strong=false) {
        uint8_t ret = 0;
        for(unsigned i = 0; i < 4; i++) {
            ret = (ret << 1) | WaitCharRaw1x4(strong);
        }
        return ret;
    }

    uint8_t WaitCharRaw4(bool strong = false) {
        if(is_quad) {
            return WaitCharRaw1x4(strong);
        } else {
            return WaitCharRaw4x1(strong);
        }
    }

    uint32_t WaitChar(unsigned nBits = 8, bool strong = false) {
        uint32_t ch = 0;
        for(unsigned i = 0; i < nBits; i += 4) {
            ch = (ch << 4) | WaitCharRaw4(strong);
        }
        return ch;
    }

    void SendChar(uint8_t ch) {
        WaitChar(4, false);
        SetOutput(Output::Char(ch));
    }

    void RunQuad() {
        uint8_t ch = WaitChar(8);
        switch(ch) {
            case 0xEB: // Fast Read Quad (max 144 MHz)
            {
                uint32_t addr = WaitChar(24, true);
                WaitChar(28); // 7 cycles pause
                if(has_read_delay)
                    WaitChar(4);
                for(unsigned i = 0; i < 32; i++) {
                    uint8_t ch = data[addr + i];
                    SendChar(ch >> 4);
                    SendChar(ch & 0xf);
                }
                break;
            }

            case 0x38: // Quad Write Mode (max 144 MHz)
            {
                uint32_t addr = WaitChar(24, true);
                for(unsigned i = 0; i < 32; i++) {
                    uint8_t ch = WaitChar(8, true);
                    data[addr + i] = ch;
                }
                break;
            }

            default:
                throw ProtocolError();

        }
    }

    void RunSingle() {
        uint8_t ch = WaitChar(8);
        if(ch == 0x35) {
            is_quad = true;
        } else {
            throw ProtocolError();
        }
    }

    virtual void Run() {
        WaitForEnable();
        try {
            if(is_quad) {
                RunQuad();
            } else {
                RunSingle();
            }
            WaitDisable();
        } catch(ProtocolError &err) {
            SetOutput(Output::HighImpedance());
        }

    }

};

#endif
