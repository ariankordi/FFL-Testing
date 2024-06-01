# Compiler and flags
CXX := g++
INCLUDES := -IninTexUtils/include -Irio/include -Iffl/include -Iinclude
LDFLAGS := -lz -lGL -lGLEW
# found that glfw3 is different on some systems
LDFLAGS += $(shell pkg-config --libs glfw3)

DEFS := -DFFL_MLC_PATH="\"./\"" -DRIO_DEBUG -DNDEBUG -DFFL_NO_OPEN_DATABASE -DFFL_TEST_DISABLE_MII_COLOR_VERIFY

# Executable
EXEC := ffl_testing_2_debug64

# OS-specific settings
ifeq ($(shell uname), Darwin)
    # macOS-specific flags
    INCLUDES += -I/opt/homebrew/include
    LDFLAGS += -L/opt/homebrew/lib -framework OpenGL -framework Foundation -framework CoreGraphics -framework AppKit -framework IOKit
    EXEC := ffl_testing_2_debug_apple
endif

CXXFLAGS += -g -std=c++17 $(INCLUDES) $(DEFS)

# Source directories
NINTEXUTILS_SRC := $(shell find ninTexUtils/src/ninTexUtils -name '*.c' -o -name '*.cpp')
RIO_SRC := $(shell find rio/src -name '*.c' -o -name '*.cpp')
FFL_SRC := $(shell find ffl/src -name '*.c' -o -name '*.cpp')
MAIN_SRC := $(wildcard ./**/*.cpp)

# Object files
NINTEXUTILS_OBJ := $(NINTEXUTILS_SRC:.c=.o)
NINTEXUTILS_OBJ := $(NINTEXUTILS_OBJ:.cpp=.o)
RIO_OBJ := $(RIO_SRC:.cpp=.o)
FFL_OBJ := $(FFL_SRC:.cpp=.o)
MAIN_OBJ := $(MAIN_SRC:.cpp=.o)

# Default target
all: $(EXEC)
# TODO: add git submodule update --init --recursive

# Linking the executable
$(EXEC): $(NINTEXUTILS_OBJ) $(RIO_OBJ) $(FFL_OBJ) $(MAIN_OBJ)
	$(CXX) $^ -o $@ $(CXXFLAGS) $(LDFLAGS)

# Compiling source files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

%.o: %.c
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean up
clean:
	rm -f $(NINTEXUTILS_OBJ) $(RIO_OBJ) $(FFL_OBJ) $(MAIN_OBJ) $(EXEC)

# Phony targets
.PHONY: all clean

