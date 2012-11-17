;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Intel 8080 sample file
;;
;; Print out "Hello World\n" to device 0x0F
;; (terminal in COMPUTRON 3000 emulator)
;;
;; Used to test assembler label handling
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

.def term 0x0F      ; the terminal device

    LXI H, string

loop:
    MOV A, M
    CPI 0x00
    JZ  end
    OUT term
    INX H
    JMP loop

end:
    HLT

string:
    DB "Hello World\n\0"

