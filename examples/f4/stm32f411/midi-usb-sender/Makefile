PROJECT = main
BUILD_DIR = bin

#SHARED_DIR = ../my-common-code
CFILES = midiusb-sender.c

DEVICE=stm32f411ce
OOCD_FILE = board/stm32f4discovery.cfg

# You shouldn't have to edit anything below here.
VPATH += $(SHARED_DIR)
INCLUDES += $(patsubst %,-I%, . $(SHARED_DIR))
OPENCM3_DIR=../../../../libopencm3

include $(OPENCM3_DIR)/mk/genlink-config.mk
include ../../../../rules.mk
include $(OPENCM3_DIR)/mk/genlink-rules.mk
flashbin:
	st-flash write main.bin 0x8000000
