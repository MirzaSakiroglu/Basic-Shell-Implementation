# Derleyici ve temel parametreler
CC = gcc
CFLAGS = -Wall -Wextra -g `pkg-config --cflags gtk+-3.0`
LDFLAGS = `pkg-config --libs gtk+-3.0`
INCLUDES = -Iinclude

# Kaynak ve hedef dosyalar
SRCS = src/main.c src/model.c src/controller.c src/view.c
OBJS = $(SRCS:.c=.o)
TARGET = multi_user_shell

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)