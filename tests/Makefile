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

#TARGET  := $(BUILD_DIR)/mod.elf
DEBUG   := 0

SOURCE_DIR := ../src

#LDSCRIPT := mod.ld
#ARCHFLAGS := -target mips -mips2 -mabi=32 -O2 -G0 -mno-abicalls -mno-odd-spreg -mno-check-zero-division \
             -fomit-frame-pointer -ffast-math -fno-unsafe-math-optimizations -fno-builtin-memset
WARNFLAGS := -Wall -Wextra -Wno-incompatible-library-redeclaration -Wno-unused-parameter -Wno-unknown-pragmas -Wno-unused-variable \
             -Wno-missing-braces -Wno-unsupported-floating-point-opt -Werror=section
CFLAGS   := $(ARCHFLAGS) $(WARNFLAGS) -D_LANGUAGE_C -ffunction-sections -g
CXXFLAGS := $(ARCHFLAGS) $(WARNFLAGS) -D_LANGUAGE_C_PLUS_PLUS -stdlib=libc++ -fno-rtti -fno-exceptions -std=c++20 -ffunction-sections -DLIBC_ASSERT_H \
             -ferror-limit=1000 -g
CPPFLAGS := \
 			-DF3DEX_GBI_2 -DF3DEX_GBI_PL -DGBI_DOWHILE \
            -I $(SOURCE_DIR) -I $(SOURCE_DIR)/mod -Wno-constant-conversion -I include # \
			-DMIPS \
 			-I include -I include/dummy_headers \
            -I mm-decomp/include -I mm-decomp/src -I mm-decomp/extracted/n64-us -idirafter include/libc -idirafter mm-decomp/include/libc \

#LDFLAGS  := -nostdlib -T $(LDSCRIPT) -Map $(BUILD_DIR)/mod.map --unresolved-symbols=ignore-all --emit-relocs -e 0 --no-nmagic -gc-sections

ifeq ($(DEBUG), 1)
    CFLAGS += -DDEBUG
    CXXFLAGS += -DDEBUG
endif

IGNORE := $(addprefix ./$(SOURCE_DIR)/, $(file < .srcignore))

OBJ=$(join $(addsuffix ../obj/, $(dir $(SOURCE))), $(notdir $(SOURCE:.cpp=.o)))

rwildcard = $(foreach d,$(filter-out $(IGNORE), $(wildcard $(1:=/*))),$(call rwildcard,$d,$2) $(filter $(subst *,%,$2),$d))
getdirs = $(sort $(dir $(1)))

# Tests source files
TEST_C_SRCS := $(wildcard ./*.c)
TEST_C_EXES := $(TEST_C_SRCS:.c=)

TEST_CXX_SRCS := $(wildcard ./*.cpp)
TEST_CXX_EXES := $(TEST_CXX_SRCS:.cpp=)

TEST_ALL_EXES := $(TEST_C_EXES) $(TEST_CXX_EXES)

# Mod source files
C_SRCS := $(call rwildcard,./$(SOURCE_DIR),*.c)
C_OBJS := $(addprefix $(BUILD_DIR)/, $(subst ./../,,$(C_SRCS:.c=.o)))

CXX_SRCS := $(call rwildcard,./$(SOURCE_DIR),*.cpp)
CXX_OBJS := $(addprefix $(BUILD_DIR)/, $(subst ./../,,$(CXX_SRCS:.cpp=.o)))

# Include files
INC_C_SRCS := $(call rwildcard,./include,*.c)
INC_C_OBJS := $(addprefix $(BUILD_DIR)/, $(INC_C_SRCS:.c=.o))

INC_CXX_SRCS := $(call rwildcard,./include,*.cpp)
INC_CXX_OBJS := $(addprefix $(BUILD_DIR)/, $(INC_CXX_SRCS:.cpp=.o))

ALL_OBJS := $(C_OBJS) $(CXX_OBJS) $(INC_C_OBJS) $(INC_CXX_OBJS)
BUILD_DIRS := $(call getdirs,$(ALL_OBJS))

all: $(TEST_ALL_EXES)

# TODO Move .o to build
$(TEST_C_EXES): % : %.c $(ALL_OBJS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(CPPFLAGS) $@.c -c -o $@.o
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(ALL_OBJS) $@.o -o $@

$(TEST_CXX_EXES): % : %.cpp $(ALL_OBJS) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(ALL_OBJS) $@.cpp -o $@

$(BUILD_DIR) $(BUILD_DIRS):
ifeq ($(OS),Windows_NT)
	if not exist "$(subst /,\,$@)" mkdir "$(subst /,\,$@)"
else
	mkdir -p $@
endif

$(C_OBJS): $(BUILD_DIR)/%.o : ../%.c | $(BUILD_DIRS)
	$(CC) $(CFLAGS) $(CPPFLAGS) $< -c -o $@

$(CXX_OBJS): $(BUILD_DIR)/%.o : ../%.cpp | $(BUILD_DIRS)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $< -c -o $@

$(INC_C_OBJS): $(BUILD_DIR)/%.o : %.c | $(BUILD_DIRS)
	$(CC) $(CFLAGS) $(CPPFLAGS) $< -c -o $@

$(INC_CXX_OBJS): $(BUILD_DIR)/%.o : %.cpp | $(BUILD_DIRS)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $< -c -o $@

clean:
ifeq ($(OS),Windows_NT)
	if exist $(BUILD_DIR) rmdir /S /Q $(BUILD_DIR)
else
	rm -rf $(BUILD_DIR) $(TEST_ALL_EXES) $(TEST_C_EXES).o
endif

-include $(ALL_DEPS)

.PHONY: clean all

# Print target for debugging
print-% : ; $(info $* is a $(flavor $*) variable set to [$($*)]) @true
