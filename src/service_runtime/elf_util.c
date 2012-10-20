/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl helper functions to deal with elf images
 */
#include <string.h>

#define NACL_LOG_MODULE_NAME  "elf_util"

#include "src/include/nacl_macros.h"
#include "src/include/nacl_platform.h"
#include "src/gio/gio.h"
#include "src/service_runtime/elf_util.h"
#include "src/service_runtime/include/bits/mman.h"
#include "src/service_runtime/sel_memory.h"
#include "src/service_runtime/nacl_syscall_handlers.h"

/* private */
struct NaClElfImage {
  Elf_Ehdr  ehdr;
  Elf_Phdr  phdrs[NACL_MAX_PROGRAM_HEADERS];
  int       loadable[NACL_MAX_PROGRAM_HEADERS];
};

enum NaClPhdrCheckAction {
  PCA_NONE,
  PCA_TEXT_CHECK,
  PCA_RODATA,
  PCA_DATA,
  PCA_IGNORE  /* ignore this segment. */
};

struct NaClPhdrChecks {
  Elf_Word                  p_type;
  Elf_Word                  p_flags;  /* rwx */
  enum NaClPhdrCheckAction  action;
  int                       required;  /* only for text for now */
  Elf_Addr                  p_vaddr;  /* if non-zero, vaddr must be this */
};

/*
 * Other than empty segments, these are the only ones that are allowed.
 */
static const struct NaClPhdrChecks nacl_phdr_check_data[] = {
  /* phdr */
  { PT_PHDR, PF_R, PCA_IGNORE, 0, 0, },
  /* text */
  { PT_LOAD, PF_R|PF_X, PCA_TEXT_CHECK, 1, NACL_TRAMPOLINE_END, },
  /* rodata */
  { PT_LOAD, PF_R, PCA_RODATA, 0, 0, },
  /* data/bss */
  { PT_LOAD, PF_R|PF_W, PCA_DATA, 0, 0, },
  /* tls */
  { PT_TLS, PF_R, PCA_IGNORE, 0, 0},

  /*
   * allow optional GNU stack permission marker, but require that the
   * stack is non-executable.
   */
  { PT_GNU_STACK, PF_R|PF_W, PCA_NONE, 0, 0, },
  /* ignored segments */
  { PT_DYNAMIC, PF_R, PCA_IGNORE, 0, 0},
  { PT_INTERP, PF_R, PCA_IGNORE, 0, 0},
  { PT_NOTE, PF_R, PCA_IGNORE, 0, 0},
  { PT_GNU_EH_FRAME, PF_R, PCA_IGNORE, 0, 0},
  { PT_GNU_RELRO, PF_R, PCA_IGNORE, 0, 0},
  { PT_NULL, PF_R, PCA_IGNORE, 0, 0},
};

#define DUMP(m, f) do { ZLOGS(loglevel, #m " = %" f, elf_hdr->m); } while (0)
static void NaClDumpElfHeader(int loglevel, Elf_Ehdr *elf_hdr)
{
//  ZLOGS(loglevel, "=================================================");
//  ZLOGS(loglevel, "Elf header");
//  ZLOGS(loglevel, "==================================================");
  ZLOGS(LOG_DEBUG, "%020o (Elf header) %020o", 0, 0);

  DUMP(e_ident+1, ".3s");
  DUMP(e_type, "#x");
  DUMP(e_machine, "#x");
  DUMP(e_version, "#x");
  DUMP(e_entry, "#x");
  DUMP(e_phoff, "#x");
  DUMP(e_shoff, "#x");
  DUMP(e_flags, "#x");
  DUMP(e_ehsize, "#x");
  DUMP(e_phentsize, "#x");
  DUMP(e_phnum, "#x");
  DUMP(e_shentsize, "#x");
  DUMP(e_shnum, "#x");
  DUMP(e_shstrndx, "#x");
 ZLOGS(loglevel, "sizeof(Elf32_Ehdr) = 0x%x", (int) sizeof *elf_hdr);
}
#undef DUMP

#define DUMP(mem, f) do { ZLOGS(loglevel, "%s: %" f, #mem, phdr->mem); } while (0)
static void NaClDumpElfProgramHeader(int loglevel, Elf_Phdr *phdr)
{
  DUMP(p_type, "x");
  DUMP(p_offset, "x");
  DUMP(p_vaddr, "x");
  DUMP(p_paddr, "x");
  DUMP(p_filesz, "x");
  DUMP(p_memsz, "x");
  DUMP(p_flags, "x");
  ZLOGS(loglevel, " (%s %s %s)",
      (phdr->p_flags & PF_R) ? "PF_R" : "",
      (phdr->p_flags & PF_W) ? "PF_W" : "",
      (phdr->p_flags & PF_X) ? "PF_X" : "");
  DUMP(p_align, "x");
}
#undef  DUMP

NaClErrorCode NaClElfImageValidateElfHeader(struct NaClElfImage *image)
{
  const Elf_Ehdr *hdr = &image->ehdr;

  if (memcmp(hdr->e_ident, ELFMAG, SELFMAG)) {
    ZLOG(LOG_ERROR, "bad elf magic");
    return LOAD_BAD_ELF_MAGIC;
  }

  if (ELFCLASS64 != hdr->e_ident[EI_CLASS]) {
    ZLOG(LOG_ERROR, "bad elf class");
    return LOAD_NOT_64_BIT;
  }

  if (ET_EXEC != hdr->e_type) {
    ZLOG(LOG_ERROR, "non executable");
    return LOAD_NOT_EXEC;
  }

  if (EM_EXPECTED_BY_NACL != hdr->e_machine) {
    ZLOG(LOG_ERROR, "bad machine: %x", hdr->e_machine);
    return LOAD_BAD_MACHINE;
  }

  if (EV_CURRENT != hdr->e_version) {
    ZLOG(LOG_ERROR, "bad elf version: %x", hdr->e_version);
    return LOAD_BAD_ELF_VERS;
  }

  return LOAD_OK;
}

/*
 * TODO(robertm): decouple validation from computation of static_text_end
 * and max_vaddr
 * todo(d'b): the function is too large, rewrite it
 */
NaClErrorCode NaClElfImageValidateProgramHeaders(
  struct NaClElfImage *image,
  uint8_t             addr_bits,
  uintptr_t           *static_text_end,
  uintptr_t           *rodata_start,
  uintptr_t           *rodata_end,
  uintptr_t           *data_start,
  uintptr_t           *data_end,
  uintptr_t           *max_vaddr) {
    /*
     * Scan phdrs and do sanity checks in-line.  Verify that the load
     * address is NACL_TRAMPOLINE_END, that we have a single text
     * segment.  Data and TLS segments are not required, though it is
     * hard to avoid with standard tools, but in any case there should
     * be at most one each.  Ensure that no segment's vaddr is outside
     * of the address space.  Ensure that PT_GNU_STACK is present, and
     * that x is off.
     */
  const Elf_Ehdr      *hdr = &image->ehdr;
  int                 seen_seg[NACL_ARRAY_SIZE(nacl_phdr_check_data)];

  int                 segnum;
  const Elf_Phdr      *php;
  size_t              j;

  *max_vaddr = NACL_TRAMPOLINE_END;

  /*
   * nacl_phdr_check_data is small, so O(|check_data| * nap->elf_hdr.e_phum)
   * is okay.
   */
  memset(seen_seg, 0, sizeof seen_seg);
  for(segnum = 0; segnum < hdr->e_phnum; ++segnum)
  {
    php = &image->phdrs[segnum];
    ZLOGS(LOG_DEBUG, "Looking at segment %d, type 0x%x, p_flags 0x%x",
        segnum, php->p_type, php->p_flags);

    if(0 == php->p_memsz)
    {
      /* We will not load this segment */
      ZLOGS(LOG_DEBUG, "Ignoring empty segment");
      continue;
    }

    for(j = 0; j < NACL_ARRAY_SIZE(nacl_phdr_check_data); ++j)
    {
      if(php->p_type == nacl_phdr_check_data[j].p_type
          && php->p_flags == nacl_phdr_check_data[j].p_flags) break;
    }

    if(j == NACL_ARRAY_SIZE(nacl_phdr_check_data))
    {
      /* segment not in nacl_phdr_check_data */
      ZLOGS(LOG_DEBUG, "Segment %d is of unexpected type 0x%x, flag 0x%x",
          segnum, php->p_type, php->p_flags);
      return LOAD_BAD_SEGMENT;
    }

    ZLOGS(LOG_DEBUG, "Matched nacl_phdr_check_data[%d]", j);
    if(seen_seg[j])
    {
      ZLOGS(LOG_DEBUG, "Segment %d is a type that has been seen", segnum);
      return LOAD_DUP_SEGMENT;
    }
    seen_seg[j] = 1;

    if(PCA_IGNORE == nacl_phdr_check_data[j].action)
    {
      ZLOGS(LOG_DEBUG, "Ignoring");
      continue;
    }

    /* We will load this segment later. Do the sanity checks. */
    if (0 != nacl_phdr_check_data[j].p_vaddr
        && (nacl_phdr_check_data[j].p_vaddr != php->p_vaddr))
    {
      ZLOGS(LOG_DEBUG, "Segment %d: bad virtual address: 0x%08x, expected 0x%08x",
              segnum, php->p_vaddr, nacl_phdr_check_data[j].p_vaddr);
      return LOAD_SEGMENT_BAD_LOC;
    }

    if (php->p_vaddr < NACL_TRAMPOLINE_END)
    {
      ZLOGS(LOG_DEBUG, "Segment %d: virtual address 0x%08x too low", segnum, php->p_vaddr);
      return LOAD_SEGMENT_OUTSIDE_ADDRSPACE;
    }

    /*
     * integer overflow?  Elf_Addr and Elf_Word are uint32_t or
     * uint64_t, so the addition/comparison is well defined.
     */
    if(php->p_vaddr + php->p_memsz < php->p_vaddr)
    {
      ZLOGS(LOG_DEBUG, "Segment %d: p_memsz caused integer overflow", segnum);
      return LOAD_SEGMENT_OUTSIDE_ADDRSPACE;
    }

    if(php->p_vaddr + php->p_memsz >= ((Elf_Addr)1U << addr_bits))
    {
      ZLOGS(LOG_DEBUG, "Segment %d: too large, ends at 0x%08x",
          segnum, php->p_vaddr + php->p_memsz);
      return LOAD_SEGMENT_OUTSIDE_ADDRSPACE;
    }

    if(php->p_filesz > php->p_memsz)
    {
      ZLOGS(LOG_DEBUG, "Segment %d: file size 0x%08x larger than memory size 0x%08x",
          segnum, php->p_filesz, php->p_memsz);
      return LOAD_SEGMENT_BAD_PARAM;
    }

    /* record our decision that we will load this segment */
    image->loadable[segnum] = 1;

    /*
     * NACL_TRAMPOLINE_END <= p_vaddr
     *                     <= p_vaddr + p_memsz
     *                     < ((uintptr_t) 1U << nap->addr_bits)
     */
    if(*max_vaddr < php->p_vaddr + php->p_memsz)
      *max_vaddr = php->p_vaddr + php->p_memsz;

    switch(nacl_phdr_check_data[j].action)
    {
      case PCA_NONE:
        break;
      case PCA_TEXT_CHECK:
        if(0 == php->p_memsz)
          return LOAD_BAD_ELF_TEXT;
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
      case PCA_IGNORE:
        break;
    }
  }

  for(j = 0; j < NACL_ARRAY_SIZE(nacl_phdr_check_data); ++j)
    if(nacl_phdr_check_data[j].required && !seen_seg[j])
      return LOAD_REQUIRED_SEG_MISSING;

  /*
   * Memory allocation will use NaClRoundPage(nap->break_addr), but
   * the system notion of break is always an exact address.  Even
   * though we must allocate and make accessible multiples of pages,
   * the linux-style brk system call (which returns current break on
   * failure) permits an arbitrarily aligned address as argument.
   */

  return LOAD_OK;
}

struct NaClElfImage *NaClElfImageNew(struct Gio *gp, NaClErrorCode *err_code)
{
  struct NaClElfImage *result;
  struct NaClElfImage image;
  int                 cur_ph;

  memset(image.loadable, 0, sizeof image.loadable);
  if(-1 == (*gp->vtbl->Seek)(gp, 0, 0))
  {
    ZLOGS(LOG_DEBUG, "could not seek to beginning of Gio object containing nexe");
    if(NULL != err_code) *err_code = LOAD_READ_ERROR;
    return 0;
  }

  if((*gp->vtbl->Read)(gp, &image.ehdr, sizeof image.ehdr) != sizeof image.ehdr)
  {
    if(NULL != err_code) *err_code = LOAD_READ_ERROR;
    ZLOGS(LOG_DEBUG, "could not load elf headers");
    return 0;
  }

  NaClDumpElfHeader(LOG_DEBUG, &image.ehdr);

  /* read program headers */
  if(image.ehdr.e_phnum > NACL_MAX_PROGRAM_HEADERS)
  {
    if(NULL != err_code) *err_code = LOAD_TOO_MANY_PROG_HDRS;
    ZLOGS(LOG_DEBUG, "too many prog headers");
    return 0;
  }

  if(image.ehdr.e_phentsize < sizeof image.phdrs[0])
  {
    if(NULL != err_code) *err_code = LOAD_PROG_HDR_SIZE_TOO_SMALL;
    ZLOGS(LOG_DEBUG, "bad prog headers size. image.ehdr.e_phentsize = 0x%x, sizeof"
        " image.phdrs[0] = 0x%x", image.ehdr.e_phentsize, sizeof image.phdrs[0]);
    return 0;
  }

  /*
   * NB: cast from e_phoff to off_t may not be valid, since off_t can be
   * smaller than Elf64_off, but since invalid values will be rejected
   * by Seek() the cast is safe (cf bsy)
   */
  if((*gp->vtbl->Seek)(gp, (off_t)image.ehdr.e_phoff, SEEK_SET) == (off_t)-1)
  {
    if(NULL != err_code) *err_code = LOAD_READ_ERROR;
    ZLOGS(LOG_DEBUG, "cannot seek tp prog headers");
    return 0;
  }

  if((size_t)(*gp->vtbl->Read)(gp, &image.phdrs[0], image.ehdr.e_phnum * sizeof image.phdrs[0])
      != (image.ehdr.e_phnum * sizeof image.phdrs[0]))
  {
    if(NULL != err_code) *err_code = LOAD_READ_ERROR;
    ZLOGS(LOG_DEBUG, "cannot load tp prog headers");
    return 0;
  }

//  ZLOGS(LOG_DEBUG, "=================================================");
//  ZLOGS(LOG_DEBUG, "Elf Program headers");
//  ZLOGS(LOG_DEBUG, "==================================================");
  ZLOGS(LOG_DEBUG, "%020o Elf Program headers %020o", 0, 0);
  for(cur_ph = 0; cur_ph < image.ehdr.e_phnum; ++cur_ph)
    NaClDumpElfProgramHeader(LOG_DEBUG, &image.phdrs[cur_ph]);

  /* we delay allocating till the end to avoid cleanup code */
  result = malloc(sizeof image);
  if(result == 0)
  {
    if(NULL != err_code) *err_code = LOAD_NO_MEMORY;
    ZLOGFAIL(1, ENOMEM, "not enough memory for image meta data");
//    return 0;
  }
  memcpy(result, &image, sizeof image);
  return result;
}

NaClErrorCode NaClElfImageLoad(struct NaClElfImage *image,
                               struct Gio          *gp,
                               uint8_t             addr_bits,
                               uintptr_t           mem_start) {
  int               segnum;
  uintptr_t         paddr;
  uintptr_t         end_vaddr;

  for(segnum = 0; segnum < image->ehdr.e_phnum; ++segnum)
  {
    const Elf_Phdr *php = &image->phdrs[segnum];

    /* did we decide that we will load this segment earlier? */
    if(!image->loadable[segnum]) continue;

    ZLOGS(LOG_DEBUG, "loading segment %d", segnum);

    if(0 == php->p_filesz)
    {
      ZLOGS(LOG_DEBUG, "zero-sized segment.  ignoring...");
      continue;
    }

    end_vaddr = php->p_vaddr + php->p_filesz;
    /* integer overflow? */
    ZLOGFAIL(end_vaddr < php->p_vaddr, EFAULT,
        "parameter error should have been detected already");

    /*
     * is the end virtual address within the NaCl application's
     * address space?  if it is, it implies that the start virtual
     * address is also.
     */
    ZLOGFAIL(end_vaddr >= ((uintptr_t)1U << addr_bits), EFAULT,
        "parameter error should have been detected already");

    paddr = mem_start + php->p_vaddr;

    ZLOGS(LOG_DEBUG, "Seek to position %d (0x%x)", php->p_offset, php->p_offset);

    /*
     * NB: php->p_offset may not be a valid off_t on 64-bit systems, but
     * in that case Seek() will error out.
     */
    if((*gp->vtbl->Seek)(gp, (off_t)php->p_offset, SEEK_SET) == (off_t)-1)
    {
      ZLOG(LOG_ERROR, "seek failure segment %d", segnum);
      return LOAD_SEGMENT_BAD_PARAM;
    }

    ZLOGS(LOG_DEBUG, "Reading %d (0x%x) bytes to address 0x%x",
        php->p_filesz, php->p_filesz, paddr);

    /*
     * Tell valgrind that this memory is accessible and undefined. For more
     * details see
     * http://code.google.com/p/nativeclient/wiki/ValgrindMemcheck#Implementation_details
     */
    NACL_MAKE_MEM_UNDEFINED((void *) paddr, php->p_filesz);

    if((Elf_Word)(*gp->vtbl->Read)(gp, (void *)paddr, php->p_filesz) != php->p_filesz)
    {
      ZLOG(LOG_ERROR, "load failure segment %d", segnum);
      return LOAD_SEGMENT_BAD_PARAM;
    }
    /* region from p_filesz to p_memsz should already be zero filled */
  }

  return LOAD_OK;
}

void NaClElfImageDelete(struct NaClElfImage *image) {
  free(image);
}

uintptr_t NaClElfImageGetEntryPoint(struct NaClElfImage *image) {
  return image->ehdr.e_entry;
}
