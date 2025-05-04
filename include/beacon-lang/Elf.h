#ifndef BEACON_ELF_H
#define BEACON_ELF_H

#include <stdint.h>

typedef uint64_t beacon_elf64_addr_t;
typedef uint64_t beacon_elf64_off_t;
typedef uint16_t beacon_elf64_half_t;
typedef uint32_t beacon_elf64_word_t;
typedef int32_t beacon_elf64_sword_t;
typedef uint64_t beacon_elf64_xword_t;
typedef int64_t beacon_elf64_xsword_t;

enum {
    BEACON_EI_MAG0 = 0,
    BEACON_EI_MAG1 = 1,
    BEACON_EI_MAG2 = 2,
    BEACON_EI_MAG3 = 3,
    BEACON_EI_CLASS = 4,
    BEACON_EI_DATA = 5,
    BEACON_EI_VERSION = 6,
    BEACON_EI_OSABI = 7,
    BEACON_EI_ABIVERSION = 8,
    BEACON_EI_PAD = 9,
    BEACON_EI_NIDENT = 16,
};

enum {
    BEACON_ELFCLASS32 = 1,
    BEACON_ELFCLASS64 = 2,
};

enum {
    BEACON_ELFDATA2LSB = 1,
    BEACON_ELFDATA2MSB = 2,
};

enum {
    BEACON_ELFCURRENT_VERSION = 1
};

enum {
    BEACON_EM_X86_64 = 62
};

enum {
    BEACON_ET_NONE = 0,
    BEACON_ET_REL = 1,
    BEACON_ET_EXEC = 2,
    BEACON_ET_DYN = 3,
    BEACON_ET_CORE = 4,
};

enum {
    BEACON_SHF_WRITE = 1,
    BEACON_SHF_ALLOC = 2,
    BEACON_SHF_EXECINSTR = 4,
};

enum {
    BEACON_SHN_UNDEF = 0,
    BEACON_SHN_ABS = 0xFFF1,
    BEACON_SHN_COMMON = 0xFFF2,
};

enum {
    BEACON_SHT_NULL = 0,
    BEACON_SHT_PROGBITS = 1,
    BEACON_SHT_SYMTAB = 2,
    BEACON_SHT_STRTAB = 3,
    BEACON_SHT_RELA = 4,
    BEACON_SHT_HASH = 5,
    BEACON_SHT_DYNAMIC = 6,
    BEACON_SHT_NOTE = 7,
    BEACON_SHT_NOBITS = 8,
    BEACON_SHT_REL = 9,
    BEACON_SHT_SHLIB = 10,
    BEACON_SHT_DYNSYM = 11,

    SHT_X86_64_UNWIND = 0x70000001,
};

enum {
    BEACON_STB_LOCAL = 0,
    BEACON_STB_GLOBAL = 1,
    BEACON_STB_WEAK = 2,
};

enum {
    BEACON_STT_NOTYPE = 0,
    BEACON_STT_OBJECT = 1,
    BEACON_STT_FUNC = 2,
    BEACON_STT_SECTION = 3,
    BEACON_STT_FILE = 4,
};

#define BEACON_ELF64_SYM_INFO(type, binding) (((binding) << 4) | (type))

typedef struct beacon_elf64_header_s
{
    uint8_t ident[16];
    beacon_elf64_half_t type;
    beacon_elf64_half_t machine;
    beacon_elf64_word_t version;
    beacon_elf64_addr_t entry;
    beacon_elf64_off_t programHeadersOffset;
    beacon_elf64_off_t sectionHeadersOffset;
    beacon_elf64_word_t flags;
    beacon_elf64_half_t elfHeaderSize;
    beacon_elf64_half_t programHeaderEntrySize;
    beacon_elf64_half_t programHeaderCount;
    beacon_elf64_half_t sectionHeaderEntrySize;
    beacon_elf64_half_t sectionHeaderNum;
    beacon_elf64_half_t sectionHeaderNameStringTableIndex;
} beacon_elf64_header_t;

typedef struct beacon_elf64_sectionHeader_s
{
    beacon_elf64_word_t name;
    beacon_elf64_word_t type;
    beacon_elf64_xword_t flags;
    beacon_elf64_addr_t address;
    beacon_elf64_off_t offset;
    beacon_elf64_xword_t size;
    beacon_elf64_word_t link;
    beacon_elf64_word_t info;
    beacon_elf64_xword_t addressAlignment;
    beacon_elf64_xword_t entrySize;
} beacon_elf64_sectionHeader_t;

typedef struct beacon_elf64_symbol_s
{
    beacon_elf64_word_t name;
    uint8_t info;
    uint8_t other;
    beacon_elf64_half_t sectionHeaderIndex;
    beacon_elf64_addr_t value;
    beacon_elf64_xword_t size;
} beacon_elf64_symbol_t;

#endif //BEACON_ELF_H
