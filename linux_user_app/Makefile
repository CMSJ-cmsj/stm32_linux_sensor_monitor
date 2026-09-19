CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -g -D_POSIX_C_SOURCE=200809L
LDFLAGS =
TARGET = uart_host
# -Wall -Wextra：开启全部警告，规范代码
# -std=c11：使用C11标准
# -g：生成gdb调试符号
# -D_POSIX_C_SOURCE=200809L：启用POSIX.2008接口（gettimeofday、strtok_r等）

SRC = main.c uart_port.c ring_buf.c sensor_frame_parser.c console_cmd.c csv_log.c
OBJ = $(SRC:.c=.o)

all:$(TARGET)

$(TARGET):$(OBJ)
	$(CC) $(OBJ) -o $(TARGET) $(LDFLAGS)

%.o :%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)
.PHONY: all clean