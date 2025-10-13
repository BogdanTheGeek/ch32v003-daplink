all : flash

TARGET := main
CH32FUN ?= ch32fun/ch32fun
RV003USB ?= rv003usb
DAPLINK ?= ../DAP
TARGET_MCU := CH32V003

ADDITIONAL_C_FILES += $(filter-out $(TARGET).c, $(wildcard *.c))
ADDITIONAL_C_FILES += $(RV003USB)/rv003usb/rv003usb.S $(RV003USB)/rv003usb/rv003usb.c
ADDITIONAL_C_FILES += $(wildcard $(DAPLINK)/Source/*.c)
EXTRA_CFLAGS := -I$(RV003USB)/lib -I$(RV003USB)/rv003usb -I$(DAPLINK)/Include

include $(CH32FUN)/ch32fun.mk

flash : cv_flash
clean : cv_clean


