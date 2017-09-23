#
# tn40xx_ Driver
#

DRV_NAME := tn40xx
DRV_OBJ  := tn40.o MV88X3120_phy.o QT2025_phy.o CX4.o TLK10232_phy.o AQ2104_phy.o
EXTRA_CFLAGS += -D_TN40xx_DEV_ -I$(M)/$(DRV_NAME)
ifeq ($(TRACE),YES)
	DRV_OBJ += trace.o
	EXTRA_CFLAGS += -D_TRACE_LOG_
endif
obj-m    += $(DRV_NAME).o
$(DRV_NAME)-objs := $(DRV_OBJ)
