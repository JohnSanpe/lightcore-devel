/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef _MOD_DEVTABLE_H_
#define _MOD_DEVTABLE_H_

#include <types.h>
#include <bits.h>

#define ACPI_ID_LEN	9

struct acpi_device_id {
    char id[ACPI_ID_LEN]; 
};

#define PLATFORM_NAME_LEN 32

struct platform_device_id {
    char        name[PLATFORM_NAME_LEN];
    const void *data;
};

#define DT_NAME_LEN 32

struct dt_device_id {
    char        name[32];
    char        type[32];
    char        compatible[128];
};

#define PCI_ANY_ID  (~0)

struct pci_device_id {
    uint32_t vendor,    device;
    uint32_t subvendor, subdevice;
    uint32_t class,     class_mask;
    int      driver_data;
};

#define PCI_DEVICE(Vendor, Device) \
    .vendor = (Vendor), .device = (Device), \
    .subvendor = (PCI_ANY_ID), .subdevice = (PCI_ANY_ID)

#define PCI_DEVICE_CLASS(Class, Class_mask) \
    .class = (Class), .class_mask = (Class_mask),   \
    .vendor = (PCI_ANY_ID), .device = (PCI_ANY_ID), \
    .subvendor = (PCI_ANY_ID), .subdevice = (PCI_ANY_ID)
    
enum usb_match_mode {
    USB_DEV_MATCH_VENDOR        = 0x0001,
    USB_DEV_MATCH_PRODUCT       = 0x0002,

    USB_DEV_MATCH_DEV_LO        = 0x0004,
    USB_DEV_MATCH_DEV_HI        = 0x0008,
    USB_DEV_MATCH_DEV_CLASS     = 0x0010,
    USB_DEV_MATCH_DEV_SUBCLASS  = 0x0020,
    USB_DEV_MATCH_DEV_PROTOCOL  = 0x0040,

    USB_DEV_MATCH_INT_CLASS     = 0x0080,
    USB_DEV_MATCH_INT_SUBCLASS  = 0x0100,
    USB_DEV_MATCH_INT_PROTOCOL  = 0x0200,
    USB_DEV_MATCH_INT_NUMBER    = 0x0400,
};

struct usb_device_id {
    uint16_t idVendor;
    uint16_t idProduct;

    uint8_t bDeviceClass;
    uint8_t bDeviceSubClass;
    uint8_t bDeviceProtocol;
    uint16_t bcdDevice_lo;
    uint16_t bcdDevice_hi;

    uint8_t bInterfaceClass;
    uint8_t bInterfaceSubClass;
    uint8_t bInterfaceProtocol;
    uint8_t bInterfaceNumber;

    int driver_data;
    enum usb_match_mode match_mode;
};

#define USB_DEVICE(Vendor, Product) \
    .match_mode = USB_DEV_MATCH_VENDOR | USB_DEV_MATCH_PRODUCT, \
    .idVendor = (Vendor), .idProduct = (Product)

#define USB_DEVICE_CLASS(Class) \
    .match_mode = USB_DEV_MATCH_DEV_CLASS, .bDeviceClass = (Class)

#define USB_INT_CLASS(Class) \
    .match_mode = USB_DEV_MATCH_INT_CLASS, .bInterfaceClass = (Class)

#endif /* _MOD_DEVTABLE_H_ */
