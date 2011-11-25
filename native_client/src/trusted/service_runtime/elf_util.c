/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl helper functions to deal with elf images
 */

#include "native_client/src/include/portability.h"

#include <stdio.h>

#include <stdlib.h>
#include <string.h>

#define NACL_LOG_MODULE_NAME  "elf_util"

#include "native_client/src/include/elf_constants.h"
#include "native_client/src/include/elf.h"
#include "native_client/src/include/nacl_macros.h"
#include "native_client/src/include/nacl_platform.h"

#include "native_client/src/shared/gio/gio.h"
#include "native_client/src/shared/platform/nacl_log.h"

#include "native_client/src/trusted/service_runtime/elf_util.h"
#include "native_client/src/trusted/service_runtime/include/bits/mman.h"
#include "native_client/src/trusted/service_runtime/nacl_config.h"
#include "native_client/src/trusted/service_runtime/nacl_syscall_common.h"
#include "native_client/src/trusted/service_runtime/nacl_text.h"
#include "native_client/src/trusted/service_runtime/nacl_valgrind_hooks.h"
#include "native_client/src/trusted/service_runtime/sel_memory.h"

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
#if NACL_ARCH(NACL_BUILD_ARCH) == NACL_arm
  /* arm exception handling unwind info (for c++)*/
  /* TODO(robertm): for some reason this does NOT end up in ro maybe because
   *             it is relocatable. Try hacking the linker script to move it.
   */
  { PT_ARM_EXIDX, PF_R, PCA_IGNORE, 0, 0, },
#endif
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



static void NaClDumpElfHeader(int loglevel, Elf_Ehdr *elf_hdr) {

#define DUMP(m,f)    do { NaClLog(loglevel,                     \
                                  #m " = %" f "\n",             \
                                  elf_hdr->m); } while (0)

  NaClLog(loglevel, "=================================================\n");
  NaClLog(loglevel, "Elf header\n");
  NaClLog(loglevel, "==================================================\n");

  DUMP(e_ident+1, ".3s");
  DUMP(e_type, "#x");
  DUMP(e_machine, "#x");
  DUMP(e_version, "#x");
  DUMP(e_entry, "#"NACL_PRIxElf_Addr);
  DUMP(e_phoff, "#"NACL_PRIxElf_Off);
  DUMP(e_shoff, "#"NACL_PRIxElf_Off);
  DUMP(e_flags, "#"NACL_PRIxElf_Word);
  DUMP(e_ehsize, "#"NACL_PRIxElf_Half);
  DUMP(e_phentsize, "#"NACL_PRIxElf_Half);
  DUMP(e_phnum, "#"NACL_PRIxElf_Half);
  DUMP(e_shentsize, "#"NACL_PRIxElf_Half);
  DUMP(e_shnum, "#"NACL_PRIxElf_Half);
  DUMP(e_shstrndx, "#"NACL_PRIxElf_Half);
#undef DUMP
 NaClLog(loglevel, "sizeof(Elf32_Ehdr) = 0x%x\n", (int) sizeof *elf_hdr);
}


static void NaClDumpElfProgramHeader(int     loglevel,
                                    Elf_Phdr *phdr) {
#define DUMP(mem, f) do {                                \
    NaClLog(loglevel, "%s: %" f "\n", #mem, phdr->mem);  \
  } while (0)

  DUMP(p_type, NACL_PRIxElf_Word);
  DUMP(p_offset, NACL_PRIxElf_Off);
  DUMP(p_vaddr, NACL_PRIxElf_Addr);
  DUMP(p_paddr, NACL_PRIxElf_Addr);
  DUMP(p_filesz, NACL_PRIxElf_Xword);
  DUMP(p_memsz, NACL_PRIxElf_Xword);
  DUMP(p_flags, NACL_PRIxElf_Word);
  NaClLog(2, " (%s %s %s)\n",
          (phdr->p_flags & PF_R) ? "PF_R" : "",
          (phdr->p_flags & PF_W) ? "PF_W" : "",
          (phdr->p_flags & PF_X) ? "PF_X" : "");
  DUMP(p_align, NACL_PRIxElf_Xword);
#undef  DUMP
  NaClLog(loglevel, "\n");
}


NaClErrorCode NaClElfImageValidateElfHeader(struct NaClElfImage *image) {
  const Elf_Ehdr *hdr = &image->ehdr;

  if (memcmp(hdr->e_ident, ELFMAG, SELFMAG)) {
    NaClLog(LOG_ERROR, "bad elf magic\n");
    return LOAD_BAD_ELF_MAGIC;
  }

#if NACL_TARGET_SUBARCH == 64
  if (ELFCLASS64 != hdr->e_ident[EI_CLASS]) {
    NaClLog(LOG_ERROR, "bad elf class\n");
    return LOAD_NOT_64_BIT;
  }
#else
  if (ELFCLASS32 != hdr->e_ident[EI_CLASS]) {
    NaClLog(LOG_ERROR, "bad elf class\n");
    return LOAD_NOT_32_BIT;
  }
#endif

  if (ET_EXEC != hdr->e_type) {
    NaClLog(LOG_ERROR, "non executable\n");
    return LOAD_NOT_EXEC;
  }

  if (EM_EXPECTED_BY_NACL != hdr->e_machine) {
    NaClLog(LOG_ERROR, "bad machine: %"NACL_PRIxElf_Half"\n", hdr->e_machine);
    return LOAD_BAD_MACHINE;
  }

  if (EV_CURRENT != hdr->e_version) {
    NaClLog(LOG_ERROR, "bad elf version: %"NACL_PRIxElf_Word"\n",
            hdr->e_version);
    return LOAD_BAD_ELF_VERS;
  }

  return LOAD_OK;
}

/* TODO(robertm): decouple validation from computation of
                   static_text_end and max_vaddr */
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
  for (segnum = 0; segnum < hdr->e_phnum; ++segnum) {
    php = &image->phdrs[segnum];
    NaClLog(3, "Looking at segment %d, type 0x%x, p_flags 0x%x\n",
            segnum, php->p_type, php->p_flags);
    if (0 == php->p_memsz) {
      /*
       * We will not load this segment.
       */
      NaClLog(3, "Ignoring empty segment\n");
      continue;
    }

    for (j = 0; j < NACL_ARRAY_SIZE(nacl_phdr_check_data); ++j) {
      if (php->p_type == nacl_phdr_check_data[j].p_type
          && php->p_flags == nacl_phdr_check_data[j].p_flags)
        break;
    }
    if (j == NACL_ARRAY_SIZE(nacl_phdr_check_data)) {
      /* segment not in nacl_phdr_check_data */
      NaClLog(2,
              "Segment %d is of unexpected type 0x%x, flag 0x%x\n",
              segnum,
              php->p_type,
              php->p_flags);
      return LOAD_BAD_SEGMENT;
    }

    NaClLog(2, "Matched nacl_phdr_check_data[%"NACL_PRIdS"]\n", j);
    if (seen_seg[j]) {
      NaClLog(2, "Segment %d is a type that has been seen\n", segnum);
      return LOAD_DUP_SEGMENT;
    }
    seen_seg[j] = 1;

    if (PCA_IGNORE == nacl_phdr_check_data[j].action) {
      NaClLog(3, "Ignoring\n");
      continue;
    }

    /*
     * We will load this segment later.  Do the sanity checks.
     */
    if (0 != nacl_phdr_check_data[j].p_vaddr
        && (nacl_phdr_check_data[j].p_vaddr != php->p_vaddr)) {
      NaClLog(2,
              ("Segment %d: bad virtual address: 0x%08"
               NACL_PRIxElf_Addr","
               " expected 0x%08"NACL_PRIxElf_Addr"\n"),
              segnum,
              php->p_vaddr,
              nacl_phdr_check_data[j].p_vaddr);
      return LOAD_SEGMENT_BAD_LOC;
    }
    if (php->p_vaddr < NACL_TRAMPOLINE_END) {
      NaClLog(2,
              ("Segment %d: virtual address (0x%08"NACL_PRIxElf_Addr
               ") too low\n"),
              segnum,
              php->p_vaddr);
      return LOAD_SEGMENT_OUTSIDE_ADDRSPACE;
    }
    /*
     * integer overflow?  Elf_Addr and Elf_Word are uint32_t or
     * uint64_t, so the addition/comparison is well defined.
     */
    if (php->p_vaddr + php->p_memsz < php->p_vaddr) {
      NaClLog(2,
              "Segment %d: p_memsz caused integer overflow\n",
              segnum);
      return LOAD_SEGMENT_OUTSIDE_ADDRSPACE;
    }
    if (php->p_vaddr + php->p_memsz >= ((Elf_Addr) 1U << addr_bits)) {
      NaClLog(2,
              "Segment %d: too large, ends at 0x%08"NACL_PRIxElf_Addr"\n",
              segnum,
              php->p_vaddr + php->p_memsz);
      return LOAD_SEGMENT_OUTSIDE_ADDRSPACE;
    }
    if (php->p_filesz > php->p_memsz) {
      NaClLog(2,
              ("Segment %d: file size 0x%08"NACL_PRIxElf_Xword" larger"
               " than memory size 0x%08"NACL_PRIxElf_Xword"\n"),
              segnum,
              php->p_filesz,
              php->p_memsz);
      return LOAD_SEGMENT_BAD_PARAM;
    }

    image->loadable[segnum] = 1;
    /* record our decision that we will load this segment */

    /*
     * NACL_TRAMPOLINE_END <= p_vaddr
     *                     <= p_vaddr + p_memsz
     *                     < ((uintptr_t) 1U << nap->addr_bits)
     */
    if (*max_vaddr < php->p_vaddr + php->p_memsz) {
      *max_vaddr = php->p_vaddr + php->p_memsz;
    }

    switch (nacl_phdr_check_data[j].action) {
      case PCA_NONE:
        break;
      case PCA_TEXT_CHECK:
        if (0 == php->p_memsz) {
          return LOAD_BAD_ELF_TEXT;
        }
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
  for (j = 0; j < NACL_ARRAY_SIZE(nacl_phdr_check_data); ++j) {
    if (nacl_phdr_check_data[j].required && !seen_seg[j]) {
      return LOAD_REQUIRED_SEG_MISSING;
    }
  }

  /*
   * Memory allocation will use NaClRoundPage(nap->break_addr), but
   * the system notion of break is always an exact address.  Even
   * though we must allocate and make accessible multiples of pages,
   * the linux-style brk system call (which returns current break on
   * failure) permits an arbitrarily aligned address as argument.
   */

  return LOAD_OK;
}


struct NaClElfImage *NaClElfImageNew(struct Gio     *gp,
                                     NaClErrorCode  *err_code) {
  struct NaClElfImage *result;
  struct NaClElfImage image;
  int                 cur_ph;

  memset(image.loadable, 0, sizeof image.loadable);
  if (-1 == (*gp->vtbl->Seek)(gp, 0, 0)) {
    NaClLog(2, "could not seek to beginning of Gio object containing nexe\n");
    if (NULL != err_code) {
      *err_code = LOAD_READ_ERROR;
    }
    return 0;
  }
  if ((*gp->vtbl->Read)(gp,
                        &image.ehdr,
                        sizeof image.ehdr)
      != sizeof image.ehdr) {
    if (NULL != err_code) {
      *err_code = LOAD_READ_ERROR;
    }
    NaClLog(2, "could not load elf headers\n");
    return 0;
  }

  NaClDumpElfHeader(2, &image.ehdr);

  /* read program headers */
  if (image.ehdr.e_phnum > NACL_MAX_PROGRAM_HEADERS) {
    if (NULL != err_code)
      *err_code = LOAD_TOO_MANY_PROG_HDRS;
    NaClLog(2, "too many prog headers\n");
    return 0;
  }

  if (image.ehdr.e_phentsize < sizeof image.phdrs[0]) {
    if (NULL != err_code) {
      *err_code = LOAD_PROG_HDR_SIZE_TOO_SMALL;
    }
    NaClLog(2, "bad prog headers size\n");
    NaClLog(2, " image.ehdr.e_phentsize = 0x%"NACL_PRIxElf_Half"\n",
            image.ehdr.e_phentsize);
    NaClLog(2, "  sizeof image.phdrs[0] = 0x%"NACL_PRIxS"\n",
            sizeof image.phdrs[0]);
    return 0;
  }

  /*
   * NB: cast from e_phoff to off_t may not be valid, since off_t can be
   * smaller than Elf64_off, but since invalid values will be rejected
   * by Seek() the cast is safe (cf bsy)
   */
  if ((*gp->vtbl->Seek)(gp,
                        (off_t) image.ehdr.e_phoff,
                        SEEK_SET) == (off_t) -1) {
    if (NULL != err_code) {
      *err_code = LOAD_READ_ERROR;
    }
    NaClLog(2, "cannot seek tp prog headers\n");
    return 0;
  }

  if ((size_t) (*gp->vtbl->Read)(gp,
                                 &image.phdrs[0],
                                 image.ehdr.e_phnum * sizeof image.phdrs[0])
      != (image.ehdr.e_phnum * sizeof image.phdrs[0])) {
    if (NULL != err_code) {
      *err_code = LOAD_READ_ERROR;
    }
    NaClLog(2, "cannot load tp prog headers\n");
    return 0;
  }

  NaClLog(2, "=================================================\n");
  NaClLog(2, "Elf Program headers\n");
  NaClLog(2, "==================================================\n");
  for (cur_ph = 0; cur_ph <  image.ehdr.e_phnum; ++cur_ph) {
    NaClDumpElfProgramHeader(2, &image.phdrs[cur_ph]);
  }

  /* we delay allocating till the end to avoid cleanup code */
  result = malloc(sizeof image);
  if (result == 0) {
    if (NULL != err_code) {
      *err_code = LOAD_NO_MEMORY;
    }
    NaClLog(LOG_FATAL, "no enough memory for image meta data\n");
    return 0;
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

  for (segnum = 0; segnum < image->ehdr.e_phnum; ++segnum) {
    const Elf_Phdr *php = &image->phdrs[segnum];

    /* did we decide that we will load this segment earlier? */
    if (!image->loadable[segnum]) {
      continue;
    }

    NaClLog(2, "loading segment %d\n", segnum);

    if (0 == php->p_filesz) {
      NaClLog(4, "zero-sized segment.  ignoring...\n");
      continue;
    }

    end_vaddr = php->p_vaddr + php->p_filesz;
    /* integer overflow? */
    if (end_vaddr < php->p_vaddr) {
      NaClLog(LOG_FATAL, "parameter error should have been detected already\n");
    }
    /*
     * is the end virtual address within the NaCl application's
     * address space?  if it is, it implies that the start virtual
     * address is also.
     */
    if (end_vaddr >= ((uintptr_t) 1U << addr_bits)) {
      NaClLog(LOG_FATAL, "parameter error should have been detected already\n");
    }

    paddr = mem_start + php->p_vaddr;

    NaClLog(4,
            "Seek to position %"NACL_PRIdElf_Off" (0x%"NACL_PRIxElf_Off").\n",
            php->p_offset,
            php->p_offset);

    /*
     * NB: php->p_offset may not be a valid off_t on 64-bit systems, but
     * in that case Seek() will error out.
     */
    if ((*gp->vtbl->Seek)(gp, (off_t) php->p_offset, SEEK_SET) == (off_t) -1) {
      NaClLog(LOG_ERROR, "seek failure segment %d", segnum);
      return LOAD_SEGMENT_BAD_PARAM;
    }
    NaClLog(4,
            "Reading %"NACL_PRIdElf_Xword" (0x%"NACL_PRIxElf_Xword") bytes to"
            " address 0x%"NACL_PRIxPTR"\n",
            php->p_filesz,
            php->p_filesz,
            paddr);

    /*
     * Tell valgrind that this memory is accessible and undefined. For more
     * details see
     * http://code.google.com/p/nativeclient/wiki/ValgrindMemcheck#Implementation_details
     */
    NACL_MAKE_MEM_UNDEFINED((void *) paddr, php->p_filesz);

    if ((Elf_Word) (*gp->vtbl->Read)(gp, (void *) paddr, php->p_filesz)
        != php->p_filesz) {
      NaClLog(LOG_ERROR, "load failure segment %d", segnum);
      return LOAD_SEGMENT_BAD_PARAM;
    }
    /* region from p_filesz to p_memsz should already be zero filled */

    /* Tell Valgrind that we've mapped a segment of nacl_file. */
    NaClFileMappingForValgrind(paddr, php->p_filesz, php->p_offset);
  }

  return LOAD_OK;
}


NaClErrorCode NaClElfImageLoadDynamically(struct NaClElfImage *image,
                                          struct NaClApp      *nap,
                                          struct Gio          *gfile) {
  int segnum;
  for (segnum = 0; segnum < image->ehdr.e_phnum; ++segnum) {
    const Elf_Phdr *php = &image->phdrs[segnum];
    int32_t result;

    /*
     * We check for PT_LOAD directly rather than using the "loadable"
     * array because we are not using NaClElfImageValidateProgramHeaders()
     * to fill out the "loadable" array for this ELF object.  This ELF
     * object does not have to fit such strict constraints (such as
     * having code at 0x20000), and safety checks are applied by
     * NaClTextDyncodeCreate() and NaClCommonSysMmapIntern().
     */
    if (PT_LOAD != php->p_type) {
      continue;
    }

    /*
     * Ideally, Gio would have a Pread() method which we would use
     * instead of Seek().  In practice, though, there is no
     * Seek()/Read() race condition here because both
     * GioMemoryFileSnapshot and NaClGioShm use a seek position that
     * is local and not shared between processes.
     */
    if ((*gfile->vtbl->Seek)(gfile, (off_t) php->p_offset,
                             SEEK_SET) == (off_t) -1) {
      NaClLog(1, "NaClElfImageLoadDynamically: seek failed\n");
      return LOAD_READ_ERROR;
    }

    if (0 != (php->p_flags & PF_X)) {
      /* Load code segment. */
      /*
       * We make a copy of the code.  This is not ideal given that
       * GioMemoryFileSnapshot and NaClGioShm already have a copy of
       * the file in memory or mmapped.
       * TODO(mseaborn): Reduce the amount of copying here.
       */
      char *code_copy = malloc(php->p_filesz);
      if (NULL == code_copy) {
        NaClLog(1, "NaClElfImageLoadDynamically: malloc failed\n");
        return LOAD_NO_MEMORY;
      }
      if ((Elf_Word) (*gfile->vtbl->Read)(gfile, code_copy, php->p_filesz)
          != php->p_filesz) {
        free(code_copy);
        NaClLog(1, "NaClElfImageLoadDynamically: "
                "failed to read code segment\n");
        return LOAD_READ_ERROR;
      }
      result = NaClTextDyncodeCreate(nap, (uint32_t) php->p_vaddr,
                                     code_copy, (uint32_t) php->p_filesz);
      free(code_copy);
      if (0 != result) {
        NaClLog(1, "NaClElfImageLoadDynamically: "
                "failed to load code segment\n");
        return LOAD_UNLOADABLE;
      }
    } else {
      /* Load data segment. */
      void *paddr = (void *) NaClUserToSys(nap, php->p_vaddr);
      size_t mapping_size = NaClRoundAllocPage(php->p_memsz);
      /*
       * Note that we do not used NACL_ABI_MAP_FIXED because we do not
       * want to silently overwrite any existing mappings, such as the
       * user app's data segment or the stack.  We detect overmapping
       * when mmap chooses not to use the preferred address we supply.
       * (Ideally mmap would provide a MAP_EXCL option for this
       * instead.)
       */
      result = NaClCommonSysMmapIntern(
          nap, (void *) php->p_vaddr, mapping_size,
          NACL_ABI_PROT_READ | NACL_ABI_PROT_WRITE,
          NACL_ABI_MAP_ANONYMOUS | NACL_ABI_MAP_PRIVATE,
          -1, 0);
      if ((int32_t) php->p_vaddr != result) {
        NaClLog(1, "NaClElfImageLoadDynamically: failed to map data segment\n");
        return LOAD_UNLOADABLE;
      }
      if ((Elf_Word) (*gfile->vtbl->Read)(gfile, paddr, php->p_filesz)
          != php->p_filesz) {
        NaClLog(1, "NaClElfImageLoadDynamically: "
                "failed to read data segment\n");
        return LOAD_READ_ERROR;
      }
      /*
       * Note that we do not need to zero the BSS (the region from
       * p_filesz to p_memsz) because it should already be zero
       * filled.  This would not be the case if we were mapping the
       * data segment from the file.
       */

      if (0 == (php->p_flags & PF_W)) {
        /* Handle read-only data segment. */
        int rc = NaCl_mprotect(paddr, mapping_size, NACL_ABI_PROT_READ);
        if (0 != rc) {
          NaClLog(1, "NaClElfImageLoadDynamically: "
                  "failed to mprotect read-only data segment\n");
          return LOAD_MPROTECT_FAIL;
        }

        NaClVmmapUpdate(&nap->mem_map,
                        php->p_vaddr >> NACL_PAGESHIFT,
                        mapping_size >> NACL_PAGESHIFT,
                        PROT_READ,
                        NULL,
                        0  /* remove: false */);
      }
    }
  }
  return LOAD_OK;
}


void NaClElfImageDelete(struct NaClElfImage *image) {
  free(image);
}


uintptr_t NaClElfImageGetEntryPoint(struct NaClElfImage *image) {
  return image->ehdr.e_entry;
}
