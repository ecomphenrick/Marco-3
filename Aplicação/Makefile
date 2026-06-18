TARGET = exe
CC = gcc
CFLAGS = -std=gnu99 -Wall -O2
LIBS = -lrt -lm

SRCS = main.c vga_driver.c driver.s
OBJS = $(patsubst %.c, %.o, $(patsubst %.s, %.o, $(SRCS)))

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(TARGET) $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.s
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean

#Use o comando no complilador (make && ./exe) se estiver no sudo su
#ou (make && sudo ./exe) caso não esteja para compilar e executar o progama