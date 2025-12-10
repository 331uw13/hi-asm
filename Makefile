FLAGS = -O2 -Wall -Wextra -Wno-switch
CC = gcc

TARGET_NAME = hi-asm

SRC  = $(shell find ./src -type f -name *.c)
OBJS = $(SRC:.c=.o)

all: $(TARGET_NAME)


%.o: %.c
	$(CC) $(FLAGS) -c $< -o $@ 

$(TARGET_NAME): $(OBJS)
	$(CC) $(OBJS) -o $@ $(FLAGS)

clean:
	rm $(OBJS) $(TARGET_NAME)


