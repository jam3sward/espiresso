gaggia: gaggia.cpp \
	pwm.o inputs.o timing.o pid.o gpio.o temperature.o boiler.o keyboard.o
	g++ -o gaggia gaggia.cpp \
	pwm.o inputs.o timing.o pid.o gpio.o temperature.o boiler.o keyboard.o -lrt

pwm.o: pwm.h pwm.cpp
	g++ -c pwm.cpp

temperature.o: temperature.h temperature.cpp
	g++ -c temperature.cpp

timing.o: timing.h timing.cpp
	g++ -c timing.cpp

pid.o: pid.h pid.cpp
	g++ -c pid.cpp

gpio.o: gpio.h gpio.cpp
	g++ -c gpio.cpp

boiler.o: boiler.h boiler.cpp
	g++ -c boiler.cpp

keyboard.o: keyboard.h keyboard.cpp
	g++ -c keyboard.cpp

inputs.o: inputs.h inputs.cpp
	g++ -c inputs.cpp
