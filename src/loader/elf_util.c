/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
/*
 * Copyright (c) 2012, LiteStack, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "src/main/tools.h"
#include "src/loader/elf.h"
#include "src/loader/elf_util.h"

/* private */
struct ElfImage {
  Elf_Ehdr  ehdr;
  Elf_Phdr  phdrs[NACL_MAX_PROGRAM_HEADERS];
  int       loadable[NACL_MAX_PROGRAM_HEADERS];
};

enum PhdrCheckAction {
  PCA_NONE,
  PCA_TEXT_CHECK,
  PCA_RODATA,
  PCA_DATA,
  PCA_IGNORE  /* ignore this segment. */
};

struct PhdrChecks {
  Elf_Word                  p_type;
  Elf_Word                  p_flags;  /* rwx */
  enum PhdrCheckAction  action;
  int                       required;  /* only for text for now */
  Elf_Addr                  p_vaddr;  /* if non-zero, vaddr must be this */
};

/*
 * Other than empty segments, these are the only ones that are allowed.
 */
static const struct PhdrChecks phdr_check_data[] = {
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

#define DUMP(m, f) ZLOGS(loglevel, #m " = %" f, elf_hdr->m)
static void DumpElfHeader(int loglevel, const Elf_Ehdr *elf_hdr)
{
  ZLOGS(loglevel, "%020o (Elf header) %020o", 0, 0);

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

#define DUMP(mem, f) ZLOGS(loglevel, "%s: %" f, #mem, phdr->mem)
static void DumpElfProgramHeader(int loglevel, const Elf_Phdr *phdr)
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

void ValidateElfHeader(const struct ElfImage *image)
{
  const Elf_Ehdr *hdr = &image->ehdr;

  ZLOGFAIL(memcmp(hdr->e_ident, ELFMAG, SELFMAG), ENOEXEC, "bad elf magic");
  ZLOGFAIL(ELFCLASS64 != hdr->e_ident[EI_CLASS], ENOEXEC, "bad elf class");
  ZLOGFAIL(ET_EXEC != hdr->e_type, ENOEXEC, "non executable");
  ZLOGFAIL(EM_EXPECTED_BY_NACL != hdr->e_machine, ENOEXEC,
      "bad machine: %x", hdr->e_machine);
  ZLOGFAIL(EV_CURRENT != hdr->e_version, ENOEXEC, "bad elf version: %x", hdr->e_version);
}

void ValidateProgramHeaders(
  struct ElfImage     *image,
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
  int seen_seg[ARRAY_SIZE(phdr_check_data)] = {0};
  int                 segnum;
  const Elf_Phdr      *php;
  size_t              j;

  *max_vaddr = NACL_TRAMPOLINE_END;

  /* phdr_check_data is small, so O(|check_data| * nap->elf_hdr.e_phum) is ok */
  ZLOGS(LOG_DEBUG, "Validating program headers");
  for(segnum = 0; segnum < hdr->e_phnum; ++segnum)
  {
    php = &image->phdrs[segnum];
    ZLOGS(LOG_INSANE, "Looking at segment %d, type 0x%x, p_flags 0x%x",
        segnum, php->p_type, php->p_flags);

    if(0 == php->p_memsz)
    {
      /* We will not load this segment */
      ZLOGS(LOG_INSANE, "Ignoring empty segment");
      continue;
    }

    for(j = 0; j < ARRAY_SIZE_SAFE(phdr_check_data); ++j)
    {
      if(php->p_type == phdr_check_data[j].p_type
          && php->p_flags == phdr_check_data[j].p_flags) break;
    }

    ZLOGFAIL(j == ARRAY_SIZE_SAFE(phdr_check_data), ENOEXEC, "Segment %d "
        "is of unexpected type 0x%x, flag 0x%x", segnum, php->p_type, php->p_flags);

    ZLOGS(LOG_INSANE, "Matched phdr_check_data[%d]", j);
    ZLOGFAIL(seen_seg[j], ENOEXEC, "Segment %d is a type that has been seen", segnum);
    seen_seg[j] = 1;

    if(PCA_IGNORE == phdr_check_data[j].action)
    {
      ZLOGS(LOG_INSANE, "Ignoring");
      continue;
    }

    /* We will load this segment later. Do the sanity checks. */
    ZLOGFAIL(0 != phdr_check_data[j].p_vaddr && (phdr_check_data[j].p_vaddr
        != php->p_vaddr), ENOEXEC, "Segment %d: bad virtual address: 0x%08x, "
        "expected 0x%08x", segnum, php->p_vaddr, phdr_check_data[j].p_vaddr);

    ZLOGFAIL(php->p_vaddr < NACL_TRAMPOLINE_END, ENOEXEC,
        "Segment %d: virtual address 0x%08x too low", segnum, php->p_vaddr);

    /*
     * integer overflow?  Elf_Addr and Elf_Word are uint32_t or
     * uint64_t, so the addition/comparison is well defined.
     */
    ZLOGFAIL(php->p_vaddr + php->p_memsz < php->p_vaddr, ENOEXEC,
        "Segment %d: p_memsz caused integer overflow", segnum);

    ZLOGFAIL(php->p_vaddr + php->p_memsz >= ((Elf_Addr)1U << addr_bits), ENOEXEC,
        "Segment %d: too large, ends at 0x%08x", segnum, php->p_vaddr + php->p_memsz);

    ZLOGFAIL(php->p_filesz > php->p_memsz, ENOEXEC, "Segment %d: file size 0x%08x "
        "larger than memory size 0x%08x", segnum, php->p_filesz, php->p_memsz);

    /* record our decision that we will load this segment */
    image->loadable[segnum] = 1;

    /*
     * NACL_TRAMPOLINE_END <= p_vaddr
     *                     <= p_vaddr + p_memsz
     *                     < ((uintptr_t) 1U << nap->addr_bits)
     */
    if(*max_vaddr < php->p_vaddr + php->p_memsz)
      *max_vaddr = php->p_vaddr + php->p_memsz;

    switch(phdr_check_data[j].action)
    {
      case PCA_TEXT_CHECK:
        ZLOGFAIL(0 == php->p_memsz, ENOEXEC, FAILED_MSG);
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
  for(j = 0; j < ARRAY_SIZE_SAFE(phdr_check_data); ++j)
    ZLOGFAIL(phdr_check_data[j].required && !seen_seg[j], ENOEXEC, FAILED_MSG);

  /*
   * Memory allocation will use NaClRoundPage(nap->break_addr), but
   * the system notion of break is always an exact address.  Even
   * though we must allocate and make accessible multiples of pages,
   * the linux-style brk system call (which returns current break on
   * failure) permits an arbitrarily aligned address as argument.
   */
}

struct ElfImage *ElfImageNew(struct Gio *gp)
{
  struct ElfImage *result;
  struct ElfImage image;
  int             cur_ph;

  memset(image.loadable, 0, sizeof image.loadable);

  /* fail if could not seek to beginning of Gio object containing program */
  ZLOGFAIL(-1 == (*gp->vtbl->Seek)(gp, 0, 0), EIO, FAILED_MSG);

  /* fail if could not load elf headers*/
  ZLOGFAIL((*gp->vtbl->Read)(gp, &image.ehdr, sizeof image.ehdr) != sizeof image.ehdr,
      EIO, FAILED_MSG);

  DumpElfHeader(LOG_INSANE, &image.ehdr);

  /* read program headers. fail if too many prog headers */
  ZLOGFAIL(image.ehdr.e_phnum > NACL_MAX_PROGRAM_HEADERS, ENOEXEC, FAILED_MSG);

  /* fail if ELF program header size too small */
  ZLOGFAIL(image.ehdr.e_phentsize < sizeof image.phdrs[0], ENOEXEC,
      "bad prog headers size. image.ehdr.e_phentsize = 0x%x, sizeof "
      "image.phdrs[0] = 0x%x", image.ehdr.e_phentsize, sizeof image.phdrs[0]);

  /*
   * NB: cast from e_phoff to off_t may not be valid, since off_t can be
   * smaller than Elf64_off, but since invalid values will be rejected
   * by Seek() the cast is safe (cf bsy). fail if cannot seek tp prog headers
   */
  ZLOGFAIL((*gp->vtbl->Seek)(gp, (off_t)image.ehdr.e_phoff, SEEK_SET) == (off_t)-1,
      EIO, FAILED_MSG);

  /* fail if cannot load tp prog headers */
  ZLOGFAIL((size_t)(*gp->vtbl->Read)(gp, &image.phdrs[0], image.ehdr.e_phnum * sizeof
      image.phdrs[0]) != (image.ehdr.e_phnum * sizeof image.phdrs[0]), EIO, FAILED_MSG);

  ZLOGS(LOG_INSANE, "%020o (elf segments) %020o", 0, 0);
  for(cur_ph = 0; cur_ph < image.ehdr.e_phnum; ++cur_ph)
    DumpElfProgramHeader(LOG_INSANE, &image.phdrs[cur_ph]);

  /* we delay allocating till the end to avoid cleanup code */
  /* fail if not enough memory for image meta data */
  result = g_malloc(sizeof image);
  ZLOGFAIL(result == NULL, ENOMEM, FAILED_MSG);

  memcpy(result, &image, sizeof image);
  return result;
}

void ElfImageLoad(const struct ElfImage *image,
    struct Gio *gp, uint8_t addr_bits, uintptr_t mem_start)
{
  int               segnum;
  uintptr_t         paddr;
  uintptr_t         end_vaddr;

  for(segnum = 0; segnum < image->ehdr.e_phnum; ++segnum)
  {
    const Elf_Phdr *php = &image->phdrs[segnum];

    /* did we decide that we will load this segment earlier? */
    if(!image->loadable[segnum]) continue;

    ZLOGS(LOG_INSANE, "loading segment %d", segnum);

    if(0 == php->p_filesz)
    {
      ZLOGS(LOG_INSANE, "zero-sized segment. ignoring...");
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

    ZLOGS(LOG_INSANE, "Seek to position %d (0x%x)", php->p_offset, php->p_offset);

    /*
     * NB: php->p_offset may not be a valid off_t on 64-bit systems, but
     * in that case Seek() will error out.
     * d'b: fail if ELF executable segment header parameter error
     */
    ZLOGFAIL((*gp->vtbl->Seek)(gp, (off_t)php->p_offset, SEEK_SET) == (off_t)-1,
        ENOEXEC, "seek failure segment %d", segnum);

    ZLOGS(LOG_INSANE, "Reading %d (0x%x) bytes to address 0x%x",
        php->p_filesz, php->p_filesz, paddr);

    ZLOGFAIL((Elf_Word)(*gp->vtbl->Read)(gp, (void *)paddr, php->p_filesz) != php->p_filesz,
        ENOEXEC, "load failure segment %d", segnum);
    /* region from p_filesz to p_memsz should already be zero filled */
  }
}

void ElfImageDelete(struct ElfImage *image)
{
  g_free(image);
}

uintptr_t ElfImageGetEntryPoint(const struct ElfImage *image)
{
  return image->ehdr.e_entry;
}
