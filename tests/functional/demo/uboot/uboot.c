/*
 * untrusted elf loader prototype: UBOOT. can be used as example
 * note: in case of error UBOOT returns error's line number
 *
 * to make it easy to keep error codes synchronized..
 * (empty space)
 * (empty space)
 * (empty space)
 *
 * TODO: fix most ugly hacks: hardcoded offsets, magical numbers,
 *   pseudo-"strcmp" e.t.c.
 */
#include <sys/mman.h>
#include "api/zvm.h"
#include "uboot.h"

/* MANIFEST for uboot should not be constant */
#undef MANIFEST
#define MANIFEST ((struct UserManifest*)*((uintptr_t*)0x1000D))

/* switch between the jump/call (0/1) method */
#define PASS_CONTROL 0

/* jump to absolute address */
#define JUMP(entry_point) \
  do { \
    uint64_t addr = (entry_point); \
    asm volatile("pushq %0" : "+r" (addr) : ); \
    asm("naclret"); \
  } while(0)

#define SET_PHDR(raw, p_type_, p_flags_, action_, required_, p_vaddr_) \
  do { \
    raw.p_type = p_type_; \
    raw.p_flags = p_flags_; \
    raw.action = action_; \
    raw.required = required_; \
    raw.p_vaddr = p_vaddr_; \
  } while(0)

/* main of untrusted elf loader */
void _start() /* no return */
{
  uintptr_t i;
  int handle = 3; /* start looking for self name right after stderr */
  uintptr_t rodata_start = 0;
  uintptr_t rodata_end = 0;
  uintptr_t data_start = 0;
  uintptr_t data_end;
  uintptr_t max_vaddr;
  uintptr_t static_text_end = 0;
  uintptr_t break_addr; /* user manifest mem_ptr */
  uintptr_t initial_entry_pt; /* user entry point */
  struct ElfImage image_, *image = &image_;

  /* find handle of elf file to load */
  for(; handle < MANIFEST->channels_count; ++handle)
    if(*(uint64_t*)(MANIFEST->channels[handle].name) == SELFNAME_1)
      if(*((uint16_t*)(MANIFEST->channels[handle].name) + 4) == SELFNAME_2)
        break;
  FAILIF(handle >= MANIFEST->channels_count);
  FAILIF((MANIFEST->channels[handle].type & 1) == 0); /* elf should be random read */

  /* LOAD ELF HEADERS */
  /* load elf headers */
  FAILIF(z_pread(handle, &image->ehdr, sizeof image->ehdr, 0) != sizeof image->ehdr);

  /* read program headers. fail if too many prog headers */
  FAILIF(image->ehdr.e_phnum > NACL_MAX_PROGRAM_HEADERS);

  /* fail if ELF program header size too small */
  FAILIF(image->ehdr.e_phentsize < sizeof image->phdrs[0]);

  /* fail if cannot load tp prog headers */
  FAILIF(z_pread(handle, &image->phdrs[0], image->ehdr.e_phnum
    * sizeof image->phdrs[0], (off_t)image->ehdr.e_phoff)
    != (image->ehdr.e_phnum * sizeof image->phdrs[0]));

  /* VALIDATE ELF HEADER */
  const Elf_Ehdr *hdr = &image->ehdr;
  const void *p = hdr->e_ident;

  FAILIF(*(uint32_t*)p != ELFMAG); /* bad elf magic */
  FAILIF(ELFCLASS64 != hdr->e_ident[EI_CLASS]); /* bad elf class */
  FAILIF(ET_EXEC != hdr->e_type); /* non executable */
  FAILIF(EM_EXPECTED_BY_NACL != hdr->e_machine); /* bad machine */
  FAILIF(EV_CURRENT != hdr->e_version); /* bad elf version */


  /* VALIDATE ELF HEADERS */
  /*
   * Scan phdrs and do sanity checks in-line.  Verify that the load
   * address is NACL_TRAMPOLINE_END, that we have a single text
   * segment.  Data and TLS segments are not required, though it is
   * hard to avoid with standard tools, but in any case there should
   * be at most one each.  Ensure that no segment's vaddr is outside
   * of the address space.  Ensure that PT_GNU_STACK is present, and
   * that x is off.
   */
  /*
   * Other than empty segments, these are the only ones that are allowed.
   */
  struct PhdrChecks phdr_check_data[12];
  int seen_seg[ARRAY_SIZE(phdr_check_data)] = {0};
  int                 segnum;
  const Elf_Phdr      *php;
  size_t              j;

  /* initialize phdr_check_data */
  SET_PHDR(phdr_check_data[0], PT_PHDR, PF_R, PCA_IGNORE, 0, 0); /* phdr */
  SET_PHDR(phdr_check_data[1], PT_LOAD, PF_R|PF_X, PCA_TEXT_CHECK, 1, NACL_TRAMPOLINE_END); /* text */
  SET_PHDR(phdr_check_data[2], PT_LOAD, PF_R, PCA_RODATA, 0, 0); /* rodata */
  SET_PHDR(phdr_check_data[3], PT_LOAD, PF_R|PF_W, PCA_DATA, 0, 0); /* data/bss */
  SET_PHDR(phdr_check_data[4], PT_TLS, PF_R, PCA_IGNORE, 0, 0); /* tls */
  /* allow optional GNU stack permission marker, but require that the stack is non-executable */
  SET_PHDR(phdr_check_data[5], PT_GNU_STACK, PF_R|PF_W, PCA_NONE, 0, 0);
  SET_PHDR(phdr_check_data[6], PT_DYNAMIC, PF_R, PCA_IGNORE, 0, 0); /* ignored */
  SET_PHDR(phdr_check_data[7], PT_INTERP, PF_R, PCA_IGNORE, 0, 0); /* ignored */
  SET_PHDR(phdr_check_data[8], PT_NOTE, PF_R, PCA_IGNORE, 0, 0); /* ignored */
  SET_PHDR(phdr_check_data[9], PT_GNU_EH_FRAME, PF_R, PCA_IGNORE, 0, 0); /* ignored */
  SET_PHDR(phdr_check_data[10], PT_GNU_RELRO, PF_R, PCA_IGNORE, 0, 0); /* ignored */
  SET_PHDR(phdr_check_data[11], PT_NULL, PF_R, PCA_IGNORE, 0, 0); /* ignored */

  max_vaddr = NACL_TRAMPOLINE_END;

  /* phdr_check_data is small, so O(|check_data| * nap->elf_hdr.e_phum) is ok */
  for(segnum = 0; segnum < hdr->e_phnum; ++segnum)
  {
    php = &image->phdrs[segnum];

    /* We will not load this segment */
    if(0 == php->p_memsz)
      continue;

    for(j = 0; j < ARRAY_SIZE(phdr_check_data); ++j)
    {
      if(php->p_type == phdr_check_data[j].p_type
          && php->p_flags == phdr_check_data[j].p_flags) break;
    }

    FAILIF(j == ARRAY_SIZE(phdr_check_data)); /* unexpected type of segment */

    FAILIF(seen_seg[j]); /* Segment is a type that has been seen */
    seen_seg[j] = 1;

    if(PCA_IGNORE == phdr_check_data[j].action)
      continue;

    /* We will load this segment later. Do the sanity checks. */
    FAILIF(0 != phdr_check_data[j].p_vaddr
      && (phdr_check_data[j].p_vaddr != php->p_vaddr)); /* bad virtual address */
    FAILIF(php->p_vaddr < NACL_TRAMPOLINE_END); /* virtual address too low */
    FAILIF(php->p_vaddr + php->p_memsz < php->p_vaddr); /* p_memsz caused integer overflow */
    FAILIF(php->p_vaddr + php->p_memsz >= ((Elf_Addr)1U << ADDR_BITS)); /* too large segment */
    FAILIF(php->p_filesz > php->p_memsz); /* segment larger than memory size */

    /* record our decision that we will load this segment */
    image->loadable[segnum] = 1;

    /*
     * NACL_TRAMPOLINE_END <= p_vaddr
     *                     <= p_vaddr + p_memsz
     *                     < ((uintptr_t) 1U << ADDR_BITS)
     */
    if(max_vaddr < php->p_vaddr + php->p_memsz)
      max_vaddr = php->p_vaddr + php->p_memsz;

    switch(phdr_check_data[j].action)
    {
      case PCA_TEXT_CHECK:
        FAILIF(0 == php->p_memsz);
        static_text_end = NACL_TRAMPOLINE_END + php->p_filesz;
        break;
      case PCA_RODATA:
        rodata_start = php->p_vaddr;
        rodata_end = php->p_vaddr + php->p_memsz;
        break;
      case PCA_DATA:
        data_start = php->p_vaddr;
        data_end = php->p_vaddr + php->p_memsz;
        break;
      case PCA_NONE:
      case PCA_IGNORE:
        break;
    }
  }

  /* check if ELF executable missing a required segment (text) */
  for(j = 0; j < ARRAY_SIZE(phdr_check_data); ++j)
    FAILIF(phdr_check_data[j].required && !seen_seg[j]);

  /*
   * if no rodata and no data, we make sure that there is space for
   * the halt sled. else if no data, but there is rodata.  this means
   * max_vaddr is just where rodata ends.  this might not be at an
   * allocation boundary, and in this the page would not be writable.
   * round max_vaddr up to the next allocation boundary so that bss
   * will be at the next writable region.
   */
  if(0 == data_start)
  {
    if(0 == rodata_start)
    {
      if(ROUNDUP_64K(max_vaddr) - max_vaddr < NACL_HALT_SLED_SIZE)
        max_vaddr += NACL_MAP_PAGESIZE;
    }
    max_vaddr = ROUNDUP_64K(max_vaddr);
  }

  /*
   * max_vaddr -- the break or the boundary between data (initialized
   * and bss) and the address space hole -- does not have to be at a
   * page boundary.
   */
  break_addr = max_vaddr;
  data_end = max_vaddr;
  initial_entry_pt = image->ehdr.e_entry;

  /* Bad program entry point address */
  FAILIF(0 != (initial_entry_pt & (NACL_INSTR_BLOCK_SIZE - 1)));
  FAILIF(initial_entry_pt >= static_text_end);

  /* CHECK ADDRESS SPACE LAYOUT SANITY */
  /* fail if Data segment exists, but is not last segment */
  if(0 != data_start)
    FAILIF(data_end != max_vaddr);

  /*
   * This should be unreachable, but we include it just for completeness
   *
   * Here is why it is unreachable:
   *
   * PhdrChecks checks the test segment starting address.  The
   * only allowed loaded segments are text, data, and rodata.
   * Thus unless the rodata is in the trampoline region, it must
   * be after the text.  And ValidateProgramHeaders ensures that
   * all segments start after the trampoline region.
   *
   * d'b: fail if no data segment. read-only data segment exists
   * but is not last segment
   */
  else if(0 != rodata_start)
    FAILIF(ROUNDUP_64K(rodata_end) != max_vaddr);

  /* fail if Read-only data segment overlaps data segment */
  if(0 != rodata_start && 0 != data_start)
    FAILIF(rodata_end > data_start);

  /* fail if Text segment overlaps rodata segment */
  if(0 != rodata_start)
    FAILIF(ROUNDUP_64K(static_text_end) > rodata_start);
  /* fail if No rodata segment, and text segment overlaps data segment */
  else if(0 != data_start)
    FAILIF(ROUNDUP_64K(static_text_end) > data_start);

  /* fail if rodata_start not a multiple of allocation size */
  FAILIF(0 != rodata_start && ROUNDUP_64K(rodata_start) != rodata_start);

  /* fail if data_start not a multiple of allocation size */
  FAILIF(0 != data_start && ROUNDUP_64K(data_start) != data_start);

  /* LOAD ELF IMAGE */
  uintptr_t         end_vaddr;

  for(segnum = 0; segnum < image->ehdr.e_phnum; ++segnum)
  {
    const Elf_Phdr *php = &image->phdrs[segnum];

    /* did we decide that we will load this segment earlier? */
    if(!image->loadable[segnum]) continue;

    if(0 == php->p_filesz)
      continue;
    end_vaddr = php->p_vaddr + php->p_filesz;

    /* integer overflow? */
    FAILIF(end_vaddr < php->p_vaddr); /* parameter error should have been detected already */

    /*
     * is the end virtual address within the NaCl application's
     * address space?  if it is, it implies that the start virtual
     * address is also.
     */
    FAILIF(end_vaddr >= ((uint64_t)1U << ADDR_BITS)); /* parameter error should have been detected already */

    /*
     * NB: php->p_offset may not be a valid off_t on 64-bit systems, but
     * in that case Seek() will error out.
     * d'b: fail if ELF executable segment header parameter error
     */
    FAILIF(z_pread(handle, (void*)(uintptr_t)php->p_vaddr, php->p_filesz,
      (off_t)php->p_offset) != php->p_filesz); /* segment load failure */
    /* region from p_filesz to p_memsz should already be zero filled */
  }

  /* SET PROTECTIONS */
  /* change protection of user .text to RX */
  i = z_mprotect((void*)NACL_TRAMPOLINE_END,
    static_text_end - NACL_TRAMPOLINE_END, PROT_READ | PROT_EXEC);
  FAILIF(i != 0); /* validation failed */

  /* change protection of read only data to R if it is not NULL */
  if(data_start != 0)
  {
    i = z_mprotect((void*)rodata_start, data_start - rodata_start, PROT_READ);
    FAILIF(i != 0); /* cannot set R protection on user R data */
  }

  /* UPDATE USER MANIFEST */
  uint32_t mft_size = ROUNDUP_64K(sizeof(struct UserManifest)
    + MANIFEST->channels_count * sizeof(struct ZVMChannel));

  /* make user manifest writable */
  FAILIF(z_mprotect(MANIFEST, mft_size, PROT_WRITE) < 0);

  /* update manifest */
  i = (uintptr_t)MANIFEST->heap_ptr + MANIFEST->heap_size; /* end of heap */
  MANIFEST->heap_ptr = (void*)break_addr;
  
  /* "call" way to pass control returns uboot's space to heap */
#if PASS_CONTROL
  MANIFEST->heap_size = i - break_addr;
#else
  /* .."jump" way does not */
  MANIFEST->heap_size = i - break_addr - NACL_MAP_PAGESIZE;
#endif

  /* protect user manifest */
  FAILIF(z_mprotect(MANIFEST, mft_size, PROT_READ) < 0);

  /* PASS CONTROL TO LOADED ELF */

/*
 * pass control via mprotect deallocating own space. however this will not
 * allow uboot to set user stack properly. user code will get stack with junk.
 * since current zerovm runtime library does not support junk in stack this
 * method to pass control to user code will not work
 */
#if PASS_CONTROL
  register uint64_t addr = initial_entry_pt;
  register uint64_t self = (uint64_t)MANIFEST->heap_size
    + (uintptr_t)MANIFEST->heap_ptr - 0x10000 /* BOOTSIZE */;

  /* restore rsp */
  asm("mov $0xffffffc0, %esp");
  asm("add %r15, %rsp");
  
  /* allocate stack space for trap function arguments */
  asm("mov $0x455a494c4147454c, %r11");
  asm("mov $0x534942414e4e4143, %rdi");
  asm(".intel_syntax noprefix");
  asm("lea edi, [rsp - 0x28]");
  asm(".att_syntax");

  /* set up trap function (with arguments) to return own page to heap */
  asm("pushq $0x0"); /* 0x0 reserved TODO: remove it and fix stack */
  asm("pushq $0x3"); /* argument 3 */
  asm("pushq $0x10000"); /* argument 2 */
  asm volatile("pushq %0" : "+r" (self) : ); /* argument 1 */
  asm("pushq $0x544f5250"); /* trap function: TrapPROT */

  /* emulate call to trap with different return address */
  asm volatile("pushq %0" : "+r" (addr) : );
  JUMP(0x10000);

/*
 * pass control via simple jump. user have to deallocate uboot space. user
 * stack will be set properly and contain correct information for nacl
 * prologue (_start). if user not deallocate uboot space it is also ok
 * since uboot is not part of heap and user code will not try to access
 * to uboot space
 */
#else
  /* store user elf entry point */
  register uint64_t addr = initial_entry_pt;
  asm volatile("pushq %0" : "+r" (addr) : );
  asm("popq %rcx");

  /*
   * set user stack. note that everything in stack already is junk and
   * can be safely rewritten
   */
  /* assuming top of stack = 0 pushing of 0 can be omitted */
  asm("mov $0xFFFFFFF8, %esp");
  asm("add %r15, %rsp");

  /* put to user stack other values */
  asm("push $0x0");
  asm("push $0x0");
  asm("push $0x0");
  asm("push $0x0");
  asm("movl $0x1, (%rsp)");
  asm("movl $0xFFFFFFF0, 4(%rsp)");
  asm("push $0x0");
  asm("push $0x0");

  /* reset registers */  
  asm("xor %rax, %rax");
  asm("xor %rbx, %rbx");
  asm("xor %rdx, %rdx");
  asm("xor %rsi, %rsi");
  asm("xor %rsi, %rsi");
  asm("xor %r8, %r8");
  asm("xor %r9, %r9");
  asm("xor %r10, %r10");
  asm("xor %r11, %r11");
  asm("xor %r12, %r12");
  asm("xor %r13, %r13");
  asm("xor %r14, %r14");
  asm("mov %rsp, %rbp");

  asm("mov $-0x20, %rdi");

  /* jump to user elf entry point */
  asm("push %rcx");
  asm("pop %rcx");
  
  asm("and $0xffffffe0,%ecx");
  asm("add %r15,%rcx");
  asm("jmpq *%rcx");
#endif
}
