CC = gcc
CFLAGS = -Wall -Wextra -pthread # -g for debugging
LDFLAGS = -pthread

# Source files
SERVER_SRCS = server.c admin_handler.c student_handler.c faculty_handler.c
CLIENT_SRCS = client.c

# Object files
SERVER_OBJS = $(SERVER_SRCS:.c=.o)
CLIENT_OBJS = $(CLIENT_SRCS:.c=.o)

# Executables
SERVER_EXEC = academia_server
CLIENT_EXEC = academia_client

# Default target
all: $(SERVER_EXEC) $(CLIENT_EXEC)

# Rule to build server
$(SERVER_EXEC): $(SERVER_OBJS)
	$(CC) $(CFLAGS) -o $@ $(SERVER_OBJS) $(LDFLAGS)

# Rule to build client
$(CLIENT_EXEC): $(CLIENT_OBJS)
	$(CC) $(CFLAGS) -o $@ $(CLIENT_OBJS) $(LDFLAGS)

# Generic rule for .c to .o
%.o: %.c common.h admin_handler.h student_handler.h faculty_handler.h
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up
clean:
	rm -f $(SERVER_OBJS) $(CLIENT_OBJS) $(SERVER_EXEC) $(CLIENT_EXEC) *.dat

# Phony targets
.PHONY: all clean
