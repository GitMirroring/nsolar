# Makefile - targets to compile and install nsolar
# Copyright (C) 2026 Segen Stoutamire
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

.SUFFIXES: .c .x .o .scm
OUTPUT   = nsolar
VERS     = 0.0.1
CC       = c99
DEPS     = guile-3.0 raylib
CFLAGS   = -O2 -g `pkgconf --cflags $(DEPS)`
LDFLAGS  = `pkgconf --libs $(DEPS)` -lm

SCMFILES = scm/main.scm
CFILES   = src/main.c src/sim.c src/render.c
HFILES   = src/vec.h src/sim.h
OFILES   = $(CFILES:.c=.o)
XFILES   = $(CFILES:.c=.x)

prefix   = /usr/local

.c.x:
	guile-snarf -o $@ $(CFLAGS) $<

.c.o:
	$(CC) $(CFLAGS) -c -o $@ $<

all: $(OUTPUT)
$(OUTPUT): $(XFILES) $(OFILES)
	$(CC) $(CFLAGS) $(OFILES) $(LDFLAGS) -o $@

install: $(OUTPUT)
	install -d -m0755 -t ${prefix}/bin $(OUTPUT)
	install -d -m0644 -t ${prefix}/share/nsolar $(SCMFILES)

uninstall:
	rm -f ${prefix}/bin/$(OUTPUT)

clean:
	rm -f $(OFILES) $(XFILES) $(OUTPUT)

DISTDIR  = $(OUTPUT)-$(VERS)
dist:
	mkdir -p $(DISTDIR)/src $(DISTDIR)/scm
	cp $(CFILES) $(HFILES) $(DISTDIR)/src
	cp $(SCMFILES) $(DISTDIR)/scm
	cp Makefile README COPYING $(DISTDIR)
	tar cf $(DISTDIR).tar $(DISTDIR)
	rm -rf $(DISTDIR)
