# Builds the barter-lang kernel on the tcc-wasm host with TinyCC. S2 ports
# the kernel of ratify-lang a2322ef (from permit-lang 3d47120 and 18deea2).
# See README.md.
TCC ?= tcc
CC ?= cc
CFLAGS = -std=c99 -Wall -Werror -Isrc
CLANG_FLAGS = -std=c99 -Wall -Wextra -Wswitch-enum -Werror -fsyntax-only -Isrc
FRONT = src/front/base.c src/front/lexer.c src/front/parser.c src/front/front.c src/front/eval.c src/front/check.c src/front/lower.c src/front/json.c src/front/codes.c src/front/fmt.c
TARGET = src/wasm.c
SRC = src/main.c $(FRONT) $(TARGET)
FRONT_TOOL = test/fronttool.c $(filter-out src/front/front.c src/front/lower.c,$(FRONT))
PATH_TOOL = test/pathtool.c $(filter-out src/front/front.c src/front/lower.c,$(FRONT))
NOTRAP_TOOL = test/notraptool.c
# The crypto layer (S8b): the C oracle of test/cryptotool.c. langc does not
# link it; only build/cryptotool does. test/eip197.h holds the EIP-197
# pairing vectors of the oracle (S8c).
CRYPTO = src/crypto/u256.c src/crypto/bn254.c src/crypto/keccak.c src/crypto/poseidon.c src/crypto/pairing.c
CRYPTO_TOOL = test/cryptotool.c $(CRYPTO)
CRYPTO_DATA = test/eip197.h
HEADERS = $(wildcard src/*.h src/front/*.h src/crypto/*.h)

.PHONY: build check check-clang clean

build: build/langc build/fronttool build/pathtool build/notraptool build/cryptotool build/poseidongen build/pairinggen

build/domain.c: domain/domain.lang gen/embed.c
	mkdir -p build
	$(TCC) -run gen/embed.c domain/domain.lang build/domain.c

build/langc: $(SRC) $(HEADERS) build/domain.c
	$(TCC) $(CFLAGS) -o build/langc $(SRC) build/domain.c

build/fronttool: $(FRONT_TOOL) $(HEADERS)
	mkdir -p build
	$(TCC) $(CFLAGS) -o build/fronttool $(FRONT_TOOL)

build/pathtool: $(PATH_TOOL) $(HEADERS)
	mkdir -p build
	$(TCC) $(CFLAGS) -o build/pathtool $(PATH_TOOL)

build/notraptool: $(NOTRAP_TOOL) $(HEADERS)
	mkdir -p build
	$(TCC) $(CFLAGS) -o build/notraptool $(NOTRAP_TOOL)

build/cryptotool: $(CRYPTO_TOOL) $(CRYPTO_DATA) $(HEADERS)
	mkdir -p build
	$(TCC) $(CFLAGS) -o build/cryptotool $(CRYPTO_TOOL)

# The Poseidon constants of domain/domain.lang (S8b2, Q-S8b2-17 (a)):
# test/gate.sh compares the print with the domain text.
build/poseidongen: gen/poseidon.c $(CRYPTO) $(HEADERS)
	mkdir -p build
	$(TCC) $(CFLAGS) -o build/poseidongen gen/poseidon.c $(CRYPTO)

# The pairing constants of domain/domain.lang (S8c, Q-S8c-2 (a)):
# test/gate.sh compares the print with the domain text.
build/pairinggen: gen/pairing.c $(CRYPTO) $(HEADERS)
	mkdir -p build
	$(TCC) $(CFLAGS) -o build/pairinggen gen/pairing.c $(CRYPTO)

check-clang: build/domain.c
	$(CC) $(CLANG_FLAGS) $(SRC) build/domain.c
	$(CC) $(CLANG_FLAGS) $(FRONT_TOOL)
	$(CC) $(CLANG_FLAGS) $(PATH_TOOL)
	$(CC) $(CLANG_FLAGS) $(NOTRAP_TOOL)
	$(CC) $(CLANG_FLAGS) $(CRYPTO_TOOL)

check: build/langc build/fronttool build/pathtool build/notraptool build/cryptotool build/poseidongen build/pairinggen check-clang
	sh test/gate.sh
	sh test/review.sh
	build/fronttool
	build/pathtool

clean:
	rm -rf build
