# C++ Runtime Library Makefile
# Bare-metal C++ runtime for LLVM integration

# Compiler settings
CXX = g++
CXXFLAGS = -ffreestanding -fno-exceptions -fno-rtti -fno-use-cxa-atexit \
           -nostdlib -Wall -Wextra -O2

# Source files
SOURCES = new.cpp exception.cpp atexit.cpp syscall_stubs.cpp
OBJECTS = $(SOURCES:.cpp=.o)

# Output library
TARGET = cpp_runtime.a

# ============================================================================
# Build Rules
# ============================================================================

all: $(TARGET)

$(TARGET): $(OBJECTS)
	ar rcs $@ $^
	@echo "✅ C++ runtime library built: $(TARGET)"

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# ============================================================================
# Clean
# ============================================================================

clean:
	rm -f $(OBJECTS) $(TARGET)
	@echo "Cleaned C++ runtime"

.PHONY: all clean
