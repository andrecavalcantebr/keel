# Generates the keel base from the .k files, as .type.h / .h pairs.
# The two-way cut is backend 4.3.2's: types; prototypes and bodies.

CG    := ../transform.out
SRC   := .
# local by default (the transform's own validation/use); the cgen Makefile
# overrides it with DEST=<absolute>/gen when calling from here, to feed tools/cgen
DEST  := ../gen

# the types the base is instantiated for
TYPES := i32 u32 char

all: parser-buffers
	$(CG) -m keel       -i $(SRC) -d $(DEST)
	$(CG) -m keel.arena -i $(SRC) -d $(DEST)
	@for T in $(TYPES); do \
	  $(CG) -m keel.outcome -i $(SRC) -d $(DEST) -t $$T; \
	  $(CG) -m keel.slice   -i $(SRC) -d $(DEST) -t $$T; \
	  $(CG) -m keel.outcome -i $(SRC) -d $(DEST) -t keel_slice_$$T  -a keel/keel_slice_$$T.type.h; \
	  $(CG) -m keel.buffer  -i $(SRC) -d $(DEST) -t $$T; \
	  $(CG) -m keel.outcome -i $(SRC) -d $(DEST) -t keel_buffer_$$T -a keel/keel_buffer_$$T.type.h; \
	done

# Parser-owned element types; declarations are shared without container dependencies.
PARSER_TYPES := KSymbol KLexeme KAstNode KInstanceUse KIsland KClosure KUnavailable
parser-buffers:
	@set -e; for T in $(PARSER_TYPES); do \
	  $(CG) -m keel.outcome -i $(SRC) -d $(DEST) -t $$T -a engine/storage_types.h; \
	  $(CG) -m keel.slice -i $(SRC) -d $(DEST) -t $$T -a engine/storage_types.h; \
	  $(CG) -m keel.buffer -i $(SRC) -d $(DEST) -t $$T -a engine/storage_types.h; \
	  $(CG) -m keel.outcome -i $(SRC) -d $(DEST) -t keel_buffer_$$T -a keel/keel_buffer_$$T.type.h; \
	done

clean:
	@rm -rf $(DEST)

.PHONY: all clean parser-buffers
