void restorer(int);
int __attribute__((used, section(".text\n\t"
			".globl	restorer\n\t"
			".type	restorer, @function\n\t"
			"restorer:\n\t"
			"movl	%esp, %ebp\n\t"
			"popl	%edx\n\t"
			"popl	%edx\n\t"
			"popl	%ecx\n\t"
			"popl	%eax\n\t"
			"ret\n\t"
			".size	restorer, .-restorer\n\t"
			".section .data"))) restorera;
