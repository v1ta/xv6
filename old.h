void old(int);
int __attribute__((used, section(".text\n\t"
  ".globl	old\n\t"
  ".type	old, @function\n\t"
  "old:\n\t"
  "movl	%esp, %ebp\n\t"
  "popl	%edx\n\t"
  "popl	%edx\n\t"
  "popl	%ecx\n\t"
  "popl	%eax\n\t"
  "ret\n\t"
  ".size	old, .-old\n\t"
  ".section .data"))) olda;
