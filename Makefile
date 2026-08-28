CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -Werror -pedantic
EXAMPLES := $(wildcard examples/*.c)

.PHONY: check
check:
	@set -e; tmpdir=$$(mktemp -d); tmp="$$tmpdir/layout-check"; \
	trap 'rm -f "$$tmp"; rmdir "$$tmpdir"' EXIT; \
	$(CC) $(CFLAGS) -Iinclude tests/layout_check.c -o "$$tmp"; \
	"$$tmp"; \
	$(CC) $(CFLAGS) -Iinclude tests/logic_check.c -o "$$tmp"; \
	"$$tmp"; \
	for source in $(EXAMPLES); do \
		$(CC) $(CFLAGS) -Iinclude -fsyntax-only "$$source"; \
	done
