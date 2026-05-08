NAME    = woody_woodpacker

CC      = gcc
CFLAGS  = -Wall -Wextra -Werror -Iincludes -Iobjs

SRC_DIR     = srcs
INC_DIR     = includes
STUB_DIR    = stub
OBJ_DIR     = objs

SRCS    = $(shell find $(SRC_DIR) -name "*.c")
OBJS    = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))

STUB_HDRS = $(OBJ_DIR)/stub.h $(OBJ_DIR)/stub32.h

all: $(NAME)

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(OBJ_DIR)/stub.h: $(STUB_DIR)/stub.s $(STUB_DIR)/xtea.inc | $(OBJ_DIR)
	nasm -f elf64 -I$(STUB_DIR)/ $(STUB_DIR)/stub.s -o $(OBJ_DIR)/stub.o
	objcopy -O binary -j .text $(OBJ_DIR)/stub.o $(OBJ_DIR)/stub.bin
	xxd -i -n stub_bin $(OBJ_DIR)/stub.bin > $@

$(OBJ_DIR)/stub32.h: $(STUB_DIR)/stub32.s $(STUB_DIR)/xtea32.inc | $(OBJ_DIR)
	nasm -f elf32 -I$(STUB_DIR)/ $(STUB_DIR)/stub32.s -o $(OBJ_DIR)/stub32.o
	objcopy -O binary -j .text $(OBJ_DIR)/stub32.o $(OBJ_DIR)/stub32.bin
	xxd -i -n stub32_bin $(OBJ_DIR)/stub32.bin > $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(STUB_HDRS) | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(NAME): $(OBJS)
	$(CC) $(CFLAGS) -o $(NAME) $(OBJS)

clean:
	rm -rf $(OBJ_DIR)

fclean: clean
	rm -f $(NAME) woody

re: fclean all

.PHONY: all clean fclean re
