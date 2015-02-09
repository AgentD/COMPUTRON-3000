#!/bin/sh

mkdir out

pcb -x gerber cpu.pcb --copy-outline all --gerberfile cpu
pcb -x gerber mem.pcb --copy-outline all --gerberfile mem
pcb -x gerber uart.pcb --copy-outline all --gerberfile uart
pcb -x gerber ide.pcb --copy-outline all --gerberfile ide
pcb -x gerber himem.pcb --copy-outline all --gerberfile himem

mv *.gbr ./out/
mv *.cnc ./out/

