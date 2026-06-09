PROJECT = main
BUILD_DIR = bin

CFILES = tft-gmt020.c

DEVICE = stm32f411ce
OOCD_FILE = board/stm32f4discovery.cfg

# You shouldn't have to edit anything below here.
INCLUDES += $(patsubst %,-I%, .)
OPENCM3_DIR = ../../../../libopencm3

include $(OPENCM3_DIR)/mk/genlink-config.mk
include ../../../../rules.mk
include $(OPENCM3_DIR)/mk/genlink-rules.mk

flashbin:
	st-flash write main.bin 0x8000000
