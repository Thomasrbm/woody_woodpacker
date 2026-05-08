NAME    = woody-woodpacker

CC      = gcc
CFLAGS  = -Wall -Wextra -Werror

SRCS    = $(shell find $(SRC) -name "*.c")
OBJS    = $(addprefix objs/, $(SRCS:.c=.o))

all: $(NAME)

objs:
	mkdir -p objs

stub.h: stub.s xtea.inc | objs
	nasm -f elf64 stub.s -o objs/stub.o
	objcopy -O binary -j .text objs/stub.o objs/stub.bin
	xxd -i -n stub_bin objs/stub.bin > stub.h

objs/%.o: %.c stub.h | objs
	$(CC) $(CFLAGS) -c $< -o $@

$(NAME): $(OBJS)
	$(CC) $(CFLAGS) -o $(NAME) $(OBJS) -lm

clean:
	rm -rf objs
	rm -f stub.h

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re
