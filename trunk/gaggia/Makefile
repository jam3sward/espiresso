gaggia: gaggia.cpp settings.h \
	pwm.o inputs.o timing.o pid.o gpio.o temperature.o boiler.o keyboard.o \
	gpiopin.o ranger.o flow.o system.o
	g++ -o gaggia gaggia.cpp \
	pwm.o inputs.o timing.o pid.o gpio.o temperature.o boiler.o keyboard.o \
	gpiopin.o ranger.o flow.o system.o \
	-lrt -lpthread -std=c++0x

pwm.o: pwm.h pwm.cpp settings.h
	g++ -c pwm.cpp

temperature.o: temperature.h temperature.cpp settings.h
	g++ -c temperature.cpp

timing.o: timing.h timing.cpp
	g++ -c timing.cpp

pid.o: pid.h pid.cpp
	g++ -c pid.cpp

gpio.o: gpio.h gpio.cpp
	g++ -c gpio.cpp

boiler.o: boiler.h boiler.cpp settings.h
	g++ -c boiler.cpp

keyboard.o: keyboard.h keyboard.cpp
	g++ -c keyboard.cpp

inputs.o: inputs.h inputs.cpp
	g++ -c inputs.cpp

gpiopin.o: gpiopin.h gpiopin.cpp
	g++ -c gpiopin.cpp

ranger.o: ranger.h ranger.cpp settings.h
	g++ -c ranger.cpp

flow.o: flow.h flow.cpp settings.h
	g++ -c flow.cpp -std=c++0x

system.o: system.h system.cpp
	g++ -c system.cpp
