//#define _FORTIFY_SOURCE 0

#include <setjmp.h>
#include <stdlib.h>

#include "green-thread.h"

typedef void (*volatile green_thread_function)(void *);

__attribute__((__noinline__))
__attribute__((regparm(3)))
void RunWithCustomStack(void *arg, green_thread_function func, void *stack) {
    // FIXME writing it in assembly may be better, this is too wanky
    register void *volatile argReg asm ("rax") = arg;
    register void (*volatile funcReg)(void *) __asm__("rbx") = func;
    register void *volatile sp asm("sp");
    sp = stack;

    funcReg(argReg);
}

GreenThread::GreenThread(unsigned stackSize) {
    stack = (char*) ::malloc(stackSize);
    stack_top = &stack[stackSize-8];
}

GreenThread::~GreenThread() {
    ::free(stack);
    stack = nullptr;
}

void GreenThread::Init() {
    if(setjmp(from_thread) == 0) {
        RunWithCustomStack(this, [](void *arg) {
            GreenThread *self = (GreenThread *)arg;
            while(1) {
                self->Run();
            }
        }, stack_top);
    }
}

void GreenThread::EnterThread() {
    if(setjmp(from_thread) == 0) {
        longjmp(to_thread, 1);
    }
}
void GreenThread::LeaveThread() {
    if(setjmp(to_thread) == 0) {
        longjmp(from_thread, 1);
    }
}
