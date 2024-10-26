#ifndef GREEN_THREAD_H
#define GREEN_THREAD_H

#include <setjmp.h>

class GreenThread {
private:
    jmp_buf to_thread, from_thread;
    char *stack, *stack_top;
public:
    GreenThread(unsigned stackSize=16*1024);
    virtual ~GreenThread();

    void Init();

    void EnterThread();
    void LeaveThread();

protected:
    virtual void Run() = 0;
};

#endif
