/*
 * syscall manager. extracts from the stack syscall number and
 * syscall arguments, tunes return address then pass control
 * to the appropiate function. to finalize the routine it restores
 * stack, set syscall return value and jumps to the user code
 *
 * call of syscall and regular c-style call are different!
 * in case of syscall stack (64-bit) has special format:
 * rsp =
 * 0 - address of syscall in trampoline
 * 4 - return address (to user code)
 * note: actually it is 64-bit call convention
 *
 * an arguments passing through 6 registers
 *     edi, esi, edx, ecx, r8d, r9d - in this exact order
 *
 * returning value holding in rax
 *
 * note: syscall_manager() directly takes control from the trusted code (zerovm)
 *
 * todo: since the code is untrusted it cannot leak important info,
 *       however some registers still should be saved for sake c standard
 *       compatibility
 */

        .text
        .xdef syscall_director

syscall_director:
        /* get syscall number: eax */
        pop %rax /* rsp now points to user code return address */
        sub $0x10000, %eax
        shr $5, %eax

        /* check if syscall number is within acceptable range */
        cmp $110, %eax
        jb sane_syscall
        xor %eax, %eax

				/* calculate the syscall address (low part): eax */
sane_syscall:
        shl $2, %eax
        add $zrt_syscalls, %eax
        mov (%r15,%rax), %eax

        /* allocate 0x18 bytes in the stack and save syscall arguments */
        naclasp $-0x18, %r15

				/* store syscall arguments */
        mov %edi, 0x00(%rsp)
        mov %esi, 0x04(%rsp)
        mov %edx, 0x08(%rsp)
        mov %ecx, 0x0c(%rsp)
        mov %r8d, 0x10(%rsp)
        mov %r9d, 0x14(%rsp)

				/* prepare syscall argument (*args) */
				mov %esp, %edi

        /* invoke syscall (rax == syscall return value if applicable) */
    		naclcall %eax, %r15

        /* restore stack to jump back to user code and prepare "jump back" address in r11 */
    		naclasp $0x18, %r15
        pop %r11

				/* jump back to user code. nacl "jump" pattern should be used */
				nacljmp %r11d, %r15
