#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
// Every field is mapped into an existing serialized SH2Context register.
struct ConvergeRoot { uint32_t input, root, remainder, iterations, top; };
uint32_t converge_root(void*, struct ConvergeRoot*);
int32_t converge_slot(void*, uint32_t);
void converge_root_checkpoint(void*,const struct ConvergeRoot*);
int converge_slot_read(void*,uint32_t);
void converge_slot_checkpoint(void*,uint32_t);
int32_t converge_slot_finish(void*,uint32_t);
#ifdef __cplusplus
}
#endif
