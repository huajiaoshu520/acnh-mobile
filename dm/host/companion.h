// Portable boundary between a companion provider and the native guest host.
#pragma once
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {
    uint64_t title_id, main_base, main_size, heap_begin, heap_end;
    char build_id[33];
} dm_companion_guest;
void dm_companion_host_ready(void);
uint64_t dm_companion_title_id(void);
int dm_companion_guest_info(dm_companion_guest *out);
int dm_companion_guest_mapped(uint64_t address,size_t size);
int dm_companion_guest_read(uint64_t address,void *out,size_t size);
typedef struct { uint64_t address;uint32_t size;const void *expect,*value; } dm_companion_write_op;
// At most 32 nonoverlapping, expect-guarded writes of 1..64 bytes to writable heap data.
int dm_companion_guest_write_batch(const dm_companion_write_op *ops,uint32_t count);
void dm_companion_input(uint64_t buttons);
void dm_companion_input_reset(void);
void dm_companion_trace_start(const char *directory);
#ifdef __cplusplus
}
#endif
