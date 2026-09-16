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

$(PRJ): $(objects)
	gcc $(LDFLAGS) $(objects) $(LDLIBS) -o $(PRJ)

run:
	./$(PRJ)

# bancos de compilação do clangd (Zed, VSCode, vim); veja ./gerar-ccjson.sh
ccjson:
	@./gerar-ccjson.sh

# a base keel, gerada dos .k de src/base para gen/
base:
	@$(MAKE) -C src/base

.PHONY: all clean ccjson base

clean:
	@rm -rf $(OBJDIR) $(PRJ)
