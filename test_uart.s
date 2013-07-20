#
# UART base address: 0xF8
#
# addr | read                       | write
# _____|____________________________|___________________________
# 0x00 | receive holding reigster   | transmit holding reigster
# 0x01 | interrupt enable reigster  | interrupt enable reigster
# 0x02 | interrupt statues reigster | FIFO control register
# 0x03 | line control register      | line control register
# 0x04 | modem control register     | modem control register
# 0x05 | line status register       | reserved
# 0x06 | modem status register      | reserved
# 0x07 | scratch pad register       | scratch pad register
# _____|____________________________|___________________________
# if line control register bit 7 is set:
# ______________________________________________________________
# addr | read                       | write
# _____|____________________________|___________________________
# 0x00 | LSB of divisor latch       | LSB of divisor latch
# 0x01 | MSB of divisor latch       | MSB of divisor latch
#
#
# interrupt enable register[00]:
#   bit 3: modem status interrupt
#   bit 2: line status interrupt
#   bit 1: holding register
#   bit 0: holding register
# interrupt status register[01]:
#   bit 7: FIFO's enabled
#   bit 6: FIFO's enabled
#   bit 3: INT priority bit 2
#   bit 2: INT priority bit 1
#   bit 1: INT priority bit 0
#   bit 0: INT status
# FIFO control register[01]:
#   bit 7: RX trigger (MSB)
#   bit 6: RX trigger (LSB)
#   bit 3: DMA mode select
#   bit 2: TX FIFO reset
#   bit 1: RX FIFO reset
#   bit 0: FIFO enable
# Line control register[00]:
#   bit 7: divisor latch enable
#   bit 6: set break
#   bit 5: set parity
#   bit 4: even parity
#   bit 3: parity enable
#   bit 2: stop bits
#   bit 1: word length (MSB)
#   bit 0: word length (LSB)
# Modem control register[00]:
#   bit 4: loop back enable
# Line status register[60]:
#   bit 7: FIFO data error
#   bit 6: trans. empty
#   bit 5: trans. holding empty
#   bit 4: break interrupt
#   bit 3: framing error
#   bit 2: parity error
#   bit 1: overrun error
#   bit 0: receive data ready
# modem status register (useless in C3k system)
#
# baud rate | divisor   LCR 5|LCR 4|LCR 3| Parity
# __________|_________  _____|_____|_____|________________
#       300 | 384           X|    X|    0| No parity
#      9600 |  12           0|    0|    1| Odd parity
#     38400 |   3           0|    1|    1| Even parity
#    115200 |   1           1|    0|    1| Force parity 1
#                           1|    1|    1| Force parity 0
#
# LCR 2|word lenght|stop bits     LCR 1|LCR 0|word lenght
# _____|___________|_________     _____|_____|___________
#    0 | 5,6,7,8   | 1              0  |  0  | 5
#    1 | 5         | 1-1/2          0  |  1  | 6
#    1 | 6,7,8     | 2              1  |  0  | 7
#                                   1  |  1  | 8
# FCR 7|FCR 6| RX trigger
# _____|_____|___________
#   0  |  0  |  1
#   0  |  1  |  4
#   1  |  0  |  8
#   1  |  1  | 14
#
.org 0x0000

DI

LD A, 0x00      # disable 16C550 interrupts
OUT (0xF9), A

LD A, 0x80
OUT (0xFB), A   # enable divisor latch

LD A, 0x0C      # set divisor 0x00C0 -> 9600 baud
OUT (0xF8), A
LD A, 0x00
OUT (0xF9), A

LD A, 0x03      # disable divisor latch, 8 bit data, 1 stop bit, no parity
OUT (0xFB), A

LD A, 0xC7      # enable and clear FIFO with 14 byte threshold
OUT (0xFA), A

LD C, 0xF8      # C = 0xF8 -> transmit holding register
LD D, 'A'       # D = 'A'
LD E, '\n'      # E = '\n'
LD A, 0x55
LD SP, 0x7FFF

loop:
    OUT (C), D      # write 'A' to transmit holding register
    OUT (C), E      # write '\n' to transmit holding register
    OUT (0x00), A   # write A to bank switch register
    CPL             # A = ~A

    # execute the "wait" loop 5000 times
    PUSH BC
    PUSH DE
    LD BC, 5000
    CALL wait
    POP DE
    POP BC

    JR loop         # goto loop

wait:
    NOP
    NOP
    NOP
    NOP
    NOP
    NOP
    NOP
    NOP
    NOP
    NOP
    DEC BC          # --BC

    LD D, A         # check if BC==0
    LD A, B
    OR C
    LD A, D
    JR NZ, wait     # if not, goto wait

    RET


