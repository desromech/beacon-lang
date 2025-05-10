#ifndef BEACON_GDB_H
#define BEACON_GDB_H

#include <stdint.h>
#include <stddef.h>

typedef enum
{
  BEACON_GDB_JIT_NOACTION = 0,
  BEACON_GDB_JIT_REGISTER_FN,
  BEACON_GDB_JIT_UNREGISTER_FN
} beacon_gdb_jit_actions_t;

typedef struct beacon_gdb_jit_code_entry_s
{
  struct beacon_gdb_jit_code_entry_s *next_entry;
  struct beacon_gdb_jit_code_entry_s *prev_entry;
  const char *symfile_addr;
  uint64_t symfile_size;
} beacon_gdb_jit_code_entry_t;

typedef struct beacon_gdb_jit_descriptor_s
{
  uint32_t version;
  /* This type should be jit_actions_t, but we use uint32_t
     to be explicit about the bitwidth.  */
  uint32_t action_flag;
  beacon_gdb_jit_code_entry_t *relevant_entry;
  beacon_gdb_jit_code_entry_t *first_entry;
} beacon_gdb_jit_descriptor_t;

void beacon_gdb_registerObjectFile(beacon_gdb_jit_code_entry_t *entry, const void *objectFileAddress, size_t objectFileSize);
void beacon_gdb_unregisterObjectFile(beacon_gdb_jit_code_entry_t *entry);

#endif //BEACON_GDB_H
