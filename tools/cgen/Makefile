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

# Procedência do binário, impressa por `--cgen-version` (cgen-tool-spec §4.1).
# Só o build_info.o a carrega, e ele é refeito a cada `make` (FORCE): o commit
# e a data nunca ficam velhos em relação ao que foi ligado. `dirty` no commit
# marca árvore com alterações não commitadas.
BUILD_COMMIT := $(shell git describe --tags --always --dirty 2>/dev/null || echo unknown)
BUILD_DATE   := $(shell date -u +%Y-%m-%dT%H:%M:%SZ)
BUILD_CC     := $(shell $(CC) --version 2>/dev/null | head -n1 | sed 's/"//g')
$(OBJDIR)/tool/build_info.o: override CFLAGS += \
  -DCGEN_BUILD_COMMIT='"$(BUILD_COMMIT)"' -DCGEN_BUILD_DATE='"$(BUILD_DATE)"' \
  -DCGEN_BUILD_CC='"$(BUILD_CC)"'
$(OBJDIR)/tool/build_info.o: FORCE
FORCE:

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

# the test suite (cgen design §10); PARSE_LEVEL=header|decl|inst|ilha (default inst)
check: all
	@sh ../../test/cgen/run.sh

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

# The transform rewrites every file it generates, and the -MMD dependencies make
# every object depend on gen/: written in place, each `make` would recompile all.
# So it generates into gen.new/ and only the files whose content differs replace
# the ones in gen/ (an unchanged file keeps its mtime).
base:
	@$(MAKE) -C $(TRANSFORM)
	@rm -rf $(GENDIR).new
	@$(MAKE) -C $(TRANSFORM)/base DEST=$(abspath $(GENDIR).new)
	@cd $(GENDIR).new && find . -type f | while read -r f; do \
	  cmp -s "$$f" "../$(notdir $(GENDIR))/$$f" || { mkdir -p "$$(dirname "../$(notdir $(GENDIR))/$$f")"; cp "$$f" "../$(notdir $(GENDIR))/$$f"; }; \
	done
	@rm -rf $(GENDIR).new

# o executável da distribuição (`make dist`, na raiz): -O2, sem -g nem -DDEBUG, em obj-release/
# para não misturar objetos com os do build de desenvolvimento
RELEASE_CFLAGS := -std=c2x -I $(SRCDIR) -I $(GENDIR) -Wall -Wextra -O2 -DNDEBUG -MMD -MP

release: base boundary
	@$(MAKE) --no-print-directory PRJ=cgen-release OBJDIR=./obj-release CFLAGS="$(RELEASE_CFLAGS)" cgen-release

cgen-release: $(objects)
	$(CC) $(objects) $(LDLIBS) -o cgen-release

# O executável de verificação (achados 2026-11-25, S1): ASan + UBSan, redzones entre as alocações da
# arena (-DCGEN_ASAN_REDZONE, memory.h) e índices checados nos buffers do bootstrap
# (-DCGEN_CHECKED_BUFFERS). `make asan` o constrói e roda a suíte do cgen com ele (CGEN_BIN); fora do
# pre-push, que roda a suíte com o executável normal. Para uma entrada só: `make cgen-asan`.
ASAN_CFLAGS := -std=c2x -I $(SRCDIR) -I $(GENDIR) -Wall -Wextra -g -O1 -DDEBUG -DCGEN_ASAN_REDZONE \
  -DCGEN_CHECKED_BUFFERS -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer -MMD -MP

cgen-asan-build: base boundary
	@$(MAKE) --no-print-directory -o base -o boundary PRJ=cgen-asan OBJDIR=./obj-asan CFLAGS="$(ASAN_CFLAGS)" cgen-asan

cgen-asan: $(objects)
	$(CC) -fsanitize=address,undefined $(objects) $(LDLIBS) -o cgen-asan

asan: cgen-asan-build
	@CGEN_BIN=$(abspath cgen-asan) sh ../../test/cgen/run.sh

.PHONY: FORCE all clean ccjson base boundary run vars release cgen-release asan cgen-asan cgen-asan-build

clean:
	@rm -rf $(OBJDIR) $(PRJ) obj-release cgen-release obj-asan cgen-asan

-include $(deps)
