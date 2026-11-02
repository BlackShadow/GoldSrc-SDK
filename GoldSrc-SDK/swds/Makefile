CXX = g++
AR = ar
CONFIG ?= Release

OUT = bin/Linux32/$(CONFIG)
OBJ = bin/obj/swds/Linux32/$(CONFIG)
TARGET = $(OUT)/libswds-reconstruction.a

CPPFLAGS = -Isrc/engine -Isrc/common -Isrc/public -Isrc/public/tier1
CXXFLAGS = -m32 -march=pentium-m -mfpmath=387 -std=gnu++11 \
	-gdwarf-2 -g2 -fno-delete-null-pointer-checks -fno-gnu-unique \
	-fvisibility=hidden -fpermissive -fno-strict-aliasing \
	-fexpensive-optimizations -fstack-protector -Wall -Wextra -Werror

ifeq ($(CONFIG),Debug)
CXXFLAGS += -O0
else
CXXFLAGS += -O3
endif

SOURCES = src/common/ObjectList.cpp src/common/TokenLine.cpp \
	src/engine/sys_engine.cpp src/public/interface.cpp src/public/registry.cpp
OBJECTS = $(SOURCES:src/%.cpp=$(OBJ)/%.o)

.PHONY: all
all: $(TARGET)

$(TARGET): $(OBJECTS)
	@mkdir -p $(@D)
	$(AR) rcs $@ $^

# The native loader retains truncated retry names and ignores getcwd failure.
$(OBJ)/public/interface.o: CXXFLAGS += -Wno-format-truncation -Wno-unused-result

$(OBJ)/%.o: src/%.cpp
	@mkdir -p $(@D)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MMD -MP -c $< -o $@

-include $(OBJECTS:.o=.d)
