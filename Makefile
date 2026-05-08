NAME    = woody-woodpacker

CC      = gcc
CFLAGS  = -Wall -Wextra -Werror

SRCS    = $(shell find $(SRC) -name "*.c")
OBJS    = $(addprefix objs/, $(SRCS:.c=.o))

all: $(NAME)

stub.h: stub.s
	nasm -f elf64 stub.s -o stub.o
	objcopy -O binary -j .text stub.o stub.bin
	xxd -i stub.bin > stub.h

objs/%.o: %.c stub.h
	mkdir -p objs
	$(CC) $(CFLAGS) -c $< -o $@

$(NAME): $(OBJS)
	$(CC) $(CFLAGS) -o $(NAME) $(OBJS) -lm

clean:
	rm -rf objs
	rm -f stub.o stub.bin stub.h

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re
