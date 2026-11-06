# keel — a raiz do repositório.
#
#   make dist    monta dist/: a árvore relocável (cgen-tool-spec §8), com o cgen em -O2
#   make check   os dois oráculos: tools/cgen (unitários, CLI) e golden/ (test/run.sh)
#   make clean   apaga dist/ e os objetos do cgen
#
# dist/ não vai para o git (.gitignore): é gerado.
#
#   make release VERSION=x.y.z   dist/ + release/keel-x.y.z.tar.gz (+ .sha256); publicar: tools/dist/release.sh

DIST     := dist
REPO     := https://github.com/andrecavalcantebr/keel
# o ref a que os links do README para fora da dist apontam: a tag, numa release
REF      ?= master
VERSION  ?=
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
	cp tools/dist/install.sh tools/dist/uninstall.sh $(DIST)/
	cp tools/dist/README.md $(DIST)/INSTALL.md
	sed -E \
	  -e 's#\]\((keel-spec|keel-rationale|keel-c-backend|cgen-tool-spec|LICENSE(-DOCS)?(\.pt)?)\.md#](doc/\1.md#g' \
	  -e 's#\]\(base/\)#](lib/base/)#g' \
	  -e 's#\]\((golden|design|tools/cgen)/\)#]($(REPO)/tree/$(REF)/\1/)#g' \
	  README.md > $(DIST)/README.md
	cp design/keel-llm-context.md $(DIST)/
	cp $(DOCS) $(LICENSES) $(DIST)/doc/
	cp -r editors/. $(DIST)/editors/
	@printf 'version %s\n%s\ncommit %s\ndate %s\n' '$(or $(VERSION),dev)' \
	  "$$($(DIST)/bin/cgen --cgen-version)" "$$(git describe --tags --always --dirty)" \
	  "$$(date -u +%Y-%m-%dT%H:%M:%SZ)" > $(DIST)/VERSION
	@echo "ok: $(DIST)/ ($$($(DIST)/bin/cgen --cgen-version))"

release:
	@test -n "$(VERSION)" || { echo "uso: make release VERSION=x.y.z" >&2; exit 1; }
	$(MAKE) dist VERSION=$(VERSION) REF=v$(VERSION)
	mkdir -p release
	rm -rf release/keel-$(VERSION)
	cp -r $(DIST) release/keel-$(VERSION)
	tar -C release -czf release/keel-$(VERSION).tar.gz keel-$(VERSION)
	rm -rf release/keel-$(VERSION)
	cd release && sha256sum keel-$(VERSION).tar.gz > keel-$(VERSION).tar.gz.sha256
	@echo "ok: release/keel-$(VERSION).tar.gz"

check:
	sh test/run.sh

clean:
	rm -rf $(DIST)
	$(MAKE) -C tools/cgen clean

.PHONY: dist release check clean
