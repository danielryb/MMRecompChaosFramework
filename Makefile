BUILD_DIR := build

# Allow the user to specify the compiler and linker on macOS
# as Apple Clang does not support MIPS architecture
ifeq ($(OS),Windows_NT)
    CC      := clang
    CXX     := clang++
    LD      := ld.lld
else ifneq ($(shell uname),Darwin)
    CC      := clang-19
    CXX     := clang++-19
    LD      := ld.lld
else
    CC      ?= clang-19
    CXX     ?= clang++-19
    LD      ?= ld.lld
endif

TARGET  := $(BUILD_DIR)/mod.elf
DEBUG   := 1

LDSCRIPT := mod.ld
ARCHFLAGS := -target mips -mips2 -mabi=32 -O2 -G0 -mno-abicalls -mno-odd-spreg -mno-check-zero-division \
             -fomit-frame-pointer -ffast-math -fno-unsafe-math-optimizations -fno-builtin-memset
WARNFLAGS := -Wall -Wextra -Wno-incompatible-library-redeclaration -Wno-unused-parameter -Wno-unknown-pragmas -Wno-unused-variable \
             -Wno-missing-braces -Wno-unsupported-floating-point-opt -Werror=section
CFLAGS   := $(ARCHFLAGS) $(WARNFLAGS) -D_LANGUAGE_C -nostdinc -ffunction-sections
CXXFLAGS := $(ARCHFLAGS) $(WARNFLAGS) -D_LANGUAGE_C_PLUS_PLUS -stdlib=libc++ -fno-rtti -fno-exceptions -std=c++20 -ffunction-sections -DLIBC_ASSERT_H \
             -ferror-limit=1000
CPPFLAGS := -DMIPS -DF3DEX_GBI_2 -DF3DEX_GBI_PL -DGBI_DOWHILE -I include -I include/dummy_headers \
            -I mm-decomp/include -I mm-decomp/src -I mm-decomp/extracted/n64-us -idirafter include/libc -idirafter mm-decomp/include/libc \
            -I src -Wno-constant-conversion -I src/mod
LDFLAGS  := -nostdlib -T $(LDSCRIPT) -Map $(BUILD_DIR)/mod.map --unresolved-symbols=ignore-all --emit-relocs -e 0 --no-nmagic -gc-sections

ifeq ($(DEBUG), 1)
    CFLAGS += -DDEBUG
    CXXFLAGS += -DDEBUG
endif

ifeq ($(OS),Windows_NT)
else ifneq ($(shell uname),Darwin)
    # Intercept specific includes on Linux to prevent them from including the glibc counterparts.
    # This is done this way because -nostdinc would prevent a system install of libc++ from being included as well.
    CXXFLAGS += -I include/libc/stdlib_override
else
    # Intercept specific includes on MacOS to prevent them from including the MacOS SDK counterparts.
    # This is done this way because -nostdinc would prevent a system install of libc++ from being included as well.
    CXXFLAGS += -I include/libc/stdlib_override_mac
endif

rwildcard = $(foreach d,$(wildcard $(1:=/*)),$(call rwildcard,$d,$2) $(filter $(subst *,%,$2),$d))
getdirs = $(sort $(dir $(1)))

C_SRCS := $(call rwildcard,src,*.c)
C_OBJS := $(addprefix $(BUILD_DIR)/, $(C_SRCS:.c=.o))
C_DEPS := $(addprefix $(BUILD_DIR)/, $(C_SRCS:.c=.d))

CXX_SRCS := $(call rwildcard,src,*.cpp)
CXX_OBJS := $(addprefix $(BUILD_DIR)/, $(CXX_SRCS:.cpp=.o))
CXX_DEPS := $(addprefix $(BUILD_DIR)/, $(CXX_SRCS:.cpp=.d))

ALL_OBJS := $(C_OBJS) $(CXX_OBJS)
ALL_DEPS := $(C_DEPS) $(CXX_DEPS)
BUILD_DIRS := $(call getdirs,$(ALL_OBJS))

all: $(TARGET)

$(TARGET): $(ALL_OBJS) $(LDSCRIPT) | $(BUILD_DIR)
	$(LD) $(ALL_OBJS) $(LDFLAGS) -o $@

$(BUILD_DIR) $(BUILD_DIRS):
ifeq ($(OS),Windows_NT)
	if not exist "$(subst /,\,$@)" mkdir "$(subst /,\,$@)"
else
	mkdir -p $@
endif

$(C_OBJS): $(BUILD_DIR)/%.o : %.c | $(BUILD_DIRS)
	$(CC) $(CFLAGS) $(CPPFLAGS) $< -MMD -MF $(@:.o=.d) -c -o $@

$(CXX_OBJS): $(BUILD_DIR)/%.o : %.cpp | $(BUILD_DIRS)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $< -MMD -MF $(@:.o=.d) -c -o $@

clean:
ifeq ($(OS),Windows_NT)
	if exist $(BUILD_DIR) rmdir /S /Q $(BUILD_DIR)
else
	rm -rf $(BUILD_DIR)
endif

-include $(ALL_DEPS)

.PHONY: clean all

# Print target for debugging
print-% : ; $(info $* is a $(flavor $*) variable set to [$($*)]) @true
