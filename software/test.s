.org 0x0000

DI
LD A, 0x55

loop:
    OUT (0x00), A   # write A to bank switch register
    CPL             # A = ~A

    # execute the "wait" loop 5000 times
    LD BC, 5000

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

    JR loop         # goto loop

