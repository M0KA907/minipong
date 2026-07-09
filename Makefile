#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------

ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif

include $(DEVKITARM)/gba_rules

# gbafix is optional; ROM still builds without it.
%.gba: %.elf
	$(OBJCOPY) -O binary $< $@
	@echo built ... $(notdir $@)
	@if command -v gbafix >/dev/null 2>&1; then \
		gbafix $@ -t$(GAME_TITLE) -c$(GAME_CODE) -m$(MAKER_CODE); \
	fi

#---------------------------------------------------------------------------------
TARGET		:=	minipong
BUILD		:=	build
SOURCES		:=	source
INCLUDES	:=	include

GAME_TITLE	:=	MINIPONG
GAME_CODE	:=	MPNG
MAKER_CODE	:=	00

#---------------------------------------------------------------------------------
# options for code generation
#---------------------------------------------------------------------------------
ARCH	:=	-mthumb -mthumb-interwork

CFLAGS	:=	-std=c99 -Wall -Wextra \
		$(ARCH)

CFLAGS	+=	$(INCLUDE) -O2

ifdef DEBUG
CFLAGS	+=	-g -O0 -DDEBUG=1
endif

CXXFLAGS	:= $(CFLAGS) -fno-rtti -fno-exceptions

ASFLAGS	:=	$(ARCH)
LDFLAGS	=	$(ARCH) -Wl,-Map,$(notdir $*.map)

LIBS	:=	-ltonc

LIBDIRS	:=	$(LIBGBA) $(DEVKITPRO)/libtonc

#---------------------------------------------------------------------------------
ifneq ($(BUILD),$(notdir $(CURDIR)))
#---------------------------------------------------------------------------------

export OUTPUT	:=	$(CURDIR)/$(TARGET)
export VPATH	:=	$(foreach dir,$(SOURCES),$(CURDIR)/$(dir))

export DEPSDIR	:=	$(CURDIR)/$(BUILD)

CFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
SFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))

export LD	:=	$(CC)

export OFILES_SOURCES := $(CFILES:.c=.o) $(SFILES:.s=.o)
export OFILES := $(OFILES_SOURCES)

export INCLUDE	:=	$(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
			$(foreach dir,$(LIBDIRS),-I$(dir)/include) \
			-I$(CURDIR)/$(BUILD)

export LIBPATHS	:=	$(foreach dir,$(LIBDIRS),-L$(dir)/lib)

.PHONY: $(BUILD) clean run test

#---------------------------------------------------------------------------------
$(BUILD):
	@[ -d $@ ] || mkdir -p $@
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

#---------------------------------------------------------------------------------
clean:
	@echo clean ...
	@rm -fr $(BUILD) $(TARGET).gba $(TARGET).elf

#---------------------------------------------------------------------------------
run: $(BUILD)
	@timeout 10 mgba-qt $(TARGET).gba 2>/dev/null || timeout 10 mgba $(TARGET).gba

#---------------------------------------------------------------------------------
# host-compiled unit tests (pure-C modules only; bodies added as modules land)
test:
	@mkdir -p $(BUILD)
	gcc -std=c99 -Wall -Wextra -Iinclude tests/test_iso.c source/iso.c -o $(BUILD)/test_iso
	@$(BUILD)/test_iso
	gcc -std=c99 -Wall -Wextra -Iinclude tests/test_game.c source/game.c source/iso.c -o $(BUILD)/test_game
	@$(BUILD)/test_game

#---------------------------------------------------------------------------------
else

DEPENDS	:=	$(OFILES:.o=.d)

#---------------------------------------------------------------------------------
# main targets
#---------------------------------------------------------------------------------
$(OUTPUT).gba	:	$(OUTPUT).elf

$(OUTPUT).elf	:	$(OFILES)

#---------------------------------------------------------------------------------
-include $(DEPSDIR)/*.d

endif
#---------------------------------------------------------------------------------
