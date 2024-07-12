# Variables
CC = gcc
CXX = g++
CFLAGS = -g
 
SRC = main.c
OBJ = $(SRC:.c=.o)

# Default Target: all
all: http

# Link executable
http: $(OBJ)
	$(CC) $(OBJ) -o http

# Compile src -> object
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up artifacts
clean:
	rm -f $(OBJ) http

rebuild: clean all