.section .text.vectors, "ax"
.balign 32
.global exception_vectors
exception_vectors:
    ldr pc, =reset_entry
    ldr pc, =undef_entry
    ldr pc, =swi_entry
    ldr pc, =prefetch_entry
    ldr pc, =dabt_entry
    ldr pc, =reserved_entry
    ldr pc, =irq_entry
    ldr pc, =fiq_entry
.ltorg

.text
.balign 4

@ Save r0-r12 + adjusted PC and hand them to fault_report(code, spsr, frame)
.macro FAULT_ENTRY name, code, adj
\name:
    sub   lr, lr, #\adj
    stmfd sp!, {r0-r12, lr}
    mov   r0, #\code
    mrs   r1, spsr
    mov   r2, sp
    bl    fault_report
    b     .
.endm

FAULT_ENTRY undef_entry,    1, 4
FAULT_ENTRY swi_entry,      2, 4
FAULT_ENTRY prefetch_entry, 3, 4
FAULT_ENTRY dabt_entry,     4, 8
FAULT_ENTRY reserved_entry, 5, 4
FAULT_ENTRY fiq_entry,      6, 4
FAULT_ENTRY reset_entry,    7, 0

irq_entry:
    sub   lr, lr, #4
    stmfd sp!, {r0-r3, r12, lr}
    vmrs  r0, fpscr                  @ ISRs may use VFP: save the caller-saved state
    stmfd sp!, {r0, r1}              @ (r1 is padding to keep sp 8-byte aligned)
    vstmdb sp!, {d0-d7}
    bl    irq_dispatch
    vldmia sp!, {d0-d7}
    ldmfd sp!, {r0, r1}
    vmsr  fpscr, r0
    ldmfd sp!, {r0-r3, r12, pc}^     @ return and restore CPSR from SPSR
