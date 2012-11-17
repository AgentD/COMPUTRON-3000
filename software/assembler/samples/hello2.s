;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Intel 8080 sample file
;;
;; Print out "Hello World\n" to device 0x0F
;; (terminal in COMPUTRON 3000 emulator)
;;
;; Used to test assembler label handling
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

    LXI H, string

loop:
    MOV A, M
    CPI 0x00
    JZ  end
    OUT 0x0F
    INX H
    JMP loop

end:
    HLT

string:
    DB "Hello World\n\0"

