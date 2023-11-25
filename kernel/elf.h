// Format of an ELF executable file

#define ELF_MAGIC 0x464C457FU  // "\x7FELF" in little endian

// File header
struct elfhdr {
  UInt32 magic;  // must equal ELF_MAGIC
  UChar elf[12];
  UInt16 type;
  UInt16 machine;
  UInt32 version;
  UInt64 entry;
  UInt64 phoff;
  UInt64 shoff;
  UInt32 flags;
  UInt16 ehsize;
  UInt16 phentsize;
  UInt16 phnum;
  UInt16 shentsize;
  UInt16 shnum;
  UInt16 shstrndx;
};

// Program section header
struct proghdr {
  UInt32 type;
  UInt32 flags;
  UInt64 off;
  UInt64 vaddr;
  UInt64 paddr;
  UInt64 filesz;
  UInt64 memsz;
  UInt64 align;
};

// Values for Proghdr type
#define ELF_PROG_LOAD           1

// Flag bits for Proghdr flags
#define ELF_PROG_FLAG_EXEC      1
#define ELF_PROG_FLAG_WRITE     2
#define ELF_PROG_FLAG_READ      4
