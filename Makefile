CC = gcc

CFLAGS = -Wall -Wextra -g -Iinclude
TARGET = city_manager

SRCS = src/city_manager.c \
       src/command.c \
       src/sanitization.c \
       src/cfg_logs_utils.c \
       src/cli_parser.c \
       src/file_utils.c \
       src/filter_utils.c \
       src/report_utils.c

OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
