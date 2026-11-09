/* P4.21 C ABI v1. Trusted native code. Guest values are big-endian addresses.
 * No pointers, native handles or mutable C statics belong in guest state.
 * Mutable deterministic state belongs in api->state (256 serialized bytes).
 * See dh_hooks.h and hookable.json for generated names and adapter limits. */
#ifndef DH_MOD_H
#define DH_MOD_H
#include <stdint.h>
#include <stddef.h>
#ifdef _WIN32
#define DH_EXPORT __declspec(dllexport)
#else
#define DH_EXPORT __attribute__((visibility("default")))
#endif
#ifdef __cplusplus
extern "C" {
#endif
#define DH_MOD_ABI 1u
#define DH_MOD_STATE_BYTES 256u
typedef uint32_t dh_guest;
typedef struct dh_api {
    uint32_t abi, size;
    /* Return 0 on invalid span, width, phase or write permission. No MMIO.
     * Pre hooks observe; post hooks run after adapter commit and may patch RAM.
     * Replacement writes remain transactional; fixmul is result-only. */
    int (*read)(dh_guest, uint32_t width, uint32_t *value);
    int (*write)(dh_guest, uint32_t width, uint32_t value);
    void (*overlay)(const char *text); /* one bounded line per mod */
    uint8_t *state;
} dh_api;
typedef struct dh_call {
    uint32_t address, argc, args[16], result;
    uint64_t frame;
} dh_call;
typedef uint32_t (*dh_callback)(const dh_api *, const dh_call *);
enum dh_hook_kind { DH_REPLACE=1, DH_PRE=2, DH_POST=3, DH_FRAME=4,
                    DH_DATA_START=5, DH_DATA_FRAME=6 };
typedef struct dh_hook {
    uint32_t kind;
    const char *target; /* semantic name, cart or runtime hex address */
    dh_callback callback;
    dh_guest address; uint32_t width, value; /* data patch only */
} dh_hook;
typedef struct dh_mod {
    uint32_t abi, size;
    const char *id;
    uint32_t hook_count;
    const dh_hook *hooks;
} dh_mod;
DH_EXPORT const dh_mod *dh_mod_query(void);
#ifdef __cplusplus
}
#endif
#include "dh_hooks.h"
#endif
