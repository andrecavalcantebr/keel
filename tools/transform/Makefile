# Simple Makefile

PRJ:=$(shell basename "$(CURDIR)").out

SRCDIR:=./src
INCDIR:=./inc
LIBDIR:=./lib
OBJDIR:=./obj

CC := gcc
CFLAGS := -std=c2x -I $(INCDIR) -I $(SRCDIR) -Wall -Wextra -g -O0 -DDEBUG -MMD -MP
LDLIBS := -lm
LDFLAGS := -L $(OBJDIR) -L $(LIBDIR)

sources:= $(wildcard $(SRCDIR)/*.c)
objects:= $(patsubst $(SRCDIR)/%.c, $(OBJDIR)/%.o, $(sources))

all: $(PRJ)

vars:
	@echo "PRJ=$(PRJ)"
	@echo "SRCDIR=$(SRCDIR)"
	@echo "INCDIR=$(INCDIR)"
	@echo "LIBDIR=$(LIBDIR)"
	@echo "OBJDIR=$(OBJDIR)"
	@echo "sources=$(sources)"
	@echo "objects=$(objects)"

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(PRJ): $(objects)
	gcc $(LDFLAGS) $(objects) $(LDLIBS) -o $(PRJ)

run:
	./$(PRJ)

# the bootstrap base, generated right here in gen/ for the tool's own
# validation — independent of the rest of the tree. Compare with
# tools/cgen/gen, built from the same source (base/) by the cgen Makefile's
# `base` target.
base:
	@$(MAKE) -C base

.PHONY: all clean base

clean:
	@rm -rf $(OBJDIR) $(PRJ)
