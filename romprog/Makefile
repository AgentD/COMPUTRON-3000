.PHONY: all
all: avrstuff romprog

.PHONY: clean
clean:
	$(RM) *.hex *.eep *.elf *.map *.sym *.lss *.bin romprog

romprog: romprog.c
	$(CC) -Wall -Wextra $^ -o $@

################################ AVR related ################################
MCU = atmega8
F_CPU = 16000000

AVRCFLAGS = -gstabs
AVRCFLAGS += -DF_CPU=$(F_CPU)UL $(CINCS)
AVRCFLAGS += -Os
AVRCFLAGS += -funsigned-char -funsigned-bitfields -fpack-struct -fshort-enums
AVRCFLAGS += -Wall -Wstrict-prototypes
AVRCFLAGS += -std=gnu99

AVRDUDE_PROGRAMMER = jtag1
AVRDUDE_PORT = /dev/ttyUSB0
AVRDUDE_WRITE_FLASH = -U flash:w:avrprog.hex
AVRDUDE_FLAGS = -p $(MCU) -P $(AVRDUDE_PORT) -c $(AVRDUDE_PROGRAMMER)

AVRCC = avr-gcc
AVROBJCOPY = avr-objcopy
AVROBJDUMP = avr-objdump
AVRNM = avr-nm
AVRDUDE = avrdude

.PHONY: avrstuff
avrstuff: avrprog.elf avrprog.hex avrprog.eep avrprog.lss avrprog.sym

.PHONY: program
program: avrprog.hex avrprog.eep
	$(AVRDUDE) $(AVRDUDE_FLAGS) $(AVRDUDE_WRITE_FLASH) $(AVRDUDE_WRITE_EEPROM)

avrprog.hex: avrprog.elf
	$(AVROBJCOPY) -O ihex -R .eeprom $< $@

avrprog.eep: avrprog.elf
	$(AVROBJCOPY) -j .eeprom --set-section-flags=.eeprom="alloc,load" \
	--change-section-lma .eeprom=0 -O ihex $< $@

avrprog.lss: avrprog.elf
	$(AVROBJDUMP) -h -S $< > $@

avrprog.sym: avrprog.elf
	$(AVRNM) -n $< > $@

avrprog.elf: avrprog.c
	$(AVRCC) -mmcu=$(MCU) $(AVRCFLAGS) $^ -o $@ -Wl,-Map=avrprog.map,--cref

