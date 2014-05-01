/*
 * untrusted boot (UBOOT) prototype
 *
 * WARNING: ro data and rw data will not be loaded! only .text elf section
 * will be processed by zerovm. so, do not use strings, static arrays e.t.c.
 *
 * WARNING: entry point should be 1st byte in .text section (zerovm will not
 * read entry point and will always assume it started from the .text beginning)
 *
 * WARNING: only zerovm api should be used
 *
 * note: since uboot has very basic error detection, all critical load errors
 * should be fatal and should be invoked from the line where they were produced.
 * FAILIF() macro can be used for that
 */
#include <sys/mman.h>
#include "api/zvm.h"
#include "uboot.h"

/* todo: how to avoid redefining of MANIFEST (and warnings)? */
#undef MANIFEST
#define MANIFEST ((struct UserManifest*)(uintptr_t)0xFF000000)

/* main of untrusted elf loader */
void _start() /* no return */
{
  /* declaration of utility functions */
  int find_handle();
  void pass(uintptr_t entry_point);
  uintptr_t AppLoadFile(int handle);
  
  /* (to remove) ### invoke tester */
  tester_tmp();

  pass(AppLoadFile(find_handle()));
  FAILIF(1); /* not reachable */
}

/*
 * functions to remove from the final release {{
 */
void remove_protection_tmp()
{
  FAILIF(zvm_mprotect(NACL_TRAMPOLINE_END, 0x10000, PROT_READ | PROT_WRITE) < 0);
}

// ### boot tester. copies uboot to the end of heap. passes control to it
// removing RX protection from address 0x20000 
void tester_tmp()
{
  int i;
  char *old = NACL_TRAMPOLINE_END; // old uboot position
  char *new = MANIFEST->heap_size + (uintptr_t)MANIFEST->heap_ptr; // new uboot position

  // copy uboot to the last heap page
  for(i = 0; i < 0x10000; ++i)
    new[i] = old[i];

  // change protection of the uboot new place
  FAILIF(zvm_mprotect(new, 0x10000, PROT_READ | PROT_EXEC) < 0);

  // jump there to exact next instruction
  
  
  // return old uboot address to the heap
  FAILIF(zvm_mprotect(old, 0x10000, PROT_READ | PROT_WRITE) < 0);

  // control will continue inthe new place
}
/* functions to remove from the final release }} */

/* find boot handle */
/* todo: instead of constant find and return "/boot/elf" handle */
int find_handle()
{
  return 3;
}

/* pass control to loaded elf via zvm_mprotect */
/* TODO: how to adjust rsp before passing control? */
/* TODO: return ubbot space to the user heap */
void pass(uintptr_t entry_point) /* no return */
{
  uint64_t addr = entry_point;

  /* set trap function arguments address */
  asm(".intel_syntax noprefix");
  asm("lea edi, [rsp - 0x28]");
  asm(".att_syntax");

  /* set trap function (with arguments) */
  asm("pushq $0x3"); /* argument 3 */
  asm("pushq $0x10000"); /* argument 2 */
  asm("pushq $0x20000"); /* argument 1 */
  asm("pushq $0x0"); /* 0x0 reserved */
  asm("pushq $0x746f7250"); /* trap function: TrapProt */

  /* prepare to jump to trap with *different* return address */
  asm volatile("pushq %0" : "+r" (addr) : );

  /* jump to trap */
  asm("push $0x10000"); /* trap address */
  asm("naclret"); /* nacl pattern */
}

/* modify user manifest. return 0 if ok */
void set_manifest(uintptr_t heap_ptr)
{
  uint32_t mft_size = ROUNDUP_64K(sizeof(struct UserManifest)
    + MANIFEST->channels_count * sizeof(struct ZVMChannel));

  /* make user manifest writable */
  FAILIF(zvm_mprotect(MANIFEST, mft_size, PROT_WRITE) < 0);

  /* update manifest */
  /* TODO: return ubbot space to the user heap */
  MANIFEST->heap_size -= ROUNDUP_64K(heap_ptr) 
    - ROUNDUP_64K((uintptr_t)MANIFEST->heap_ptr);
  MANIFEST->heap_ptr = (void*)heap_ptr;

  /* protect user manifest */
  FAILIF(zvm_mprotect(MANIFEST, mft_size, PROT_READ | PROT_EXEC) < 0);
}

/* set RX protection on user code */
void set_code(uintptr_t static_text_start, uintptr_t static_text_end)
{
  int i;

  /* change protection of user .text to RX */
  i = zvm_mprotect((void*)static_text_start,
    static_text_end - static_text_start, PROT_READ | PROT_EXEC);
  FAILIF(i < 0); /* validation failed */
}

/* set R protection on user R data */
void set_ro_data(uintptr_t data_start, uintptr_t rodata_start)
{
  int i;

  /* change protection of area to R if read only data is not NULL */
  if(data_start == 0)
    return;
  i = zvm_mprotect((void*)rodata_start, data_start - rodata_start, PROT_READ);
  FAILIF(i < 0); /* cannot set R protection on user R data */
}

/*
 * read elf headers
 * note: "image" should contain enough space to hold all data
 * note: since uboot has very basic error detection
 */
void ElfImageNew(int handle, struct ElfImage *image)
{
  /* load elf headers */
  FAILIF(zvm_pread(handle, &image->ehdr, sizeof image->ehdr, 0) != sizeof image->ehdr);

  /* read program headers. fail if too many prog headers */
  FAILIF(image->ehdr.e_phnum > NACL_MAX_PROGRAM_HEADERS);

  /* fail if ELF program header size too small */
  FAILIF(image->ehdr.e_phentsize < sizeof image->phdrs[0]);

  /* fail if cannot load tp prog headers */
  FAILIF(zvm_pread(handle, &image->phdrs[0], image->ehdr.e_phnum
    * sizeof image->phdrs[0], (off_t)image->ehdr.e_phoff)
    != (image->ehdr.e_phnum * sizeof image->phdrs[0]));
}

void ValidateElfHeader(const struct ElfImage *image)
{
  const Elf_Ehdr *hdr = &image->ehdr;
  const void *p = hdr->e_ident;

  FAILIF(*(uint32_t*)p != ELFMAG); /* bad elf magic */
  FAILIF(ELFCLASS64 != hdr->e_ident[EI_CLASS]); /* bad elf class */
  FAILIF(ET_EXEC != hdr->e_type); /* non executable */
  FAILIF(EM_EXPECTED_BY_NACL != hdr->e_machine); /* bad machine */
  FAILIF(EV_CURRENT != hdr->e_version); /* bad elf version */
}

/* validate elf headers and return elf sections addresses by reference */
#define SET_PHDR(raw, p_type_, p_flags_, action_, required_, p_vaddr_) \
  do { \
    raw.p_type = p_type_; \
    raw.p_flags = p_flags_; \
    raw.action = action_; \
    raw.required = required_; \
    raw.p_vaddr = p_vaddr_; \
  } while(0)
void ValidateProgramHeaders(
  struct ElfImage     *image,
  uintptr_t           *static_text_end,
  uintptr_t           *rodata_start,
  uintptr_t           *rodata_end,
  uintptr_t           *data_start,
  uintptr_t           *data_end,
  uintptr_t           *max_vaddr)
{
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
  const Elf_Ehdr      *hdr = &image->ehdr;
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

  *max_vaddr = NACL_TRAMPOLINE_END;

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
    if(*max_vaddr < php->p_vaddr + php->p_memsz)
      *max_vaddr = php->p_vaddr + php->p_memsz;

    switch(phdr_check_data[j].action)
    {
      case PCA_TEXT_CHECK:
        FAILIF(0 == php->p_memsz);
        *static_text_end = NACL_TRAMPOLINE_END + php->p_filesz;
        break;
      case PCA_RODATA:
        *rodata_start = php->p_vaddr;
        *rodata_end = php->p_vaddr + php->p_memsz;
        break;
      case PCA_DATA:
        *data_start = php->p_vaddr;
        *data_end = php->p_vaddr + php->p_memsz;
        break;
      case PCA_NONE:
      case PCA_IGNORE:
        break;
    }
  }

  /* check if ELF executable missing a required segment (text) */
  for(j = 0; j < ARRAY_SIZE(phdr_check_data); ++j)
    FAILIF(phdr_check_data[j].required && !seen_seg[j]);
}
#undef SET_PHDR

/* Basic address space layout sanity check */
void CheckAddressSpaceLayoutSanity(
  uintptr_t static_text_end,
  uintptr_t data_start,
  uintptr_t rodata_start,
  uintptr_t rodata_end,
  uintptr_t data_end,
  uintptr_t max_vaddr)
{
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
}

int AddrIsValidEntryPt(uintptr_t addr, uintptr_t static_text_end)
{
  if(0 != (addr & (NACL_INSTR_BLOCK_SIZE - 1)))
    return 0;
  return addr < static_text_end;
}

void ElfImageLoad(const struct ElfImage *image, int handle)
{
  int               segnum;
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
    FAILIF(zvm_pread(handle, (void*)(uintptr_t)php->p_vaddr, php->p_filesz,
      (off_t)php->p_offset) != php->p_filesz); /* segment load failure */
    /* region from p_filesz to p_memsz should already be zero filled */
  }
}

/* load elf in place. out protection on elf sections. return user entry point */
uintptr_t AppLoadFile(int handle)
{
  uintptr_t rodata_start;
  uintptr_t rodata_end;
  uintptr_t data_start;
  uintptr_t data_end;
  uintptr_t max_vaddr;
  uintptr_t static_text_end;
  uintptr_t break_addr; /* user manifest mem_ptr */
  uintptr_t initial_entry_pt; /* user entry point */
  struct ElfImage image_, *image = &image_;

  /* temporay object will be deleted at end of function */
  ElfImageNew(handle, image);
  ValidateElfHeader(image);

  ValidateProgramHeaders(image, &static_text_end, &rodata_start,
      &rodata_end, &data_start, &data_end, &max_vaddr);

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
  FAILIF(!AddrIsValidEntryPt(initial_entry_pt, static_text_end)); /* Bad user entry point */

  CheckAddressSpaceLayoutSanity(static_text_end,
    data_start, rodata_start, rodata_end, data_end, max_vaddr);

  ElfImageLoad(image, handle);

  set_code(NACL_TRAMPOLINE_END, static_text_end);
  set_ro_data(data_start, rodata_start);
  set_manifest(break_addr);

  return initial_entry_pt;
}

