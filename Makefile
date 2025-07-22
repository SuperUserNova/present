CC = gcc
CFLAGS = $(shell pkg-config --cflags gtk4 libadwaita-1)
LIBS = $(shell pkg-config --libs gtk4 libadwaita-1) -lm

SRC = test_animations.c animations.c
OBJ = $(SRC:.c=.o)

test_animations: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f test_animations *.o

.PHONY: clean
