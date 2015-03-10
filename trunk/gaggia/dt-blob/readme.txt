Step 1: check that Raspberry Pi firmware is later than July 15th 2014
---------------------------------------------------------------------

vcgencmd version

Step 2: install device tree compiler
------------------------------------

sudo apt-get update
sudo apt-get install device-tree-compiler

Step 3: edit the device tree source file as needed
--------------------------------------------------

nano dt-blob.dts

Step 4: compile the device tree source file to binary blob
----------------------------------------------------------

sudo dtc -I dts -O dtb -o /boot/dt-blob.bin dt-blob.dts
