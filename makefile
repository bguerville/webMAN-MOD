CELL_MK_DIR = $(CELL_SDK)/samples/mk
include $(CELL_MK_DIR)/sdk.makedef.mk

BUILD_TYPE     	= release

LIBSTUB_DIR		= ./lib
PRX_DIR			= .
INSTALL			= cp
PEXPORTPICKUP		= ppu-lv2-prx-exportpickup
PRX_LDFLAGS_EXTRA	= -L ./lib -Wl,--strip-unused-data

CRT_HEAD                += $(shell ppu-lv2-gcc -print-file-name'='ecrti.o)
CRT_HEAD                += $(shell ppu-lv2-gcc -print-file-name'='crtbegin.o)
CRT_TAIL                += $(shell ppu-lv2-gcc -print-file-name'='crtend.o)
CRT_HEAD                += $(shell ppu-lv2-gcc -print-file-name'='ecrtn.o)

PPU_SRCS = printf.c libc.c main.c cobra/cobra.c
PPU_PRX_TARGET = webftp_server.elf
PPU_PRX_LDFLAGS += $(PRX_LDFLAGS_EXTRA)
PPU_PRX_STRIP_FLAGS = -s
PPU_PRX_LDLIBS 	= -lfs_stub -lnet_stub -lrtc_stub -lio_stub
PPU_PRX_LDLIBS 	+= -lnetctl_stub -lsysmodule_stub -lhttp_util_stub -lhttp_stub 
#-lgcm_sys_stub
PPU_PRX_LDLIBS 	+= -lcrashdump_system_export_stub \
                   -lsysPrxForUser_export_stub \
                   -lvsh_export_stub \
                   -lpaf_export_stub \
                   -lvshmain_export_stub \
                   -lvshtask_export_stub \
                   -lallocator_export_stub \
                   -lsdk_export_stub \
                   -lstdc_export_stub \
                   -lpngdec_ppuonly_export_stub \
                   -lxsetting_export_stub \
                   -lvshnet_export_stub \
                   -lnetctl_main_export_stub

PPU_CFLAGS += -Os -ffunction-sections -fdata-sections -fno-builtin-printf -nodefaultlibs -std=gnu99 -Wno-shadow -Wno-unused-parameter
#PPU_CFLAGS += -finline-limit=100

ifeq ($(BUILD_TYPE), debug)
PPU_CFLAGS += -DDEBUG -DDEBUG_FILE
endif

all:
	$(MAKE) $(PPU_OBJS_DEPENDS)
	$(PPU_PRX_STRIP) --strip-debug --strip-section-header $(PPU_PRX_TARGET)
	$(PRX_DIR)/scetool -0 SELF -1 TRUE -s FALSE -2 04 -3 1070000052000001 -4 01000002 -5 APP -6 0003004000000000 -A 0001000000000000 -e $(PPU_PRX_TARGET) $(PPU_SPRX_TARGET)
	

include $(CELL_MK_DIR)/sdk.target.mk




