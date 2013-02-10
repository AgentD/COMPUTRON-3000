#!/bin/sh

mkdir out

pcb -x gerber cpu.pcb --copy-outline all --gerberfile cpu
pcb -x gerber mem.pcb --copy-outline all --gerberfile mem

mv *.gbr ./out/
mv *.cnc ./out/

