/* SPDX-License-Identifier: MIT */
#pragma once
#include <stdint.h>
#include <string.h>
enum { DEVICE_BYTES=18, CFG_BYTES=9, GAME_BYTES=40, WEB_BYTES=25, REPORT_BYTES=25 };
struct usb_layout {
    uint8_t blob[384], compat[16+4*24];
    uint16_t length;
    uint8_t web_offset, compat_length;
};
static inline unsigned usb_string(uint8_t *p,const char *text)
{
    unsigned n=strlen(text);
    p[0]=2+2*n; p[1]=3;
    for(unsigned i=0;i<n;i++) p[2+i*2]=text[i];
    return p[0];
}
/* Continuous Chapter 9 descriptors, then strings and the Zephyr end marker. */
static inline void make_layout(struct usb_layout *l,unsigned slots,uint8_t pid,const char *name)
{
    memset(l,0,sizeof(*l));
    uint8_t descriptor[]={18,1,0,2,0,0,0,64,9,0x12,pid,0,0x10,1,1,2,0,1,
        9,2,CFG_BYTES+slots*GAME_BYTES+WEB_BYTES,0,slots+1,1,0,0x80,50};
    memcpy(l->blob,descriptor,sizeof(descriptor));
    unsigned offset=sizeof(descriptor);
    l->compat_length=16+24*slots;
    l->compat[0]=l->compat_length; l->compat[5]=1; l->compat[6]=4; l->compat[8]=slots;
    for(unsigned i=0;i<slots;i++) {
        uint8_t in=0x81+i,out=2+i;
        uint8_t game[]={9,4,i,0,2,0xff,0x5d,1,0,
            17,0x21,0,1,1,0x25,in,20,0,0,0,0,0x13,out,8,0,0,
            7,5,in,3,32,0,1,7,5,out,3,32,0,8};
        memcpy(l->blob+offset,game,sizeof(game)); offset+=sizeof(game);
        uint8_t *entry=l->compat+16+i*24;
        entry[0]=i; entry[1]=1; memcpy(entry+2,"XUSB10",6);
    }
    l->web_offset=offset;
    uint8_t web[]={9,4,slots,0,1,3,0,0,0,9,0x21,0x11,1,0,1,0x22,REPORT_BYTES,0,
                   7,5,0x85,3,64,0,8};
    memcpy(l->blob+offset,web,sizeof(web)); offset+=sizeof(web);
    const uint8_t language[]={4,3,9,4};
    memcpy(l->blob+offset,language,4);offset+=4;
    offset+=usb_string(l->blob+offset,"SkyWithClouds");
    offset+=usb_string(l->blob+offset,name);
    l->length=offset+2;
}
