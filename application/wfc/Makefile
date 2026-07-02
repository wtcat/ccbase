# wfc — WatchFace Compiler (offline PC tool)
CC      ?= cc
CFLAGS  ?= -std=c11 -O2 -Wall -Wextra
CPPFLAGS = -I../../docs

wfc: wfc.c cJSON.c cJSON.h wfb_format_dep
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ wfc.c cJSON.c

# convenience: fail early if the format header moved
wfb_format_dep:
	@test -f ../../docs/wfb_format.h || { echo "missing ../../docs/wfb_format.h"; exit 1; }

.PHONY: wfb_format_dep clean
clean:
	rm -f wfc
