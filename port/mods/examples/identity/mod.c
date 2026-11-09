#include "dh_mod.h"
/* This C algorithm supplies r0. The pinned convergence recipe supplies the
 * original full MACH/MACL and r1; all other registers remain untouched. */
static uint32_t fixed_mul16(const dh_api *api,const dh_call *c) {
    (void)api;
    uint64_t bits=(uint64_t)((int64_t)(int32_t)c->args[0]*(int32_t)c->args[1]);
    return (uint32_t)(bits>>16);
}
static const dh_hook hooks[]={{DH_REPLACE,"fixed_mul16",fixed_mul16,0,0,0}};
static const dh_mod mod={DH_MOD_ABI,sizeof(dh_mod),"identity",1,hooks};
DH_EXPORT const dh_mod *dh_mod_query(void){return &mod;}
