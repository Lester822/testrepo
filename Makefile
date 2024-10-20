DIR = src
TARGET = quash
MAIN_SRC = execute

GPP = g++
GPPFLAGS = -Wall -g -std=c++11

build:
	$(GPP) $(GPPFLAGS) $(DIR)/$(MAIN_SRC).cpp -o $(TARGET)

test: build
	/.$(TARGET)

clean:
	rm -f $(TARGET)