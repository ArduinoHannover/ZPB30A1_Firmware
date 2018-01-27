CC=sdcc
CFLAGS=-mstm8
DEFINES=STM8S005
PROCESSOR=stm8s005k6
SOURCE=electronic_load.c
OUTPUT_DIR=build/
OUTPUT_FW=firmware/
OUTPUT_FILE=electronic_load

# SDCC includes working wrong on Windows with "/" slahes
ifeq ($(OS),Windows_NT)
INCLUDES=-I.\\inc
else
INCLUDES=-I./inc
endif

all:	compile flash

compile:
	mkdir -p $(OUTPUT_DIR)
	mkdir -p $(OUTPUT_FW)
	$(CC) $(CFLAGS) $(INCLUDES) -D $(DEFINES) -o $(OUTPUT_DIR) $(SOURCE)
	packihx $(OUTPUT_DIR)$(OUTPUT_FILE).ihx > $(OUTPUT_FW)$(OUTPUT_FILE).hex

clean:
	rm $(OUTPUT_DIR)/*

flash_windows:
	stvp_cmdline -BoardName=ST-LINKV2 -Device=$(PROCESSOR) -Port=USB -ProgMode=SWIM \
	-verbose -no_loop -no_log \
	-FileProg=$(OUTPUT_FW)$(OUTPUT_FILE).hex

flash_unix:
	stm8flash -c stlinkv2 -p $(PROCESSOR) -w $(OUTPUT_FW)$(OUTPUT_FILE).hex

ifeq ($(OS),Windows_NT)
flash:	flash_windows
else
flash:	flash_unix
endif
