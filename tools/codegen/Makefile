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

# a base do bootstrap, gerada aqui mesmo em gen/ para validação própria da
# ferramenta — sem depender do resto da árvore. Compare com o /gen da raiz,
# que sai do mesmo fonte (base/) via o alvo `base` do Makefile raiz.
base:
	@$(MAKE) -C base

.PHONY: all clean base

clean:
	@rm -rf $(OBJDIR) $(PRJ)
