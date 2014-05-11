/*
 * untrusted boot (UBOOT) prototype. mini version with reduced checks
 */
#ifndef BOOT_H__
#define BOOT_H__ 1

/* constants */
#define ADDR_BITS (32)
#define NACL_MAX_PROGRAM_HEADERS 128
#define NACL_NULL_REGION_SHIFT   16
#define NACL_TRAMPOLINE_START    (1 << NACL_NULL_REGION_SHIFT)
#define NACL_TRAMPOLINE_SHIFT    16
#define NACL_TRAMPOLINE_SIZE     (1 << NACL_TRAMPOLINE_SHIFT)
#define NACL_TRAMPOLINE_END      (NACL_TRAMPOLINE_START + NACL_TRAMPOLINE_SIZE)
#define NACL_MAP_PAGESHIFT       16
#define NACL_MAP_PAGESIZE        (1U << NACL_MAP_PAGESHIFT)
#define NACL_HALT_SLED_SIZE      32
#define NACL_BLOCK_SHIFT 5
#define NACL_INSTR_BLOCK_SIZE    (1 << NACL_BLOCK_SHIFT)

/* "/boot/elf" reserved for channel with elf to load */
#define SELFNAME_1 0x6c652f746f6f622fLLU /* "/boot/el" */
#define SELFNAME_2 ((uint16_t)0x66) /* "f\000" */

/* macros */
#define FAILIF(c) do { if(c) zvm_exit(__LINE__); } while(0)
#define ARRAY_SIZE(arr) ((sizeof arr)/sizeof arr[0])
#define ROUNDUP_64K(a) ROUNDDOWN_64K((a) + NACL_MAP_PAGESIZE - 1LLU)
#define ROUNDDOWN_64K(a) ((a) & ~(NACL_MAP_PAGESIZE - 1LLU))

/* taken from zerovm elf.h {{ */
#define EI_NIDENT     16 /* fwd, see rest of EI_* below */
#define ET_NONE       0  /* no file type */
#define ET_REL        1  /* relocatable file */
#define ET_EXEC       2  /* executable file */
#define ET_DYN        3  /* shared object file */
#define ET_CORE       4  /* core file */

/* all platforms (but x86_64) removed */
#define EM_X86_64     62 /* x86-64 */
#define EM_EXPECTED_BY_NACL EM_X86_64

#define EV_NONE       0  /* invalid version */
#define EV_CURRENT    1  /* current version */
#define EI_MAG0       0  /* file identification */
#define EI_MAG1       1  /* file identification */
#define EI_MAG2       2  /* file identification */
#define EI_MAG3       3  /* file identification */
#define EI_CLASS      4  /* file class */
#define EI_DATA       5  /* data encoding */
#define EI_VERSION    6  /* file version */

/*
 * ELFMAG and SELFMAG are names/values from linux elf.h, for code usage
 * compatibility.
 */
#define ELFMAG        0x464c457f /* "\177ELF" */

/* EI_CLASS values */
#define ELFCLASSNONE  0
#define ELFCLASS32    1
#define ELFCLASS64    2

/* EI_DATA values */
#define ELFDATANONE   0
#define ELFDATA2LSB   1
#define ELFDATA2MSB   2

#define PT_NULL       0           /* Unused entry */
#define PT_LOAD       1           /* Loadable segment */
#define PT_DYNAMIC    2           /* Dynamic linking tables */
#define PT_INTERP     3           /* Program interpreter path name */
#define PT_NOTE       4           /* Note section */
#define PT_SHLIB      5           /* Reserved */
#define PT_PHDR       6           /* Program header table */
#define PT_LOOS       0x60000000  /* Environment-specific low */
#define PT_HIOS       0x6fffffff  /* Environment-specific high */
#define PT_LOPROC     0x70000000  /* Processor-specific low */
#define PT_HIPROC     0x7fffffff  /* Processor-specific high */
/*
 * These are from linux elf.h, for code usage
 * compatibility.
 */
#define PT_TLS        7
#define PT_GNU_STACK  0x6474e551
#define PT_GNU_EH_FRAME 0x6474e550 /* GCC .eh_frame_hdr segment */
#define PT_GNU_RELRO  0x6474e552   /* Read-only after relocation */

#define PF_X          1
#define PF_W          2
#define PF_R          4

#define SHF_WRITE     0x1        /* Has writable data */
#define SHF_ALLOC     0x2        /* Allocated in memory image of program */
#define SHF_EXECINSTR 0x4        /* Contains executable instructions */
#define SHF_MASKOS    0x0f000000 /* Environment-specific use */
#define SHF_MASKPROC  0xf0000000 /* Processor-specific use */

/* Define 64-bit specific types */
typedef uint64_t  Elf64_Addr;    /* alignment 8 */
typedef uint64_t  Elf64_Off;     /* alignment 8 */
typedef uint16_t  Elf64_Half;    /* alignment 2 */
typedef uint32_t  Elf64_Word;    /* alignment 4 */
typedef int32_t   Elf64_Sword;   /* alignment 4 */
typedef uint64_t  Elf64_Xword;   /* alignment 8 */
typedef int64_t   Elf64_Sxword;  /* alignment 8 */
/* unsigned char, size 1, alignment 1 */

/* Define the structure of the file header for 64 bits. */
typedef struct {
  unsigned char e_ident[EI_NIDENT];
  Elf64_Half    e_type;
  Elf64_Half    e_machine;
  Elf64_Word    e_version;
  Elf64_Addr    e_entry;
  Elf64_Off     e_phoff;
  Elf64_Off     e_shoff;
  Elf64_Word    e_flags;
  Elf64_Half    e_ehsize;
  Elf64_Half    e_phentsize;
  Elf64_Half    e_phnum;
  Elf64_Half    e_shentsize;
  Elf64_Half    e_shnum;
  Elf64_Half    e_shstrndx;
} Elf64_Ehdr;

/* Define the structure of a program header table for 64-bits. */
typedef struct {
  Elf64_Word    p_type;
  Elf64_Word    p_flags;
  Elf64_Off     p_offset;
  Elf64_Addr    p_vaddr;
  Elf64_Addr    p_paddr;
  Elf64_Xword   p_filesz;
  Elf64_Xword   p_memsz;
  Elf64_Xword   p_align;
} Elf64_Phdr;

/* Define 64-bit section headers. */
typedef struct {
  Elf64_Word    sh_name;
  Elf64_Word    sh_type;
  Elf64_Xword   sh_flags;
  Elf64_Addr    sh_addr;
  Elf64_Off     sh_offset;
  Elf64_Xword   sh_size;
  Elf64_Word    sh_link;
  Elf64_Word    sh_info;
  Elf64_Xword   sh_addralign;
  Elf64_Xword   sh_entsize;
} Elf64_Shdr;

/* Define sub architecture neutral types */
typedef Elf64_Addr   Elf_Addr;
typedef Elf64_Off    Elf_Off;
typedef Elf64_Half   Elf_Half;
typedef Elf64_Word   Elf_Word;
typedef Elf64_Sword  Elf_Sword;
typedef Elf64_Xword  Elf_Xword;
typedef Elf64_Sxword Elf_Sxword;

/* Define a neutral form of the file header. */
typedef Elf64_Ehdr Elf_Ehdr;

/* Define a neutral form of a program header. */
typedef Elf64_Phdr Elf_Phdr;
/* taken from zerovm elf.h }} */

/* taken from nacl {{ */
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
/* taken from nacl }} */

#endif /* BOOT_H__ */

