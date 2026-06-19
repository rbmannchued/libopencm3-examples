PROJECT = main
BUILD_DIR = bin

#SHARED_DIR = ../my-common-code
CFILES = display-ili9341.c \
ili9341-libopencm3/ili9341.c \
ili9341-libopencm3/fonts.c

DEVICE=stm32f446ce
OOCD_FILE = board/stm32f4discovery.cfg

# You shouldn't have to edit anything below here.
VPATH += $(SHARED_DIR)
INCLUDES += $(patsubst %,-I%, . $(SHARED_DIR))
INCLUDES += -I ili9341-libopencm3/
OPENCM3_DIR=../../../../libopencm3

include $(OPENCM3_DIR)/mk/genlink-config.mk
include ../../../../rules.mk
include $(OPENCM3_DIR)/mk/genlink-rules.mk
flashbin:
	st-flash write main.bin 0x8000000
