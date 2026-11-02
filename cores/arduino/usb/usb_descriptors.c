#include <stdint.h>
#include <stdbool.h>
#include "tusb.h"
#include "usb_modules.h"

#define USB_VID 0xCafe
#define USB_PID 0x4001

//--------------------------------------------------------------------+
// Device descriptor
//--------------------------------------------------------------------+
static tusb_desc_device_t const desc_device = {
    .bLength            = sizeof(tusb_desc_device_t),
    .bDescriptorType    = TUSB_DESC_DEVICE,
    .bcdUSB             = 0x0200,
    .bDeviceClass       = TUSB_CLASS_MISC,
    .bDeviceSubClass    = MISC_SUBCLASS_COMMON,
    .bDeviceProtocol    = MISC_PROTOCOL_IAD,
    .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,
    .idVendor           = USB_VID,
    .idProduct          = USB_PID,
    .bcdDevice          = 0x0100,
    .iManufacturer      = 1,
    .iProduct           = 2,
    .iSerialNumber      = 3,
    .bNumConfigurations = 1
};

uint8_t const *tud_descriptor_device_cb(void) {
    return (uint8_t const *) &desc_device;
}

//--------------------------------------------------------------------+
// Configuration + HID report descriptors, built at runtime from
// whichever optional modules (Keyboard / Mouse) were linked in.
//--------------------------------------------------------------------+
enum { REPORT_ID_KEYBOARD = 1, REPORT_ID_MOUSE = 2 };

static uint8_t  hid_report[128];
static uint16_t hid_report_len = 0;
static uint8_t  cfg_desc[128];
static bool     built = false;

static void put(uint8_t **p, const uint8_t *src, uint16_t n) { while (n--) *(*p)++ = *src++; }

static void build_descriptors(void) {
    static const uint8_t kbd[]   = { TUD_HID_REPORT_DESC_KEYBOARD(HID_REPORT_ID(REPORT_ID_KEYBOARD)) };
    static const uint8_t mouse[] = { TUD_HID_REPORT_DESC_MOUSE(HID_REPORT_ID(REPORT_ID_MOUSE)) };

    uint8_t *r = hid_report;
    if (USB_HAS_KEYBOARD) put(&r, kbd, sizeof kbd);
    if (USB_HAS_MOUSE)    put(&r, mouse, sizeof mouse);
    hid_report_len = (uint16_t)(r - hid_report);

    bool hid = hid_report_len > 0;
    uint8_t itf_total = 2 + (hid ? 1 : 0);

    uint8_t *p = cfg_desc;
    uint8_t *start = p;

    // Config header with placeholder length (we patch it at the end)
    {
        const uint8_t h[] = { TUD_CONFIG_DESCRIPTOR(1, itf_total, 0, 0 /* placeholder */, 0x00, 100) };
        put(&p, h, sizeof h);
    }

    // CDC
    { 
        const uint8_t c[] = { TUD_CDC_DESCRIPTOR(0, 4, 0x81, 8, 0x02, 0x82, 64) }; 
        put(&p, c, sizeof c); 
    }

    // HID (use 0x83)
    if (hid) {
        const uint8_t d[] = { TUD_HID_DESCRIPTOR(2, 5, HID_ITF_PROTOCOL_NONE, hid_report_len, 0x83, 8, 1) };
        put(&p, d, sizeof d);
    }

    // Patch the real total length into the config descriptor
    uint16_t total = (uint16_t)(p - start);
    cfg_desc[2] = (uint8_t)(total & 0xFF);
    cfg_desc[3] = (uint8_t)(total >> 8);

    built = true;
}

uint8_t const *tud_descriptor_configuration_cb(uint8_t index) {
    (void) index;
    if (!built) build_descriptors();
    return cfg_desc;
}

uint8_t const *tud_hid_descriptor_report_cb(uint8_t instance) {
    (void) instance;
    if (!built) build_descriptors();
    return hid_report;
}

//--------------------------------------------------------------------+
// HID callbacks TinyUSB requires whenever the HID class is compiled in
//--------------------------------------------------------------------+
uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type,
                               uint8_t *buffer, uint16_t reqlen) {
    (void) instance; (void) report_id; (void) report_type; (void) buffer; (void) reqlen;
    return 0;
}

void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type,
                           uint8_t const *buffer, uint16_t bufsize) {
    (void) instance; (void) report_id; (void) report_type; (void) buffer; (void) bufsize;
}

//--------------------------------------------------------------------+
// String descriptors
//--------------------------------------------------------------------+
static char const *string_desc[] = { "", "PiZero", "PiZero CDC", "000001", "CDC", "PiZero HID" };
static uint16_t desc_str_buf[32];

uint16_t const *tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
    (void) langid;
    uint8_t n;
    if (index == 0) {
        desc_str_buf[1] = 0x0409;           // English (US)
        n = 1;
    } else {
        if (index >= sizeof(string_desc) / sizeof(string_desc[0])) return 0;
        char const *s = string_desc[index];
        for (n = 0; s[n] && n < 30; n++) desc_str_buf[1 + n] = s[n];
    }
    desc_str_buf[0] = (uint16_t) ((TUSB_DESC_STRING << 8) | (2 * n + 2));
    return desc_str_buf;
}
