CC = gcc
CFLAGS = $(shell pkg-config --cflags gtk4 libadwaita-1)
LIBS = $(shell pkg-config --libs gtk4 libadwaita-1)

present: main.c
	$(CC) $(CFLAGS) -o $@ $< $(LIBS)

clean:
	rm -f present
