CC=gcc
CFLAGS=-Wall -Werror -I.
DEPS = 
ODIR=obj
SRCDIR=src
BINDIR=bin

_OBJ = main.o run_program.o process_batch.o console.o parser.o builtins.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

$(ODIR)/%.o: $(SRCDIR)/%.c $(DEPS)
	$(CC) -g -c -o $@ $< $(CFLAGS)

$(BINDIR)/shell: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

build: $(ODIR)/*.o $(BINDIR)/shell

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o $(BINDIR)/* *~ core

run: build
	./shell