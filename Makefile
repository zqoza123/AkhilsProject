# Particle Simulation - OpenGL
# Requires: glfw, glm (install via: brew install glfw glm)

CXX = g++
CC = gcc
CXXFLAGS = -std=c++17 -g -Iinclude
CFLAGS = -g -Iinclude
LDFLAGS = -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo -framework CoreFoundation

# Homebrew paths for glfw/glm on macOS
HOMEBREW_PREFIX := $(shell brew --prefix 2>/dev/null || echo /opt/homebrew)
INCLUDES = -I$(HOMEBREW_PREFIX)/include
LIBS = -L$(HOMEBREW_PREFIX)/lib -lglfw

TARGET = particle_sim
OBJS = src/main.o src/glad.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(OBJS) -o $(TARGET) $(LIBS) $(LDFLAGS)

src/main.o: src/main.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c src/main.cpp -o src/main.o

src/glad.o: src/glad.c
	$(CC) $(CFLAGS) $(INCLUDES) -c src/glad.c -o src/glad.o

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET) $(OBJS)

.PHONY: all run clean
