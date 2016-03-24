#include "signal.h"
void trampoline(siginfo_t info);
int __attribute__((used, section(".text\n\t"
  ".globl	trampoline\n\t"
  ".type	trampoline, @function\n\t"
  "trampoline:\n\t"
  "movl	%esp, %ebp\n\t"
  "popl	%edx\n\t"
  "popl	%edx\n\t"
  "popl	%ecx\n\t"
  "popl	%eax\n\t"
  "ret\n\t"
  ".size	trampoline, .-trampoline\n\t"
  ".section .data"))) trampolinea;
