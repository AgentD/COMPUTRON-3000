;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Intel 8080 sample file
;;
;; Print out "Hello World\n" to device 0x0F
;; (terminal in COMPUTRON 3000 emulator)
;;
;; Used to test basic assembler translation before
;; label handling, etc was implemented
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

    MVI A, 'H'
    OUT 0x0F
    MVI A, 'e'
    OUT 0x0F
    MVI A, 'l'
    OUT 0x0F
    MVI A, 'l'
    OUT 0x0F
    MVI A, 'o'
    OUT 0x0F
    MVI A, ' '
    OUT 0x0F
    MVI A, 'W'
    OUT 0x0F
    MVI A, 'o'
    OUT 0x0F
    MVI A, 'r'
    OUT 0x0F
    MVI A, 'l'
    OUT 0x0F
    MVI A, 'd'
    OUT 0x0F
    MVI A, '\n'
    OUT 0x0F

    HLT

