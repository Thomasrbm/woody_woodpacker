NAME    = woody

CC      = gcc
CFLAGS  = -Wall -Wextra -Werror

SRCS    = main.c
OBJS    = $(addprefix objs/, $(SRCS:.c=.o))

all: $(NAME)

objs/%.o: %.c
	mkdir -p objs
	$(CC) $(CFLAGS) -c $< -o $@

$(NAME): $(OBJS)
	$(CC) $(CFLAGS) -o $(NAME) $(OBJS) -lm

clean:
	rm -rf objs

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re