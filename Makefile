# keel — a raiz do repositório.
#
#   make dist    monta dist/: a árvore relocável (cgen-tool-spec §8), com o cgen em -O2
#   make check   os dois oráculos: tools/cgen (unitários, CLI) e golden/ (test/run.sh)
#   make clean   apaga dist/ e os objetos do cgen
#
# dist/ não vai para o git (.gitignore): é gerado.

DIST     := dist
DOCS     := keel-spec.md keel-rationale.md keel-c-backend.md cgen-tool-spec.md
LICENSES := LICENSE.md LICENSE.pt.md LICENSE-DOCS.md LICENSE-DOCS.pt.md

dist:
	$(MAKE) -C tools/cgen release
	rm -rf $(DIST)
	mkdir -p $(DIST)/bin $(DIST)/lib/base $(DIST)/lib/src $(DIST)/doc $(DIST)/editors
	cp tools/cgen/cgen-release $(DIST)/bin/cgen
	cp -r base/. $(DIST)/lib/base/
	cp -r tools/cgen/src tools/cgen/gen $(DIST)/lib/src/
	cp tools/dist/build.sh $(DIST)/lib/src/
	cp tools/dist/README.md tools/dist/install.sh tools/dist/uninstall.sh $(DIST)/
	cp $(DOCS) $(LICENSES) $(DIST)/doc/
	cp -r editors/. $(DIST)/editors/
	@echo "ok: $(DIST)/ ($$($(DIST)/bin/cgen --cgen-version))"

check:
	sh test/run.sh

clean:
	rm -rf $(DIST)
	$(MAKE) -C tools/cgen clean

.PHONY: dist check clean
