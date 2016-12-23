#!/bin/sh

mkdir -p out

for pcb in *.pcb
do
	name=$(basename $pcb .pcb)

	pcb -x gerber $pcb --copy-outline all --gerberfile $name

	mv $name.*.{gbr,cnc} ./out/
done
