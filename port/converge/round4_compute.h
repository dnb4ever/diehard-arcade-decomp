#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
struct SortState {
    uint32_t cursor, bucket, remaining, source, node, destination;
    uint32_t digit, key, entity, head, next, phase, branch, base;
};
struct MatrixState {
    uint32_t matrix, output, vector, remaining, offset, result, mode;
    uint64_t accumulator;
};
uint32_t compute_read(uint32_t);
void compute_write(uint32_t,uint32_t);
void sort_boundary(void*,const struct SortState*,unsigned);
void matrix_boundary(void*,const struct MatrixState*,unsigned);
void converge_sort(void*,struct SortState*);
void converge_translate(void*,struct MatrixState*);
#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
extern "C" {
#endif
struct CopyState {uint32_t source,destination,remaining,value;};
void copy_boundary(void*,const struct CopyState*);
void converge_copy(void*,struct CopyState*);
#ifdef __cplusplus
}
#endif
