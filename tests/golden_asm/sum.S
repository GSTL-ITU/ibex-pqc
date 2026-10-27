# uint32_t sum_to(uint32_t n): returns 1 + 2 + ... + n

  .section .text
  .global sum_to
sum_to:
  li    t0, 0
1:
  beqz  a0, 2f
  add   t0, t0, a0
  addi  a0, a0, -1
  j     1b
2:
  mv    a0, t0
  ret
