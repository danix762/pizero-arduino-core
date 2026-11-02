#ifndef DWC2_H
#define DWC2_H

#include <stdint.h>

// BCM2835 USB (DWC2) base address
#define USB_BASE            0x20980000UL

// Core Global Registers
#define GOTGCTL             (*(volatile uint32_t *)(USB_BASE + 0x000))
#define GOTGINT             (*(volatile uint32_t *)(USB_BASE + 0x004))
#define GAHBCFG             (*(volatile uint32_t *)(USB_BASE + 0x008))
#define GUSBCFG             (*(volatile uint32_t *)(USB_BASE + 0x00C))
#define GRSTCTL             (*(volatile uint32_t *)(USB_BASE + 0x010))
#define GINTSTS             (*(volatile uint32_t *)(USB_BASE + 0x014))
#define GINTMSK             (*(volatile uint32_t *)(USB_BASE + 0x018))
#define GRXSTSR             (*(volatile uint32_t *)(USB_BASE + 0x01C))
#define GRXSTSP             (*(volatile uint32_t *)(USB_BASE + 0x020))
#define GRXFSIZ             (*(volatile uint32_t *)(USB_BASE + 0x024))
#define GNPTXFSIZ           (*(volatile uint32_t *)(USB_BASE + 0x028))
#define GSNPSID             (*(volatile uint32_t *)(USB_BASE + 0x040))
#define GHWCFG1             (*(volatile uint32_t *)(USB_BASE + 0x044))
#define GHWCFG2             (*(volatile uint32_t *)(USB_BASE + 0x048))
#define GHWCFG3             (*(volatile uint32_t *)(USB_BASE + 0x04C))
#define GHWCFG4             (*(volatile uint32_t *)(USB_BASE + 0x050))
// FIFO size registers
#define GRXFSIZ             (*(volatile uint32_t *)(USB_BASE + 0x024))
#define GNPTXFSIZ           (*(volatile uint32_t *)(USB_BASE + 0x028))
#define DIEPTXF0            (*(volatile uint32_t *)(USB_BASE + 0x104))  // EP0 dedicated TX FIFO
#define DIEPTXF(n)          (*(volatile uint32_t *)(USB_BASE + 0x104 + 4*(n)))  // for later EPs

// Device mode registers
#define DCFG                (*(volatile uint32_t *)(USB_BASE + 0x800))
#define DCTL                (*(volatile uint32_t *)(USB_BASE + 0x804))
#define DSTS                (*(volatile uint32_t *)(USB_BASE + 0x808))
#define DIEPMSK             (*(volatile uint32_t *)(USB_BASE + 0x810))
#define DOEPMSK             (*(volatile uint32_t *)(USB_BASE + 0x814))
#define DAINT               (*(volatile uint32_t *)(USB_BASE + 0x818))
#define DAINTMSK            (*(volatile uint32_t *)(USB_BASE + 0x81C))

// Endpoint registers (we'll expand later)
#define DIEPCTL0            (*(volatile uint32_t *)(USB_BASE + 0x900))
#define DOEPCTL0            (*(volatile uint32_t *)(USB_BASE + 0xB00))

// Bit definitions we need early
#define GRSTCTL_CSFTRST     (1U << 0)
#define GRSTCTL_AHBIDLE     (1U << 31)

#define GUSBCFG_PHYSEL      (1U << 6)
#define GUSBCFG_FDMOD       (1U << 30)   // Force Device Mode
#define GUSBCFG_FHMOD       (1U << 29)   // Force Host Mode

#define GAHBCFG_GLBLINTRMSK (1U << 0)
#define GAHBCFG_DMAEN       (1U << 5)

#define DCTL_SFTDISCON      (1U << 1)    // Soft disconnect
#define DCTL_CGOUTNAK       (1U << 8)
#define DCTL_CGNPINNAK      (1U << 7)

#define DCFG_DSPD_HS        (0x0 << 0)   // High speed
#define DCFG_DSPD_FS        (0x1 << 0)   // Full speed
#define DCFG_NZLSOHSK       (1U << 2)

#ifdef __cplusplus
extern "C" {
#endif

void usb_hw_prepare(void);
#ifdef __cplusplus
}
#endif

#endif
