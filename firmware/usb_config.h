#ifndef _USB_CONFIG_H
#define _USB_CONFIG_H

// Defines the number of endpoints for this device. (Always add one for EP0). For two EPs, this should be 3.
#define ENDPOINTS 2

#define USB_PORT    D // [A,C,D] GPIO Port to use with D+, D- and DPU
#define USB_PIN_DP  4 // [0-4] GPIO Number for USB D+ Pin
#define USB_PIN_DM  3 // [0-4] GPIO Number for USB D- Pin
#define USB_PIN_DPU 2 // [0-7] GPIO for feeding the 1.5k Pull-Up on USB D- Pin; Comment out if not used / tied to 3V3!

#define RV003USB_DEBUG_TIMING      0
#define RV003USB_OPTIMIZE_FLASH    1
#define RV003USB_EVENT_DEBUGGING   0
#define RV003USB_HANDLE_IN_REQUEST 1
#define RV003USB_OTHER_CONTROL     1
#define RV003USB_HANDLE_USER_DATA  1
#define RV003USB_HID_FEATURES      1

#ifndef __ASSEMBLER__

#include "DAP_config.h"
#include <tinyusb_hid.h>

#ifdef INSTANCE_DESCRIPTORS

// Taken from http://www.usbmadesimple.co.uk/ums_ms_desc_dev.htm
static const uint8_t device_descriptor[] = {
    18,         // Length
    1,          // Type (Device)
    0x00, 0x02, // bcdUSB
    0x0,        // Device Class
    0x0,        // Device Subclass
    0x0,        // Device Protocol  (000 = use config descriptor)
    0x08,       // Max packet size for EP0 (This has to be 8 because of the USB Low-Speed Standard)

    0x09, 0x12, // ID Vendor
    0x03, 0xd0, // ID Product
    0x10, 0x01, // bcdDevice

    1, // Manufacturer string
    2, // Product string
    3, // Serial string
    1, // Max number of configurations
};

static const uint8_t special_hid_desc[] = {
   HID_USAGE_PAGE ( 0xff ), // Usage Page = 0xFF (Vendor Defined Page 1)
	HID_USAGE      ( 0x01 ),   // Usage (Vendor Usage 1)
	HID_COLLECTION ( HID_COLLECTION_APPLICATION ),
		HID_USAGE_MIN    ( 0x01 ),
		HID_USAGE_MAX    ( 0x40 ),  // 64 input usages total (0x01 to 0x40)
		HID_LOGICAL_MIN  ( 0x00 ),  // Logical Minimum (data bytes may have minimum value = 0x00)
		HID_LOGICAL_MAX  ( 0xff ),  // Logical Maximum (data bytes may have maximum value = 0x00FF = unsigned 255)
		HID_REPORT_SIZE  ( 8 ),     // Report Size: 8-bit field size
		HID_REPORT_COUNT ( 0x40 ),  // Report Count: Make sixty-four 8-bit fields
		HID_INPUT        ( HID_DATA | HID_ARRAY | HID_ABSOLUTE ),
		HID_USAGE_MIN    ( 0x01 ),
		HID_USAGE_MAX    ( 0x40 ),  // 64 output usages total (0x01 to 0x40)
		HID_OUTPUT       ( HID_DATA | HID_ARRAY | HID_ABSOLUTE ),
	HID_COLLECTION_END,
};

static const uint8_t config_descriptor[] = {
    0x09,       // bLength
    0x02,       // bDescriptorType
 
   0x29, 0x00, // wTotalLength
    1,          // bNumInterfaces
    0x01,       // gurationValue
    0x00,       // iConfiguration
    0x00,       // bmAttributes, D6: self power  D5: remote wake-up
    0x64,       // MaxPower, 100 * 2mA = 200mA

    // I/F descriptor: HID
    0x09, // bLength
    0x04, // bDescriptorType
    0x00, // bInterfaceNumber
    0x00, // bAlternateSetting
    0x02, // bNumEndpoints
    0x03, // bInterfaceClass
    0x00, // bInterfaceSubClass
    0x00, // bInterfaceProtocol
    0x00, // iInterface

    // HID Descriptor
    0x09,       // bLength
    0x21,       // bDescriptorType
    0x10, 0x01, // HID Class Spec
    0x00,       // H/W target country.
    0x01,       // Number of HID class descriptors to follow.
    0x22,       // Descriptor type.
    sizeof(special_hid_desc), 0x00,   // Total length of report descriptor.

    // EP Descriptor: interrupt out.
    0x07,       // bLength
    0x05,       // bDescriptorType
    0x01,       // bEndpointAddress
    0x03,       // bmAttributes (INTERRUPT)
    0x08, 0x00, // wMaxPacketSize
    1,          // bInterval

    // EP Descriptor: interrupt in.
    0x07,       // bLength
    0x05,       // bDescriptorType
    0x81,       // bEndpointAddress
    0x03,       // bmAttributes (INTERRUPT)
    0x08, 0x00, // wMaxPacketSize
    1,          // bInterval
};

#define STR_MANUFACTURER u"CNLohr"
#define STR_PRODUCT      u"RV003 CMSIS-DAP V1.3"
#define STR_SERIAL       u"1234"

struct usb_string_descriptor_struct
{
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint16_t wString[];
};
const static struct usb_string_descriptor_struct string0 __attribute__((section(".rodata"))) = {
    4,
    3,
    {0x0409}};
const static struct usb_string_descriptor_struct string1 __attribute__((section(".rodata"))) = {
    sizeof(STR_MANUFACTURER),
    3,
    STR_MANUFACTURER};
const static struct usb_string_descriptor_struct string2 __attribute__((section(".rodata"))) = {
    sizeof(STR_PRODUCT),
    3,
    STR_PRODUCT};
const static struct usb_string_descriptor_struct string3 __attribute__((section(".rodata"))) = {
    sizeof(STR_SERIAL),
    3,
    STR_SERIAL};

// This table defines which descriptor data is sent for each specific
// request from the host (in wValue and wIndex).
const static struct descriptor_list_struct
{
    uint32_t lIndexValue;
    const uint8_t *addr;
    uint8_t length;
} descriptor_list[] = {
    {0x00000100, device_descriptor, sizeof(device_descriptor)},
    {0x00000200, config_descriptor, sizeof(config_descriptor)},
    // interface number // 2200 for hid descriptors.
    {0x00002200, special_hid_desc, sizeof(special_hid_desc)},
    {0x00002100, config_descriptor + 18, 9}, // Not sure why, this seems to be useful for Windows + Android.

    {0x00000300, (const uint8_t *)&string0, 4},
    {0x04090301, (const uint8_t *)&string1, sizeof(STR_MANUFACTURER)},
    {0x04090302, (const uint8_t *)&string2, sizeof(STR_PRODUCT)},
    {0x04090303, (const uint8_t *)&string3, sizeof(STR_SERIAL)}};
#define DESCRIPTOR_LIST_ENTRIES ((sizeof(descriptor_list)) / (sizeof(struct descriptor_list_struct)))

#endif // INSTANCE_DESCRIPTORS

#endif

#endif
