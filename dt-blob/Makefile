dt-blob.dtb: dt-blob.dts
	dtc -I dts -O dtb -o dt-blob.dtb dt-blob.dts

install: dt-blob.dtb
	cp dt-blob.dtb /boot/dt-blob.bin
