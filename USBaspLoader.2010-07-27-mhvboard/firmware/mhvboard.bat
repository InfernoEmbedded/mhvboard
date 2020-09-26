avrdude -c usbasp -p atmega328p -U lfuse:w:0xf7:m -U hfuse:w:0xda:m -U efuse:w:0x03:m && ^
avrdude -c usbasp -p atmega328p -U flash:w:main.hex:i && ^
avrdude -c usbasp -p atmega328p -U lock:w:0x2f:m

