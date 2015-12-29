gaggia-overlay.dtb: gaggia-overlay.dts
	dtc -@ -I dts -O dtb -o gaggia-overlay.dtb gaggia-overlay.dts

install: gaggia-overlay.dtb
	cp gaggia-overlay.dtb /boot/overlays

