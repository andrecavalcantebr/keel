# cgen — um executável, dois diretórios de fonte (desenho do cgen §3.1, D8)
#
#   src/tool/     depende da invocação e do sistema de arquivos
#   src/engine/   depende só do fonte: não abre arquivo, não escreve, não sai
#
# A fronteira é verificada pelo alvo `boundary`, que roda junto com `all`.

PRJ:=$(shell basename "$(CURDIR)")

SRCDIR:=./src
OBJDIR:=./obj
GENDIR:=./gen

CC := gcc
CFLAGS := -std=c2x -I $(SRCDIR) -I $(GENDIR) -Wall -Wextra -g -O0 -DDEBUG -MMD -MP
LDLIBS := -lm

sources := $(shell find $(SRCDIR) -name '*.c')
objects := $(patsubst $(SRCDIR)/%.c, $(OBJDIR)/%.o, $(sources))
deps    := $(objects:.o=.d)

all: base boundary $(PRJ)

vars:
	@echo "sources=$(sources)"
	@echo "objects=$(objects)"

# o obj/ espelha a árvore do src/, então cada .o precisa do seu diretório
$(OBJDIR)/%.o: $(SRCDIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(objects): | base

$(PRJ): $(objects)
	$(CC) $(objects) $(LDLIBS) -o $(PRJ)

run:
	./$(PRJ)

# A fronteira do motor, verificada sem compilar nada. Mesma espécie de
# invariante estrutural que o runner do golden aplica ao gerado (I1, I2), e
# pela mesma razão: regra que só se verifica lendo o código é regra que se
# perde.
FORBIDDEN := stdio\.h|stdlib\.h|unistd\.h|fcntl\.h|sys/

boundary:
	@bad=$$(grep -rnE '^[[:space:]]*#[[:space:]]*include[[:space:]]*<($(FORBIDDEN))' \
	          $(SRCDIR)/engine 2>/dev/null); \
	if [ -n "$$bad" ]; then \
	  echo "engine/ não pode fazer E/S (desenho do cgen §3.1):"; \
	  echo "$$bad" | sed 's/^/  /'; \
	  exit 1; \
	fi; \
	echo "ok     engine boundary             (desenho do cgen §3.1)"

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

.PHONY: all clean ccjson base boundary run vars

clean:
	@rm -rf $(OBJDIR) $(PRJ)

-include $(deps)
