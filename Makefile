.SUFFIXES: .c .o
OUTPUT   = nsolar
VERS     = 0.0.1
CC       = cc
DEPS     = guile-3.0 raylib
CFLAGS   = -O2 -g -std=c11 `pkgconf --cflags $(DEPS)`
LDFLAGS  = `pkgconf --libs $(DEPS)`

CFILES   = src/main.c src/sim.c
OFILES   = $(CFILES:.c=.o)

.c.o:
	$(CC) $(CFLAGS) -c -o $@ $<

all: $(OUTPUT)
$(OUTPUT): $(OFILES)
	$(CC) $(CFLAGS) $(OFILES) $(LDFLAGS) -o $@

install: $(OUTPUT)
	install -m0755 $(OUTPUT) /usr/local/bin

uninstall:
	rm -f /usr/local/bin/$(OUTPUT)

clean:
	rm -f $(OFILES) $(OUTPUT)

DISTDIR  = $(OUTPUT)-$(VERS)
dist:
	mkdir -p $(DISTDIR)/src
	cp $(CFILES) $(DISTDIR)/src
	cp Makefile README $(DISTDIR)
	tar cf $(DISTDIR).tar $(DISTDIR)
	rm -rf $(DISTDIR)
