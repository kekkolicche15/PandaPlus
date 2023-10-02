# Cross toolchain variables
# If these are not in your path, you can make them absolute.
XT_PRG_PREFIX = mipsel-linux-gnu-
CC = $(XT_PRG_PREFIX)gcc
LD = $(XT_PRG_PREFIX)ld

# uMPS3-related paths
ifneq ($(wildcard /usr/bin/umps3),)
	UMPS3_DIR_PREFIX = /usr
else
	UMPS3_DIR_PREFIX = /usr/local
endif

UMPS3_DATA_DIR = $(UMPS3_DIR_PREFIX)/share/umps3
UMPS3_INCLUDE_DIR = $(UMPS3_DIR_PREFIX)/include/umps3
UMPS3_INCLUDE_DIR2 = $(UMPS3_DIR_PREFIX)/include/umps3/umps
UMPS3_INCLUDE_DIR3 = /usr/include
# Simplistic search for the umps3 installation prefix.
# If you have umps3 installed on some weird location, set UMPS3_DIR_PREFIX by hand.

# Compiler options
CFLAGS_LANG = -ffreestanding -list -Wall -O0
CFLAGS_MIPS = -mips1 -mabi=32 -mno-gpopt -EL -G 0 -mno-abicalls -fno-pic -mfp32
CFLAGS = $(CFLAGS_LANG) $(CFLAGS_MIPS) -I$(UMPS3_INCLUDE_DIR) -I$(UMPS3_INCLUDE_DIR3) -I$(UMPS3_INCLUDE_DIR2) -Ih 

# Linker options
LDFLAGS = -G 0 -nostdlib -T $(UMPS3_DATA_DIR)/umpscore.ldscript -m elf32ltsmip

# Add the location of crt*.S to the search path
VPATH = $(UMPS3_DATA_DIR)
BINDIR  := ./bin
SRCDIR1 := ./phase1
SRCDIR2 := ./phase2
VMDIR  := ./vm
FILES := $(basename $(notdir $(wildcard $(SRCDIR1)/*.c))) $(basename $(notdir $(wildcard $(SRCDIR2)/*.c))) libumps crtso
OBJECTS = $(addprefix $(BINDIR)/, $(addsuffix .o,$(FILES)))
HEADERS = $(wildcard ./h/*.h) $(wildcard ./h/umps/*.h)

$(shell mkdir -p $(BINDIR))
$(shell mkdir -p $(VMDIR))

all : $(BINDIR)/kernel.core.umps

$(BINDIR)/kernel.core.umps : $(BINDIR)/kernel
	umps3-elf2umps -k $<
	@printf "{\n\t\"boot\":{\n\t\t\"core-file\":\"../bin/kernel.core.umps\",\n\t\t\"load-core-file\":true\n\t},\n\t\"bootstrap-rom\":\"${UMPS3_DIR_PREFIX}/share/umps3/coreboot.rom.umps\",\n\t\"clock-rate\":1,\n\t\"devices\":{\n\t\t\"terminal0\":{\n\t\t\t\"enabled\":true,\n\t\t\t\"file\":\"term0.umps\"\n\t\t}},\n\t\"execution-rom\":\"${UMPS3_DIR_PREFIX}/share/umps3/exec.rom.umps\",\n\t\"num-processors\":1,\n\t\"num-ram-frames\":64,\n\t\"symbol-table\":{\n\t\t\"asid\":64,\n\t\t\"file\":\"../bin/kernel.stab.umps\"\n\t},\n\t\"tlb-floor-address\":\"0x80000000\",\n\t\"tlb-size\":16\n}" > $(VMDIR)/umps3

$(BINDIR)/kernel : $(OBJECTS)
	$(LD) -o $@ $^ $(LDFLAGS)

clean :
	-rm -rf $(BINDIR)
	-rm -rf $(VMDIR)

# Pattern rule for assembly modules
$(BINDIR)/%.o: $(SRCDIR1)/%.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@
$(BINDIR)/%.o: $(SRCDIR2)/%.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@
$(BINDIR)/%.o: %.S
	$(CC) $(CFLAGS) -c $< -o $@