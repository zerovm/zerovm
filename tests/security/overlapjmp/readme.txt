zerovm must show something like:

validator state = 1
user return code = 0
etag = da39a3ee5e6b4b0d3255bfef95601890afd80709
accounting = 0.00 0.00 16777216 0 0 0 0 0 0 0 0 0
exit state = src/main/zvm_main.c 165: validation failed

and valz will show:
VALIDATOR: segment[0] p_type 6 p_offset 0 vaddr 0 paddr 0 align 8
VALIDATOR:     filesz 158 memsz 158 flags 4
VALIDATOR: segment[1] p_type 1 p_offset 160 vaddr 20000 paddr 20000 align 32
VALIDATOR:     filesz 1ac0 memsz 1ac0 flags 5
VALIDATOR: parsing segment 1
VALIDATOR: 20005: Use of 67 (ADDR16) prefix not allowed by Native Client
VALIDATOR: 20005: Assignment to non-64 bit memory address
VALIDATOR: 2000a: Use of 67 (ADDR16) prefix not allowed by Native Client
VALIDATOR: 2000a: Assignment to non-64 bit memory address
VALIDATOR: 2000f: Use of 67 (ADDR16) prefix not allowed by Native Client
VALIDATOR: 2000f: Assignment to non-64 bit memory address
VALIDATOR: 20016: Use of 67 (ADDR16) prefix not allowed by Native Client
VALIDATOR: 20014: Illegal assignment to EBP
VALIDATOR: 20016: Assignment to non-64 bit memory address
VALIDATOR: 20016: Jump not native client compliant
VALIDATOR: 20045: Use of 67 (ADDR16) prefix not allowed by Native Client
VALIDATOR: 20045: Assignment to non-64 bit memory address
VALIDATOR: 2004a: Use of 67 (ADDR16) prefix not allowed by Native Client
VALIDATOR: 2004a: Assignment to non-64 bit memory address
VALIDATOR: 2004f: Use of 67 (ADDR16) prefix not allowed by Native Client
VALIDATOR: 2004f: Assignment to non-64 bit memory address
VALIDATOR: 20056: Use of 67 (ADDR16) prefix not allowed by Native Client
VALIDATOR: 20054: Illegal assignment to EBP
VALIDATOR: 20056: Assignment to non-64 bit memory address
VALIDATOR: 20056: Jump not native client compliant
VALIDATOR: 202ca: CPU model does not support CMOV instructions.
VALIDATOR: 20cc0: CPU model does not support LongMode instructions.
VALIDATOR: Checking block alignment and jump targets: 20000 to 21ac0
VALIDATOR: segment[2] p_type 1 p_offset 1c20 vaddr 30000 paddr 30000 align 16
VALIDATOR:     filesz 420 memsz 420 flags 4
VALIDATOR: segment[3] p_type 7 p_offset 0 vaddr 0 paddr 0 align 8
VALIDATOR:     filesz 0 memsz 0 flags 4
VALIDATOR: segment[4] p_type 1 p_offset 2040 vaddr 40000 paddr 40000 align 16
VALIDATOR:     filesz 28 memsz 40 flags 6
*** overlapjmp.nexe IS UNSAFE ***
*** overlapjmp.nexe IS UNSAFE ***

