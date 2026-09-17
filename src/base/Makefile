# Gera a base keel a partir dos .k, no trio .type.h / .h / .impl.h
# O corte em tres e o do backend 4.3.2: tipos, interface, corpos.

CG    := ../../tools/codegen/codegen.out
SRC   := .
DEST  := ../../gen

# os tipos para os quais a base e instanciada
TYPES := i32 u32 char

all:
	$(CG) -m keel       -i $(SRC) -d $(DEST)
	$(CG) -m keel.arena -i $(SRC) -d $(DEST)
	@for T in $(TYPES); do \
	  $(CG) -m keel.outcome -i $(SRC) -d $(DEST) -t $$T; \
	  $(CG) -m keel.slice   -i $(SRC) -d $(DEST) -t $$T; \
	  $(CG) -m keel.outcome -i $(SRC) -d $(DEST) -t keel_slice_$$T  -a keel/keel_slice_$$T.type.h; \
	  $(CG) -m keel.buffer  -i $(SRC) -d $(DEST) -t $$T; \
	  $(CG) -m keel.outcome -i $(SRC) -d $(DEST) -t keel_buffer_$$T -a keel/keel_buffer_$$T.type.h; \
	done

clean:
	@rm -rf $(DEST)

.PHONY: all clean
