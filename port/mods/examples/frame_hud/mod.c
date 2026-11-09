#include "dh_mod.h"
#include <stdio.h>
#include <string.h>
/* Counter uses the SDK's serialized bytes so rollback/rewind restore it. */
static uint32_t show_frame(const dh_api *api,const dh_call *c) {
    uint32_t scene=0,count=0;char text[128];
    memcpy(&count,api->state,sizeof count);++count;memcpy(api->state,&count,sizeof count);
    api->read(DH_SCENE_INDEX,4,&scene);
    snprintf(text,sizeof text,"FRAME %llu | SCENE %u | HUD TICKS %u",(unsigned long long)c->frame,scene,count);
    api->overlay(text);return 0;
}
static const dh_hook hooks[]={{DH_FRAME,0,show_frame,0,0,0}};
static const dh_mod mod={DH_MOD_ABI,sizeof(dh_mod),"frame-hud",1,hooks};
DH_EXPORT const dh_mod *dh_mod_query(void){return &mod;}
