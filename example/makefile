# http://www.cs.colby.edu/maxwell/courses/tutorials/maketutor/
CC=gcc
CFLAGS=-I.
ODIR=obj

_DEPS = cli_shell.h docopt_args.h navalfate_autogen.h
DEPS = $(patsubst %,%,$(_DEPS))

_OBJ = main.o cli_shell.o navalfate_autogen.o navalfate_impl.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

$(ODIR)/%.o: %.c $(DEPS)
	@mkdir -p $(@D)
	$(CC) -c -o $@ $< $(CFLAGS)

example: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

.PHONY: clean all autogen clean-autogen

clean:
	rm -f $(ODIR)/*.o example

autogen:
	docopt-uc navalfate navalfate.docopt

clean-autogen:
	rm -f *_autogen.*

all: clean clean-autogen autogen example
