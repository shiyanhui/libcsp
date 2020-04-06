# Copyright (c) 2020, Yanhui Shi <lime.syh at gmail dot com>
# All rights reserved.
#
# Permission to use, copy, modify, and/or distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
# WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
# ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
# WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
# ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
# OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

PLUGIN_DIR = `$(CC) -print-file-name=plugin`
VERSION_NUMBER = `echo $(VERSION) | $(SED) -e "s/\./:/g"`
MAJOR_VERSION = `echo $(VERSION) | $(AWK) -F . '{print $$1}'`

ACLOCAL_AMFLAGS = -I m4
AM_CXXFLAGS = -I $(PLUGIN_DIR)/include
AM_CFLAGS = -fplugin=.libs/libcspplugin.so.$(MAJOR_VERSION) \
	-fplugin-arg-libcspplugin.so-building-libcsp=true

bin_PROGRAMS = cspcli
lib_LTLIBRARIES = libcspplugin.la libcsp.la

cspcli_SOURCES = plugin/cli.cpp plugin/fs.hpp plugin/namer.hpp plugin/sa.hpp

libcspplugin_la_SOURCES = \
	plugin/fs.hpp plugin/namer.hpp plugin/plugin.cpp plugin/proc.hpp plugin/sa.hpp

libcsp_la_SOURCES = \
	src/chan.h src/common.h src/cond.h src/core.h src/core.c src/corepool.h \
	src/corepool.c src/csp.h src/mem.c src/monitor.c src/mutex.h src/netpoll.h \
	src/netpoll.c src/proc.h src/proc.c src/rand.h src/rand.c src/rbq.h \
	src/rbtree.h src/runq.h src/runq.c src/sched.h src/sched.c src/timer.h \
	src/timer.c

libcspplugin_la_LDFLAGS = -version-number $(VERSION_NUMBER)
libcsp_la_LDFLAGS	= -version-number $(VERSION_NUMBER) -pthread

install-exec-hook:
	$(LN_S) -f $(DESTDIR)$(libdir)/libcspplugin.so.$(VERSION) $(PLUGIN_DIR)/libcsp.so

install-data-hook:
	rm -rf $(includedir)/libcsp $(datadir)/libcsp || true
	$(MKDIR_P) $(includedir)/libcsp $(datadir)/libcsp
	cp config.h src/chan.h src/common.h src/cond.h src/core.h src/csp.h \
		src/mutex.h src/netpoll.h src/proc.h src/rbq.h src/runq.h src/sched.h \
		src/timer.h $(includedir)/libcsp
	cp $(WORKING_DIR)/*.sf $(WORKING_DIR)/*.cg $(WORKING_DIR)/.session $(datadir)/libcsp

uninstall-local:
	rm -rf $(includedir)/libcsp
	rm -rf $(datadir)/libcsp
	rm -f $(PLUGIN_DIR)/libcsp.so

clean-local:
	rm -rf $(WORKING_DIR)/.sf $(WORKING_DIR)/*.cg $(WORKING_DIR)/.session || true
