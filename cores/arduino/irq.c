#include <stdint.h>
#include <stddef.h>
#include "irq.h"

#define INTC       0x2000B200UL
#define IRQ_PEND1  (*(volatile uint32_t *)(INTC + 0x04))
#define IRQ_PEND2  (*(volatile uint32_t *)(INTC + 0x08))
#define IRQ_EN1    (*(volatile uint32_t *)(INTC + 0x10))
#define IRQ_EN2    (*(volatile uint32_t *)(INTC + 0x14))
#define IRQ_ENB    (*(volatile uint32_t *)(INTC + 0x18))
#define IRQ_DIS1   (*(volatile uint32_t *)(INTC + 0x1C))
#define IRQ_DIS2   (*(volatile uint32_t *)(INTC + 0x20))
#define IRQ_DISB   (*(volatile uint32_t *)(INTC + 0x24))

#define GPIO_BASE  0x20200000UL
#define GPEDS0     (*(volatile uint32_t *)(GPIO_BASE + 0x40))
#define GPEDS1     (*(volatile uint32_t *)(GPIO_BASE + 0x44))

#define IRQ_STACK_WORDS 4096          // 16 KB: usb_task() runs here when USB_BG_ENABLE is 1
#define STACK_WORDS     1024          // 4 KB for abort and undefined modes
static uint32_t irq_stack[IRQ_STACK_WORDS] __attribute__((aligned(8)));
static uint32_t abt_stack[STACK_WORDS]     __attribute__((aligned(8)));
static uint32_t und_stack[STACK_WORDS]     __attribute__((aligned(8)));

extern uint32_t exception_vectors[];
extern void handle_gpio_interrupt(void);   // WInterrupts.cpp

static irq_handler_t handlers[64];
static volatile uint32_t enabled1, enabled2;

void irq_register(unsigned irq, irq_handler_t fn) {
    if (irq < 64) handlers[irq] = fn;
}

void irq_enable(unsigned irq) {
    if (irq < 32)      { enabled1 |= 1u << irq;        IRQ_EN1 = 1u << irq; }
    else if (irq < 64) { enabled2 |= 1u << (irq - 32); IRQ_EN2 = 1u << (irq - 32); }
}

void irq_disable(unsigned irq) {
    if (irq < 32)      { enabled1 &= ~(1u << irq);        IRQ_DIS1 = 1u << irq; }
    else if (irq < 64) { enabled2 &= ~(1u << (irq - 32)); IRQ_DIS2 = 1u << (irq - 32); }
}

static void run_irq(unsigned n) {
    if (handlers[n]) handlers[n]();
    else irq_disable(n);               // unhandled source: mask it so it can't storm
}

void irq_dispatch(void) {              // called from irq_entry in vectors.S
    uint32_t p1 = IRQ_PEND1 & enabled1;
    uint32_t p2 = IRQ_PEND2 & enabled2;
    while (p1) { unsigned n = __builtin_ctz(p1);      p1 &= p1 - 1; run_irq(n); }
    while (p2) { unsigned n = 32 + __builtin_ctz(p2); p2 &= p2 - 1; run_irq(n); }
}

void irq_init(void) {
    uint32_t r;

    // VFP on (idempotent): the IRQ entry code saves/restores VFP registers
    __asm__ volatile("mrc p15,0,%0,c1,c0,2" : "=r"(r));
    r |= 0xFu << 20;
    __asm__ volatile("mcr p15,0,%0,c1,c0,2" :: "r"(r) : "memory");
    __asm__ volatile("mcr p15,0,%0,c7,c5,4" :: "r"(0) : "memory");
    __asm__ volatile("vmsr fpexc, %0" :: "r"(0x40000000u));

    // Controller quiet, stale GPIO events cleared
    IRQ_DIS1 = 0xFFFFFFFFu;
    IRQ_DIS2 = 0xFFFFFFFFu;
    IRQ_DISB = 0xFFFFFFFFu;
    GPEDS0 = 0xFFFFFFFFu;
    GPEDS1 = 0xFFFFFFFFu;

    // Stack pointers for IRQ, abort and undefined modes
    uint32_t irq_top = (uint32_t)&irq_stack[STACK_WORDS];
    uint32_t abt_top = (uint32_t)&abt_stack[STACK_WORDS];
    uint32_t und_top = (uint32_t)&und_stack[STACK_WORDS];
    __asm__ volatile(
        "mrs  r3, cpsr      \n"
        "cps  #0x12         \n"    // IRQ mode
        "mov  sp, %0        \n"
        "cps  #0x17         \n"    // abort mode
        "mov  sp, %1        \n"
        "cps  #0x1B         \n"    // undefined mode
        "mov  sp, %2        \n"
        "msr  cpsr_c, r3    \n"    // back to where we were
        :: "r"(irq_top), "r"(abt_top), "r"(und_top)
        : "r3", "memory");

    // Vector table: low vectors (V=0) + VBAR
    __asm__ volatile("mrc p15,0,%0,c1,c0,0" : "=r"(r));
    r &= ~(1u << 13);
    __asm__ volatile("mcr p15,0,%0,c1,c0,0" :: "r"(r) : "memory");
    __asm__ volatile("mcr p15,0,%0,c12,c0,0" :: "r"((uint32_t)exception_vectors) : "memory");
    __asm__ volatile("mcr p15,0,%0,c7,c5,4" :: "r"(0) : "memory");

    // GPIO bank interrupts (gpio_int[0..3] = IRQ 49..52)
    for (unsigned n = 49; n <= 52; n++) {
        irq_register(n, handle_gpio_interrupt);
        irq_enable(n);
    }

    __asm__ volatile("cpsie i" ::: "memory");   // unmask IRQs
}
