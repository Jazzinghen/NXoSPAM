  .text
  .align 0

  .global nx_extract
/* Alignment manager: avoids data abort exception while extracting unaligned
 * data.
 *
 * r0 -> pointer to the array
 * r1 -> required size in bytes
 */
nx_extract:
  stmfd sp!, {r4}
  mov r3, #0
  sub r4, r1, #4
L1:
  ldrb r2, [r0]
  orr r3, r2
  mov r3, r3, ror #8
  subs r1, r1, #1
  addne r0, r0, #1
  bne L1
  mov r0, r3
L2:
  adds r4, r4, #1
  movle r0, r0, ror #8
  ble L2
  ldmfd sp!, {r4}
  bx lr
