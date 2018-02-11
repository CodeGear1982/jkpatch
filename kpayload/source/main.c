/* golden */
/* 1/2/2018 */

#include "jkpayload.h"

#include "rpc.h"
#include "fself.h"
#include "fpkg.h"

void hook_trap_fatal(struct trapframe *tf) {
	// print registers
	const char regnames[15][8] = { "rdi", "rsi", "rdx", "rcx", "r8", "r9", "rax", "rbx", "rbp", "r10", "r11", "r12", "r13", "r14", "r15" };
	for(int i = 0; i < 15; i++) {
		uint64_t rv = *(uint64_t *)((uint64_t)tf + (sizeof(uint64_t) * i));
		uprintf("\t%s %llX %i", regnames[i], rv, rv);
	}
	uprintf("\trip %llX %i", tf->tf_rip, tf->tf_rip);
	uprintf("\trsp %llX %i", tf->tf_rsp, tf->tf_rsp);

	uint64_t sp = 0;
	if ((tf->tf_rsp & 3) == 3) {
		sp = *(uint64_t *)(tf + 1);
	} else {
		sp = (uint64_t)(tf + 1);
	}

	// stack backtrace
	uint64_t kernbase = getkernbase();
	uint64_t backlog = 128;
	uprintf("stack backtrace (0x%llX):", sp);
	for (int i = 0; i < backlog; i++) {
		uint64_t sv = *(uint64_t *)((sp - (backlog * sizeof(uint64_t))) + (i * sizeof(uint64_t)));
		if (sv > kernbase) {
			uprintf("\t%i <kernbase>+0x%llX", i, sv - kernbase);
		}
	}

	kern_reboot(4);
}

void install_trap_hook() {
	// disable write protect
	uint64_t CR0 = __readcr0();
	__writecr0(CR0 & ~CR0_WP);

	uint64_t kernbase = getkernbase();

	memcpy((void *)(kernbase + 0xECA92), "\x4C\x89\xE7", 3); // mov rdi, r12
	write_jmp(kernbase + 0xECA95, (uint64_t)hook_trap_fatal);

	// restore CR0
	__writecr0(CR0);
}

int payload_entry(void *arg) {
	// initialize uart
	init_uart();

	// initialize rpc
	init_rpc();

	// fake self binaries
	install_fself_hooks();

	// fake package containers
	shellcore_fpkg_patch();
	install_fpkg_hooks();

	// install trap hook
	install_trap_hook();

	return 0;
}
