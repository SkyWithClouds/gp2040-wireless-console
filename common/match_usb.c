/* SPDX-License-Identifier: MIT */
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/usb/usb_device.h>
#include "match_usb.h"
#include "layout.h"
#ifdef CONFIG_USB_DEVICE_OS_DESC
#include <os_desc.h>
#endif
enum {HID_BYTES=64};
static unsigned game_count;
static struct usb_layout layout;
static atomic_t epoch, configured, suspended, busy[4], done[4], lamp[4], live_busy;
static uint8_t game_dma[4][20] __aligned(4), live_dma[64] __aligned(4);
static const uint8_t hid_report[] = {
    0x06,0x60,0xff, 0x09,0x01, 0xa1,0x01, /* vendor page FF60, application */
    0x15,0x00, 0x26,0xff,0x00, 0x75,0x08, 0x95,HID_BYTES,
    0x09,0x02, 0x81,0x02, 0x09,0x03, 0xb1,0x02, 0xc0
};
static struct k_spinlock lock;
static uint8_t last_feature[HID_BYTES], control_copy[HID_BYTES], idle_rate;
struct queued_command { atomic_val_t epoch; uint8_t data[HID_BYTES]; };
K_MSGQ_DEFINE(commands,sizeof(struct queued_command),4,4);

static int hid_descriptor(struct usb_setup_packet *s,int32_t *len,uint8_t **data)
{
    if (s->bmRequestType!=0x81 || s->bRequest!=6 || s->wIndex!=game_count) return -EINVAL;
    unsigned size;
    if (s->wValue==0x2200) { *data=(uint8_t *)hid_report; size=sizeof(hid_report); }
    else if (s->wValue==0x2100) { *data=(layout.blob+layout.web_offset+9); size=9; }
    else return -EINVAL;
    *len=MIN(s->wLength,size);
    return 0;
}
static int hid_control(struct usb_setup_packet *s,int32_t *len,uint8_t **data)
{
    if (s->wIndex!=game_count) return -ENOTSUP;
    if (s->bmRequestType==0xa1 && s->bRequest==1 && s->wValue==0x0300) {
        k_spinlock_key_t key=k_spin_lock(&lock);
        memcpy(control_copy,last_feature,HID_BYTES);
        k_spin_unlock(&lock,key);
        if (!memcmp(control_copy,"HX\1",3) && control_copy[3]==MATCH_BOOTLOADER && !control_copy[6]) match_boot_request();
        *data=control_copy; *len=MIN(s->wLength,HID_BYTES);
        return 0;
    }
    if (s->bmRequestType==0x21 && s->bRequest==9 && s->wValue==0x0300 &&
        s->wLength==HID_BYTES && *len==HID_BYTES) {
        struct queued_command cmd={.epoch=atomic_get(&epoch)};
        memcpy(cmd.data,*data,HID_BYTES);
        return k_msgq_put(&commands,&cmd,K_NO_WAIT);
    }
    if (s->bmRequestType==0x21 && s->bRequest==10) { idle_rate=s->wValue>>8; return 0; }
    if (s->bmRequestType==0xa1 && s->bRequest==2) { *len=1; *data=&idle_rate; return 0; }
    return -ENOTSUP;
}
_Static_assert(sizeof(hid_report)==REPORT_BYTES,"report descriptor length");
static void usb_status(enum usb_dc_status_code status,const uint8_t *param)
{
    switch(status) {
    case USB_DC_CONFIGURED: case USB_DC_RESET: case USB_DC_DISCONNECTED:
        atomic_set(&configured,status==USB_DC_CONFIGURED && param && *param);
        atomic_clear(&suspended); atomic_clear(&live_busy);
        for(unsigned i=0;i<game_count;i++) { atomic_clear(&busy[i]);atomic_clear(&lamp[i]); }
        atomic_inc(&epoch);
        break;
    case USB_DC_SUSPEND: atomic_set(&suspended,1);break;
    case USB_DC_RESUME: atomic_clear(&suspended);break;
    default: break;
    }
}
static void on_in(uint8_t ep,enum usb_dc_ep_cb_status_code status)
{
    if(status!=USB_DC_EP_DATA_IN) return;
    if(ep==0x85) {atomic_clear(&live_busy);return;}
    unsigned i=ep-0x81;
    if(i<game_count) {atomic_inc(&done[i]);atomic_clear(&busy[i]);}
}
static void on_out(uint8_t ep,enum usb_dc_ep_cb_status_code status)
{
    unsigned i=ep-2;
    uint8_t p[32]; uint32_t n;
    if(i<game_count && status==USB_DC_EP_DATA_OUT && !usb_read(ep,p,sizeof(p),&n) &&
       n>=3 && p[0]==1 && p[1]==3) {
        uint16_t gen=(atomic_get(&lamp[i])>>8)+1;
        atomic_set(&lamp[i],BIT(24)|((uint32_t)gen<<8)|p[2]);
    }
}
static struct usb_ep_cfg_data endpoints[9];
USBD_DEFINE_CFG_DATA(management) = {
    .interface={.custom_handler=hid_descriptor,.class_handler=hid_control},.endpoint=endpoints
};
int match_usb_open(unsigned slots)
{
    if(slots>4) return -EINVAL;
    game_count=slots;
    uint8_t pid=slots?3+slots:CONFIG_USB_DEVICE_PID;
    make_layout(&layout,slots,pid,CONFIG_USB_DEVICE_PRODUCT);
    management.interface_descriptor=layout.blob+layout.web_offset;
    for(unsigned i=0;i<slots;i++) {
        endpoints[2*i]=(struct usb_ep_cfg_data){.ep_cb=on_in,.ep_addr=0x81+i};
        endpoints[2*i+1]=(struct usb_ep_cfg_data){.ep_cb=on_out,.ep_addr=2+i};
    }
    endpoints[2*slots]=(struct usb_ep_cfg_data){.ep_cb=on_in,.ep_addr=0x85};
    management.num_endpoints=2*slots+1;
#ifdef CONFIG_USB_DEVICE_OS_DESC
    static uint8_t ms[]={18,3,'M',0,'S',0,'F',0,'T',0,'1',0,'0',0,'0',0,0x20,0};
    static struct usb_os_descriptor os={.string=ms,.string_len=18,.vendor_code=0x20};
    os.compat_id=layout.compat;os.compat_id_len=layout.compat_length;
    usb_register_os_desc(&os);
#endif
    usb_set_config(layout.blob);
    return usb_enable(usb_status);
}
unsigned match_usb_epoch(void) { return atomic_get(&epoch); }
unsigned match_usb_completions(unsigned i) { return atomic_get(&done[i]); }
uint32_t match_usb_lamp(unsigned i) { return atomic_get(&lamp[i]); }
bool match_usb_ready(void) { return atomic_get(&configured) && !atomic_get(&suspended); }
bool match_usb_submit(unsigned i,const struct gp_input *s)
{
    if(!match_usb_ready() || !atomic_cas(&busy[i],0,1)) return false;
    uint8_t *p=game_dma[i];p[1]=20;
    sys_put_le16(s->buttons,p+2);p[4]=s->lt;p[5]=s->rt;
    sys_put_le16(s->lx,p+6);sys_put_le16(s->ly,p+8);
    sys_put_le16(s->rx,p+10);sys_put_le16(s->ry,p+12);
    uint32_t n;
    if(!usb_write(0x81+i,p,20,&n) && n==20) return true;
    atomic_clear(&busy[i]);return false;
}
void match_usb_service(void)
{
    match_service();
    if(!match_usb_ready()) return;
    struct queued_command cmd;
    if(!k_msgq_get(&commands,&cmd,K_NO_WAIT) && cmd.epoch==atomic_get(&epoch)) {
        uint8_t reply[64]={0};
        if(!match_command(cmd.data,reply)) {memcpy(reply,cmd.data,6);reply[6]=1;}
        k_spinlock_key_t key=k_spin_lock(&lock);
        memcpy(last_feature,reply,64); k_spin_unlock(&lock,key);
    }
    static uint32_t last;
    uint32_t now=k_uptime_get_32(),n;
    if(!game_count || now-last<150 || !atomic_cas(&live_busy,0,1)) return;
    last=now;
    memcpy(live_dma,"RS\1",3);live_dma[3]=game_count;
    for(unsigned i=0;i<game_count;i++) {
        uint8_t *p=live_dma+4+i*15;
        const struct rf_stats *s=&match_stats[i];
        p[0]=s->online;sys_put_le32(s->frames,p+1);sys_put_le32(s->gaps,p+5);
        sys_put_le32(s->duplicates,p+9);sys_put_le16(MIN(s->timeouts,65535),p+13);
    }
    if(usb_write(0x85,live_dma,64,&n) || n!=64) atomic_clear(&live_busy);
}
