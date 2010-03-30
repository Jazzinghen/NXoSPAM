.code 32
.text
.align 0

  .global nx_bmp_msb_pos
nx_bmp_msb_pos:
  movs r1, r0
  moveq r0, #0
  bxeq lr
  mov r0, #33
getmsb_cycle:
  sub r0, r0, #1
  movs r1, r1, lsl #1
  bxcs lr
  b getmsb_cycle

  .global nx_bmp_set
nx_bmp_set:
  ldr r2, [r0]
  subs r1, r1, #1
  movmi r2, #0
  movpl r3, #1
  movpl r3, r3, lsl r1
  orrpl r2, r2, r3
  str r2, [r0]
  bx lr

  .global nx_bmp_reset
nx_bmp_reset:
  ldr r2, [r0]
  mov r3, #1
  subs r1, r1, #1
  bxeq lr
  mov r3, r3, lsl r1
  bic r2, r2, r3
  str r2, [r0]
  bx lr

