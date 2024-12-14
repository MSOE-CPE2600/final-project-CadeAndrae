# Compiler and flags
CC = gcc
CXX = g++
CFLAGS = -Wall -pthread
CXXFLAGS = -Wall -pthread `pkg-config --cflags opencv4`
LDFLAGS = -lm -ljpeg `pkg-config --libs opencv4`

# Target executable
TARGET = motion_detect

# Source files
C_SOURCES = main.c handle_motion.c image_utils.c network_utils.c
CPP_SOURCES = vid_to_jpg.cpp

# Object files
C_OBJS = $(C_SOURCES:.c=.o)
CPP_OBJS = $(CPP_SOURCES:.cpp=.o)
OBJS = $(C_OBJS) $(CPP_OBJS)

# Default target
all: $(TARGET)

# Build the target executable
$(TARGET): $(OBJS)
	$(CXX) $(CFLAGS) $(OBJS) -o $@ $(LDFLAGS)

# Compile C source files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Compile C++ source files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	rm -f $(TARGET) $(OBJS)
