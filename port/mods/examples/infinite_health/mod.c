#include "dh_mod.h"
#include <stdio.h>
#include <string.h>
/* Same pointer/join evidence as player_training_tick, deliberately independent
 * of the built-in cheat option. Only the two joined player actors are touched. */
static uint32_t maintain_health(const dh_api *api,const dh_call *c) {
    uint32_t p,count=0;unsigned offset=c->address?0:4;
    memcpy(&count,api->state+offset,4);++count;memcpy(api->state+offset,&count,4);
    for(p=0;p<2;++p) {
        uint32_t joined,task,actor,slot,hp,mode;
        if(!api->read(DH_PLAYER_JOIN_STATES+p,1,&joined)||joined!=2)continue;
        if(!api->read(DH_ACTOR_TASK_SLOTS+4*(p+1),4,&task)||task<0x06004000||task>0x060fffe0||(task&1))continue;
        if(!api->read(task+8,4,&actor)||actor<0x06004000||actor>0x06100000-DH_ACTOR_SIZE||(actor&1))continue;
        if(!api->read(actor+DH_ACTOR_SLOT_ID,1,&slot)||slot!=p+1)continue;
        api->read(0x06099794,2,&hp);api->read(0x0609979a,2,&mode);hp*=mode==1?2:1;
        if(hp && hp<=342) {api->write(actor+DH_ACTOR_HEALTH,2,hp);api->write(0x06099788+2*p,2,hp);}
    }
    (void)c;api->overlay("INFINITE HEALTH - joined players");return 0;
}
static const dh_hook hooks[]={{DH_POST,"player_refresh_ready_mask",maintain_health,0,0,0},{DH_FRAME,0,maintain_health,0,0,0}};
static const dh_mod mod={DH_MOD_ABI,sizeof(dh_mod),"infinite-health",2,hooks};
DH_EXPORT const dh_mod *dh_mod_query(void){return &mod;}
