# Simple Makefile

PRJ:=$(shell basename "$(CURDIR)")

SRCDIR:=./src
INCDIR:=./inc
LIBDIR:=./lib
OBJDIR:=./obj
GENDIR:=./gen

CC := gcc
CFLAGS := -std=c2x -I $(INCDIR) -I $(SRCDIR) -I $(GENDIR) -Wall -Wextra -g -O0 -DDEBUG -MMD -MP
LDLIBS := -lm
LDFLAGS := -L $(OBJDIR) -L $(LIBDIR)

sources:= $(wildcard $(SRCDIR)/*.c)
objects:= $(patsubst $(SRCDIR)/%.c, $(OBJDIR)/%.o, $(sources))

all: base $(PRJ)

vars:
	@echo "PRJ=$(PRJ)"
	@echo "SRCDIR=$(SRCDIR)"
	@echo "INCDIR=$(INCDIR)"
	@echo "LIBDIR=$(LIBDIR)"
	@echo "OBJDIR=$(OBJDIR)"
	@echo "GENDIR=$(GENDIR)"
	@echo "sources=$(sources)"
	@echo "objects=$(objects)"

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(objects): | base

$(PRJ): $(objects)
	gcc $(LDFLAGS) $(objects) $(LDLIBS) -o $(PRJ)

run:
	./$(PRJ)

# clangd compilation databases (Zed, VSCode, vim); see ./gen-ccjson.sh
ccjson:
	@../../gen-ccjson.sh

# the bootstrap base: source in tools/transform/base (the $T meta-language,
# not real keel — /base, at the root, is the final target), generated here in
# gen/ by the transform. The same source also generates tools/transform/gen/,
# for the tool's own validation; see tools/transform/base/Makefile.
TRANSFORM := ../transform

base:
	@$(MAKE) -C $(TRANSFORM)
	@$(MAKE) -C $(TRANSFORM)/base DEST=$(abspath $(GENDIR))

.PHONY: all clean ccjson base

clean:
	@rm -rf $(OBJDIR) $(PRJ)
