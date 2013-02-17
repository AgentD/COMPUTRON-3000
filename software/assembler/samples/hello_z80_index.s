;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Zilog z80 sample file
;;
;; Test indexed addressing
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

.def term 0x0F      ; the terminal device

    LD IX, string

    LD A, (IX+0)
    OUT (term), A
    LD A, (IX+1)
    OUT (term), A
    LD A, (IX+2)
    OUT (term), A
    LD A, (IX+3)
    OUT (term), A
    LD A, (IX+4)
    OUT (term), A
    LD A, (IX+11)
    OUT (term), A

    HLT

string:
    .db "Hello World\n\0"

