CXX ?= c++
CXXFLAGS ?= -std=c++20 -Wall -Wextra -Wpedantic -Werror -fno-exceptions -fno-rtti
EXAMPLES := $(wildcard examples/*.cpp)

.PHONY: check
check:
	@set -e; tmpdir=$$(mktemp -d); tmp="$$tmpdir/layout-check"; \
	trap 'rm -f "$$tmp"; rmdir "$$tmpdir"' EXIT; \
	$(CXX) $(CXXFLAGS) -Iinclude tests/layout_check.cpp -o "$$tmp"; \
	"$$tmp"; \
	$(CXX) $(CXXFLAGS) -Iinclude tests/logic_check.cpp -o "$$tmp"; \
	"$$tmp"; \
	for source in $(EXAMPLES); do \
		$(CXX) $(CXXFLAGS) -Iinclude -fsyntax-only "$$source"; \
	done
