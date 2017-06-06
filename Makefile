MCU=attiny85
AVRDUDEMCU=t85
#F_CPU=1000000L
F_CPU=8000000L
CC=/usr/bin/avr-g++
CFLAGS=-g -Os -Wall -mcall-prologues -mmcu=$(MCU) -IBasicSerial -DF_CPU=$(F_CPU) -Wno-write-strings
OBJ2HEX=/usr/bin/avr-objcopy
AVRDUDE=/usr/bin/avrdude
TARGET=blinky

all :
	$(CC) -c $(CFLAGS) BasicSerial/BasicSerial.S -o BasicSerial.o
	$(CC) $(CFLAGS) $(TARGET).cpp BasicSerial.o -o $(TARGET)
	$(OBJ2HEX) -R .eeprom -O ihex $(TARGET) $(TARGET).hex
	rm -f $(TARGET)

install : all
	sudo gpio -g mode 22 out
	sudo gpio -g write 22 0
	sudo $(AVRDUDE) -p $(AVRDUDEMCU) -P /dev/spidev0.0 -c linuxspi -b 10000 -U flash:w:$(TARGET).hex
	sudo gpio -g write 22 1

noreset : all
	sudo $(AVRDUDE) -p $(AVRDUDEMCU) -P /dev/spidev0.0 -c linuxspi -b 10000 -U flash:w:$(TARGET).hex

fuse :
	sudo gpio -g mode 22 out
	sudo gpio -g write 22 0
# http://www.engbedded.com/fusecalc
# Default of use 8mhz divide clock by 8, serial program downloading SPI enabled
#	sudo $(AVRDUDE) -p $(AVRDUDEMCU) -P /dev/spidev0.0 -c linuxspi -b 10000 -U lfuse:w:0x62:m -U hfuse:w:0xdf:m -U efuse:w:0xff:m
	sudo $(AVRDUDE) -p $(AVRDUDEMCU) -P /dev/spidev0.0 -c linuxspi -b 10000 -U lfuse:w:0xe2:m -U hfuse:w:0xdf:m -U efuse:w:0xff:m
	sudo gpio -g write 22 1

clean :
	rm -f *.hex *.obj *.o
