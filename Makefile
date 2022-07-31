#---------------------------------------------------------------------------------
# Clear the implicit built in rules
#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------
ifeq ($(strip $(DEVKITPRO)),)
$(error "Please set DEVKITPRO in your environment. export DEVKITPRO=<path to>devkitPro")
endif

ifeq ($(strip $(DEVKITPPC)),)
$(error "Please set DEVKITPPC in your environment. export DEVKITPPC=<path to>devkitPPC")
endif

include $(DEVKITPRO)/libogc2/gamecube_rules

#---------------------------------------------------------------------------------
# TARGET is the name of the output
# BUILD is the directory where object files & intermediate files will be placed
# SOURCES is a list of directories containing source code
# INCLUDES is a list of directories containing extra header files
#---------------------------------------------------------------------------------
TARGET		:=	$(notdir $(CURDIR))
BUILD		:=	build
SOURCES		:=	source source/fatfs
DATA		:=	data
INCLUDES	:=

#---------------------------------------------------------------------------------
# options for code generation
#---------------------------------------------------------------------------------

CFLAGS		= -g -Os -flto -Wall $(MACHDEP) $(INCLUDE)
CXXFLAGS	= $(CFLAGS)

LDFLAGS		= -g $(MACHDEP) -Wl,-Map,$(notdir $@).map -T$(PWD)/ipl.ld

#---------------------------------------------------------------------------------
# any extra libraries we wish to link with the project
#---------------------------------------------------------------------------------
LIBS	:=	-logc

#---------------------------------------------------------------------------------
# list of directories containing libraries, this must be the top level containing
# include and lib
#---------------------------------------------------------------------------------
LIBDIRS	:=

#---------------------------------------------------------------------------------
# no real need to edit anything past this point unless you need to add additional
# rules for different file extensions
#---------------------------------------------------------------------------------
ifneq ($(BUILD),$(notdir $(CURDIR)))
#---------------------------------------------------------------------------------

export OUTPUT		:=	$(CURDIR)/$(BUILD)/$(TARGET)
export OUTPUT_SX	:=	$(CURDIR)/$(BUILD)/qoob_sx_$(TARGET)_upgrade

export VPATH	:=	$(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) \
			$(foreach dir,$(DATA),$(CURDIR)/$(dir))

export DEPSDIR	:=	$(CURDIR)/$(BUILD)

#---------------------------------------------------------------------------------
# automatically build a list of object files for our project
#---------------------------------------------------------------------------------
CFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
sFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
SFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.S)))
BINFILES	:=	$(foreach dir,$(DATA),$(notdir $(wildcard $(dir)/*.*)))

#---------------------------------------------------------------------------------
# use CXX for linking C++ projects, CC for standard C
#---------------------------------------------------------------------------------
ifeq ($(strip $(CPPFILES)),)
	export LD	:=	$(CC)
else
	export LD	:=	$(CXX)
endif

export OFILES_BIN	:=	$(addsuffix .o,$(BINFILES))
export OFILES_SOURCES := $(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(sFILES:.s=.o) $(SFILES:.S=.o)
export OFILES := $(OFILES_BIN) $(OFILES_SOURCES)

export HFILES := $(addsuffix .h,$(subst .,_,$(BINFILES)))

#---------------------------------------------------------------------------------
# build a list of include paths
#---------------------------------------------------------------------------------
export INCLUDE	:=	$(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
			$(foreach dir,$(LIBDIRS),-I$(dir)/include) \
			-I$(CURDIR)/$(BUILD) \
			-I$(LIBOGC_INC)

#---------------------------------------------------------------------------------
# build a list of library paths
#---------------------------------------------------------------------------------
export LIBPATHS	:=	-L$(LIBOGC_LIB) $(foreach dir,$(LIBDIRS),-L$(dir)/lib)

#---------------------------------------------------------------------------------
# build tools
#---------------------------------------------------------------------------------
export BUILDTOOLS	:=	$(CURDIR)/buildtools
export DOL2IPL		:=	$(BUILDTOOLS)/dol2ipl.py
ifeq ($(OS),Windows_NT)
export DOLXZ		:=	$(BUILDTOOLS)/dolxz.exe
export DOL2GCI		:=	$(BUILDTOOLS)/dol2gci.exe
export DOLTOOL		:=	$(BUILDTOOLS)/doltool.exe
else
export DOLXZ		:=	$(BUILDTOOLS)/dolxz
export DOL2GCI		:=	$(BUILDTOOLS)/dol2gci
export DOLTOOL		:=	$(BUILDTOOLS)/doltool
endif

#---------------------------------------------------------------------------------

.PHONY: all dol gci qoobpro qoobsx viper dol_compressed gci_compressed $(BUILD) clean run

export BUILD_MAKE := @mkdir -p $(BUILD) && $(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

#---------------------------------------------------------------------------------
all: dol gci qoobpro qoobsx viper dol_compressed gci_compressed
dol:
	$(BUILD_MAKE) $(OUTPUT).dol
gci:
	$(BUILD_MAKE) $(OUTPUT).gci
qoobpro:
	$(BUILD_MAKE) $(OUTPUT).gcb
qoobsx:
	$(BUILD_MAKE) $(OUTPUT_SX).elf $(OUTPUT_SX).dol
viper:
	$(BUILD_MAKE) $(OUTPUT).vgc
dol_compressed:
	$(BUILD_MAKE) $(OUTPUT)_xz.dol
gci_compressed:
	$(BUILD_MAKE) $(OUTPUT)_xz.gci

#---------------------------------------------------------------------------------
$(BUILD): all

#---------------------------------------------------------------------------------
clean:
	@echo clean ...
	@rm -rf $(BUILD)

#---------------------------------------------------------------------------------
run: $(BUILD)
	usb-load $(OUTPUT)_xz.dol

#---------------------------------------------------------------------------------
else

DEPENDS	:=	$(OFILES:.o=.d)

#---------------------------------------------------------------------------------
# main targets
#---------------------------------------------------------------------------------

#---------------------------------------------------------------------------------
# Base DOL
#---------------------------------------------------------------------------------
$(OUTPUT).dol: $(OUTPUT).elf
$(OUTPUT).elf: $(OFILES)

$(OFILES_SOURCES) : $(HFILES)

#---------------------------------------------------------------------------------
# GCI
#---------------------------------------------------------------------------------
%.gci: %.dol
	@echo convert DOL to GCI ... $(notdir $@) $(DOL2GCI)
	@$(DOL2GCI) $< $@ boot.dol

#---------------------------------------------------------------------------------
# Qoob Pro
#---------------------------------------------------------------------------------
$(OUTPUT).gcb: $(OUTPUT).dol
	@echo pack Qoob Pro IPL ... $(notdir $@)
	@cd $(PWD); $(DOL2IPL) ipl.rom $< $@

#---------------------------------------------------------------------------------
# Qoob SX
#---------------------------------------------------------------------------------
$(OUTPUT)_xz.qbsx: $(OUTPUT)_xz.elf
	@echo pack Qoob SX IPL ... $(notdir $@)
	@cd $(PWD); $(DOL2IPL) /dev/null $< $@

$(OUTPUT_SX).elf: $(OUTPUT)_xz.qbsx
	@echo splice Qoob SX updater ... $(notdir $@)
	@cd $(PWD); cp -f qoob_sx_13c_upgrade.elf $@
	@cd $(PWD); dd if=$< of=$@ obs=4 seek=1851 conv=notrunc
	@cd $(PWD); printf 'QOOB SX iplboot install\0' \
		| dd of=$@ obs=4 seek=1810 conv=notrunc

#---------------------------------------------------------------------------------
# Viper
#---------------------------------------------------------------------------------
$(OUTPUT).vgc: $(OUTPUT).dol
	@echo pack Viper IPL... $(notdir $@)
	@cd $(PWD); $(DOL2IPL) /dev/null $< $@

#---------------------------------------------------------------------------------
# Compression
#---------------------------------------------------------------------------------
$(OUTPUT)_xz.dol: $(OUTPUT).dol
	@echo compress DOL ... $(notdir $@)
	@$(DOLXZ) $< $@ -cube

$(OUTPUT)_xz.elf: $(OUTPUT)_xz.dol
	@echo convert compressed DOL to ELF ... $(notdir $@)
	@$(DOLTOOL) -e $<

-include $(DEPENDS)

#---------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------
