.section .text.boot, "ax"
.global _start
.global main

_start:
    // Set stack pointer
    ldr sp, =0x80000

    // Enable VFP: grant access to CP10/CP11, then switch the VFP unit on
    mrc p15, 0, r0, c1, c0, 2
    orr r0, r0, #(0xF << 20)
    mcr p15, 0, r0, c1, c0, 2
    mov r0, #0
    mcr p15, 0, r0, c7, c5, 4        // prefetch flush so the new access rights apply
    mov r0, #0x40000000
    vmsr fpexc, r0

    // Clear BSS
    ldr r0, =__bss_start__
    ldr r1, =__bss_end__
    mov r2, #0

clear_bss_loop:
    cmp r0, r1
    bge clear_bss_done
    str r2, [r0], #4
    b clear_bss_loop

clear_bss_done:
    // Jump to C main
    bl main

hang:
    b hang
