#!/bin/sh

mkdir out

pcb -x gerber --metric cpu.pcb --copy-outline all --gerberfile cpu
pcb -x gerber --metric mem.pcb --copy-outline all --gerberfile mem

mv *.gbr ./out/
mv *.cnc ./out/

