/*
 * SPDX-FileCopyrightText: Copyright (c) 2021, Ha Thach (tinyusb.org)
 * SPDX-License-Identifier: MIT
 *
 * This file is part of the TinyUSB stack.
 */

#ifndef TUSB_DWC2_BCM_H_
#define TUSB_DWC2_BCM_H_

#ifdef __cplusplus
 extern "C" {
#endif

#include <stdint.h>

// Pi Zero (BCM2835) USB OTG register base. Must equal USB_BASE in your dwc2.h.
#define USB_OTG_GLOBAL_BASE  0x20980000u
#define USB_IRQn             9          // only used if you later hook a real interrupt

// First test: we poll instead of using interrupts, so these do nothing.
static inline void BP_EnableIRQ(int n)  { (void) n; }
static inline void BP_DisableIRQ(int n) { (void) n; }

// No cache is enabled in your start.S, so cache maintenance is a no-op.
static inline void data_clean(void *a, uint32_t s)                { (void) a; (void) s; }
static inline void data_invalidate(void *a, uint32_t s)           { (void) a; (void) s; }
static inline void data_clean_and_invalidate(void *a, uint32_t s) { (void) a; (void) s; }

#define DWC2_EP_MAX         8

static const dwc2_controller_t _dwc2_controller[] =
{
  { .reg_base = USB_OTG_GLOBAL_BASE, .irqnum = USB_IRQn, .ep_count = DWC2_EP_MAX, .otg_dfifo_depth = 4096 }
};

#define dcache_clean(_addr, _size)              data_clean(_addr, _size)
#define dcache_invalidate(_addr, _size)         data_invalidate(_addr, _size)
#define dcache_clean_invalidate(_addr, _size)   data_clean_and_invalidate(_addr, _size)

// MCU specific to enable dwc2 clock/power before any access to register
TU_ATTR_ALWAYS_INLINE static inline void dwc2_clock_init(uint8_t rhport, tusb_role_t role) {
  (void) rhport;
  (void) role;
}

TU_ATTR_ALWAYS_INLINE static inline void dwc2_int_set(uint8_t rhport, tusb_role_t role, bool enabled) {
  (void) role;
  if (enabled) {
    BP_EnableIRQ(_dwc2_controller[rhport].irqnum);
  } else {
    BP_DisableIRQ(_dwc2_controller[rhport].irqnum);
  }
}

static inline void dwc2_remote_wakeup_delay(void)
{
  // try to delay for 1 ms
  // TODO implement later
}

// MCU specific PHY init, called BEFORE core reset
static inline void dwc2_phy_init(dwc2_regs_t * dwc2, uint8_t hs_phy_type)
{
  (void) dwc2;
  (void) hs_phy_type;

  // nothing to do
}

// MCU specific PHY deinit, disable PHY power
static inline void dwc2_phy_deinit(dwc2_regs_t * dwc2, uint8_t hs_phy_type) {
  (void) dwc2;
  (void) hs_phy_type;
  // nothing to do
}

// MCU specific PHY update, it is called AFTER init() and core reset
static inline void dwc2_phy_update(dwc2_regs_t * dwc2, uint8_t hs_phy_type)
{
  (void) dwc2;
  (void) hs_phy_type;

  // nothing to do
}

#ifdef __cplusplus
 }
#endif

#endif
