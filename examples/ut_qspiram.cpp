#include <qspiram.hpp>
#include <stdio.h>

QSPIRAM ram(1024, true);

void write_buffer(unsigned bytes, uint8_t *data) {
    for(unsigned i = 0; i < bytes; ++i) {
        ram.OnChar((data[i] >> 4) & 0xf);
        ram.OnChar(data[i] >> 0);
    }
}

void read_buffer(unsigned bytes, uint8_t *data) {
    for(unsigned i = 0; i < bytes; ++i) {
        uint8_t ch = ram.OnChar(0);
        data[i] = (ch << 4) | ram.OnChar(0);
    }
}

void do_write_operation(uint32_t addr, uint8_t *ch, unsigned n) {
    ram.OnCSEnable();
    ram.OnChar(0x3);
    ram.OnChar(0x8);

    write_buffer(3, (uint8_t *)&addr);
    write_buffer(n, ch);

    ram.OnCSDisable();
}

void do_read_operation(uint32_t addr, uint8_t *ch, unsigned n) {
    ram.OnCSEnable();
    ram.OnChar(0xE);
    ram.OnChar(0xB);

    uint32_t nl = 0;
    write_buffer(3, (uint8_t *)&addr);
    write_buffer(4, (uint8_t*) &nl);
    read_buffer(n, ch);

    ram.OnCSDisable();
}

int main() {
    ram.Init();

    // Enable QSPI mode (quad vs single)
    ram.OnCSEnable();
    uint8_t ch = 0x35;
    for(unsigned i = 0; i < 8; i++) {
        ram.OnChar((ch >> (7-i)) & 1);
    }
    ram.OnCSDisable();

    // New content
    uint8_t buffer[32] = {};
    snprintf((char*) buffer, 32, "Hello, world!");
    printf("Buffer: %s\n", buffer);

    // Write it
    do_write_operation(0x100, buffer, 32);

    // Check directly from memory
    printf("Read back Directly: %s\n", &ram.GetContent()[0x100]);

    // Request read operation
    uint8_t read_buffer[32] = {};
    do_read_operation(0x100, read_buffer, 32);
    printf("Read back through QSPI: %s\n", read_buffer);

}
