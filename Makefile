NAME=bdf2h

SRC=main.c
OBJ=$(SRC:.c=.o)

CFLAGS=-Wall -Werror
LDFLAGS=

LD=$(CC)

all: $(NAME)

$(NAME): $(OBJ)
	$(LD) $(LDFLAGS) -o $(NAME) $(OBJ)

clean:
	rm -fr $(NAME) $(OBJ)

re: clean all
