#include "tusb.h"

// Minimal TinyUSB descriptors for CDC + HID keyboard

// Interface numbers
enum {
    ITF_NUM_CDC = 0,
    ITF_NUM_CDC_DATA,
    ITF_NUM_HID,
    ITF_NUM_TOTAL
};

#define CONFIG_TOTAL_LEN    (TUD_CONFIG_DESC_LEN + TUD_CDC_DESC_LEN + TUD_HID_DESC_LEN)

// Device descriptor
static tusb_desc_device_t const desc_device = {
    .bLength = sizeof(tusb_desc_device_t),
    .bDescriptorType = TUSB_DESC_DEVICE,
    .bcdUSB = 0x0200,
    .bDeviceClass = 0x00,
    .bDeviceSubClass = 0x00,
    .bDeviceProtocol = 0x00,
    .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,
    .idVendor = 0x2E8A,
    .idProduct = 0x000A,
    .bcdDevice = 0x0100,
    .iManufacturer = 0x01,
    .iProduct = 0x02,
    .iSerialNumber = 0x03,
    .bNumConfigurations = 0x01
};

// HID report descriptor (keyboard) - placed before configuration so sizeof() works
const uint8_t hid_keyboard_report_desc[] = {
    TUD_HID_REPORT_DESC_KEYBOARD()
};

// Configuration descriptor: CDC (2 interfaces) + HID keyboard (1 interface)
uint8_t const desc_configuration[] = {
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN, 0x00, 50),
    // CDC: notification EP 0x82, data OUT 0x01, data IN 0x81
    TUD_CDC_DESCRIPTOR(ITF_NUM_CDC, 4, 0x82, 8, 0x01, 0x81, 64),
    // HID keyboard: use interrupt IN 0x83
    TUD_HID_DESCRIPTOR(ITF_NUM_HID, 5, HID_ITF_PROTOCOL_NONE, sizeof(hid_keyboard_report_desc), 0x83, 8, 10)
};

// String descriptors
char const* string_desc_arr[] = {
    (const char[]){0},
    "Waveshare",
    "RP2350 Touch HID",
    "123456",
    "CDC Interface",
    "HID Keyboard"
};

static uint16_t _desc_str[32];

// Device descriptor callback
uint8_t const* tud_descriptor_device_cb(void)
{
    return (uint8_t const*) &desc_device;
}

// Configuration descriptor callback
uint8_t const* tud_descriptor_configuration_cb(uint8_t index)
{
    (void) index;
    return desc_configuration;
}

// String descriptor callback
uint16_t const* tud_descriptor_string_cb(uint8_t index, uint16_t langid)
{
    (void) langid;

    if ( index == 0 ) {
        _desc_str[0] = 0x0409; // supported language: English (0x0409)
        return _desc_str;
    }

    if ( index >= (uint8_t)(sizeof(string_desc_arr)/sizeof(string_desc_arr[0])) ) return NULL;

    const char* str = string_desc_arr[index];
    uint8_t len = 0;
    while (str[len] && len < 31) {
        _desc_str[1+len] = (uint16_t) str[len];
        len++;
    }
    _desc_str[0] = (TUSB_DESC_STRING << 8) | (2*len + 2);
    return _desc_str;
}

// Return pointer to HID report descriptor
uint8_t const* tud_hid_descriptor_report_cb(uint8_t instance)
{
    (void) instance;
    return hid_keyboard_report_desc;
}

// Optional HID callbacks (stubs)
void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize)
{
    (void) instance; (void) report_id; (void) report_type; (void) buffer; (void) bufsize;
}

uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen)
{
    (void) instance; (void) report_id; (void) report_type; (void) buffer; (void) reqlen;
    return 0;
}
