BIN=zmt.bin

# ZGDK has sound enabled by default, uncomment this to disable sound and reduce your binary size
ENABLE_GFX=0
ENABLE_SOUND=0
ENABLE_CRC32=0
# ENABLE_CONIO=1
# ENABLE_WINDOWS=1


ifdef BREAK
ZOS_CFLAGS += -DBREAK
endif

ifndef ZGDK_PATH
	$(error "Failure: ZGDK_PATH variable not found. It must point to ZGDK path.")
endif

GFX_STRIP = 48

ifeq ($(EMULATOR), 1)
ZOS_CFLAGS += -DEMULATOR=1
endif

# include $(ZGDK_PATH)/base_sdcc.mk
include $(ZVB_SDK_PATH)/sdcc/base_sdcc.mk

## Add your own rules here

STAT_BYTES = stat
ifeq ($(detected_OS),Darwin)
	STAT_BYTES += -f %z
# TODO: Support Windows?
else
	STAT_BYTES += -c %s
endif

all::
	@echo "Binary Size" $$($(STAT_BYTES) $(OUTPUT_DIR)/$(BIN)) $(BIN)

run:
	$(ZEAL_NATIVE_BIN) -H bin -r $(ZEAL_NATIVE_ROM) -t tf.img -e eeprom.img

native: all run
