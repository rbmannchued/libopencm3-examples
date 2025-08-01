PROJECT = main
BUILD_DIR = bin

CFILES = FreeRTOS-blink.c \
FreeRTOS-LTS/FreeRTOS/FreeRTOS-Kernel/list.c \
FreeRTOS-LTS/FreeRTOS/FreeRTOS-Kernel/queue.c \
FreeRTOS-LTS/FreeRTOS/FreeRTOS-Kernel/tasks.c \
FreeRTOS-LTS/FreeRTOS/FreeRTOS-Kernel/timers.c \
FreeRTOS-LTS/FreeRTOS/FreeRTOS-Kernel/event_groups.c \
FreeRTOS-LTS/FreeRTOS/FreeRTOS-Kernel/portable/GCC/ARM_CM4F/port.c \
FreeRTOS-LTS/FreeRTOS/FreeRTOS-Kernel/portable/MemMang/heap_4.c


DEVICE=stm32f411ce
OOCD_FILE = board/stm32f4discovery.cfg


VPATH += $(SHARED_DIR)
INCLUDES += $(patsubst %,-I%, . $(SHARED_DIR))
INCLUDES += -I FreeRTOS-LTS/FreeRTOS/FreeRTOS-Kernel/portable/GCC/ARM_CM4F/
INCLUDES += -I FreeRTOS-LTS/FreeRTOS/FreeRTOS-Kernel/include/
OPENCM3_DIR=../../../../libopencm3
CFLAGS += -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard
include $(OPENCM3_DIR)/mk/genlink-config.mk
include ../../../../rules.mk
include $(OPENCM3_DIR)/mk/genlink-rules.mk
flashbin:
	st-flash write main.bin 0x8000000
