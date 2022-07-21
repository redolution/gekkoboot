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

CFLAGS	= -g -O2 -Wall $(MACHDEP) $(INCLUDE)
CXXFLAGS	=	$(CFLAGS)

LDFLAGS	=	-g $(MACHDEP) -Wl,-Map,$(notdir $@).map -T$(PWD)/ipl.ld

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

export OUTPUT	:=	$(CURDIR)/$(TARGET)
export OUTPUT_SX := $(CURDIR)/qoob_sx_$(TARGET)_upgrade

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

export OUTPUT	:=	$(CURDIR)/$(TARGET)
.PHONY: all dol gci qoob qoobsx viper xeno compressed_dol compressed_gci $(BUILD) clean run

export BUILD_MAKE := @mkdir -p $(BUILD) && $(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

#---------------------------------------------------------------------------------
all: dol gci qoob viper compressed_dol compressed_gci
dol:
	$(BUILD_MAKE) $(OUTPUT).dol
gci:
	$(BUILD_MAKE) $(OUTPUT).gci
qoob:
	$(BUILD_MAKE) $(OUTPUT).gcb
qoobsx:
	$(BUILD_MAKE) $(OUTPUT_SX).elf
viper:
	$(BUILD_MAKE) $(OUTPUT).vgc
xeno:
	$(BUILD_MAKE) $(OUTPUT)_xeno.dol
compressed_dol:
	$(BUILD_MAKE) $(OUTPUT)_xz.dol
compressed_gci:
	$(BUILD_MAKE) $(OUTPUT)_xz.gci

#---------------------------------------------------------------------------------
$(BUILD): all

#---------------------------------------------------------------------------------
clean:
	@echo clean ...
	@rm -rf $(BUILD)
	@rm -f $(OUTPUT)* $(OUTPUT_SX)*

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
# Base DOL (PicoBoot)
#---------------------------------------------------------------------------------
$(OUTPUT).dol: $(OUTPUT).elf
$(OUTPUT).elf: $(OFILES)

$(OFILES_SOURCES) : $(HFILES)

#---------------------------------------------------------------------------------
# GCI
#---------------------------------------------------------------------------------
%.gci: %.dol
	@echo converting DOL to GCI ... $(notdir $@)
	@dol2gci $< $@ boot.dol #TODO: should not use boot.dol save name?

#---------------------------------------------------------------------------------
# Qoob
#---------------------------------------------------------------------------------
$(OUTPUT).gcb: $(OUTPUT).dol
	@echo packing Qoob IPL ... $(notdir $@)
	@cd $(PWD); ./dol2ipl.py ipl.rom $< $@

#---------------------------------------------------------------------------------
# Qoob SX
#---------------------------------------------------------------------------------
$(OUTPUT)_xz.qbsx: $(OUTPUT)_xz.elf
	@echo packing Qoob SX IPL ... $(notdir $@)
	@cd $(PWD); ./dol2ipl.py /dev/null $< $@

$(OUTPUT_SX).elf: $(OUTPUT)_xz.qbsx
	@echo splicing Qoob SX bios ... $(notdir $@)
	@cd $(PWD); cp -f qoob_sx_13c_upgrade.elf $@
	@cd $(PWD); dd if=$< of=$@ obs=4 seek=1851 conv=notrunc
	@cd $(PWD); printf 'QOOB SX iplboot install\0' \
		| dd of=$@ obs=4 seek=1810 conv=notrunc

#---------------------------------------------------------------------------------
# Viper
#---------------------------------------------------------------------------------
$(OUTPUT).vgc: $(OUTPUT).dol
	@echo packing Viper IPL... $(notdir $@)
	@cd $(PWD); ./dol2ipl.py /dev/null $< $@

#---------------------------------------------------------------------------------
# Xeno
#---------------------------------------------------------------------------------
$(OUTPUT)_xeno.dol: $(OUTPUT)_xeno.elf
$(OUTPUT)_xeno.elf: $(OFILES)
	@echo linking Xeno ... $(notdir $@)
	@$(LD)  $^ $(LDFLAGS) -Wl,--section-start,.init=0x81700000 $(LIBPATHS) $(LIBS) -o $@

#---------------------------------------------------------------------------------
# Compression
#---------------------------------------------------------------------------------
$(OUTPUT)_xz.dol: $(OUTPUT).dol
	@echo compressing DOL ... $(notdir $@)
	@dolxz $< $@ -cube

$(OUTPUT)_xz.elf: $(OUTPUT)_xz.dol
	@echo converting compressed DOL to ELF ... $(notdir $@)
	@doltool -e $< #TODO: Where does this tool come from?

-include $(DEPENDS)

#---------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------
