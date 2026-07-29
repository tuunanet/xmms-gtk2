srcdir ?= .
top_srcdir ?= ..
top_builddir ?= $(top_srcdir)

CPPFLAGS += -I$(top_builddir) -I$(top_builddir)/xmms -I$(top_srcdir) -I$(top_srcdir)/libxmms -I$(top_srcdir)/xmms
CFLAGS += $(shell pkg-config --cflags gtk+-2.0)
LDLIBS += $(shell pkg-config --libs gtk+-2.0)

.PHONY: check clean test-c-lint test-intl-generated-sources test-package-recipes test-plugin-linkage test-release-tools

check: test-xentry test-filebrowser test-font-load test-popup-position test-keyboard-shortcuts test-pluginenum test-outputplugin test-alsa-pcm-state test-alsa-volume test-mpg123-file-duration test-mpg123-stream-position test-c-lint test-intl-generated-sources test-package-recipes test-plugin-linkage test-release-tools
	./test-xentry
	./test-filebrowser
	./test-font-load
	./test-popup-position
	./test-keyboard-shortcuts
	./test-pluginenum
	./test-outputplugin
	./test-alsa-pcm-state
	./test-alsa-volume
	./test-mpg123-file-duration
	./test-mpg123-stream-position

test-xentry: $(srcdir)/test-xentry.c $(top_srcdir)/libxmms/xentry.c $(top_srcdir)/libxmms/xentry.h
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ $(srcdir)/test-xentry.c $(top_srcdir)/libxmms/xentry.c $(LDLIBS)

test-filebrowser: CFLAGS += -ffunction-sections -fdata-sections
test-filebrowser: LDFLAGS += -Wl,--gc-sections
test-filebrowser: $(srcdir)/test-filebrowser.c $(top_srcdir)/xmms/util.c $(top_srcdir)/xmms/util.h
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ $(srcdir)/test-filebrowser.c $(top_srcdir)/xmms/util.c $(LDLIBS)

test-font-load: CFLAGS += -ffunction-sections -fdata-sections
test-font-load: LDFLAGS += -Wl,--gc-sections
test-font-load: $(srcdir)/test-font-load.c $(top_srcdir)/xmms/util.c $(top_srcdir)/xmms/util.h
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ $(srcdir)/test-font-load.c $(top_srcdir)/xmms/util.c $(LDLIBS)

test-popup-position: CFLAGS += -ffunction-sections -fdata-sections
test-popup-position: LDFLAGS += -Wl,--gc-sections
test-popup-position: $(srcdir)/test-popup-position.c $(top_srcdir)/xmms/util.c $(top_srcdir)/xmms/util.h
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ $(srcdir)/test-popup-position.c $(top_srcdir)/xmms/util.c $(LDLIBS)

test-keyboard-shortcuts: CFLAGS += -DXMMS_BINARY=\"$(top_builddir)/xmms/xmms\"
test-keyboard-shortcuts: $(srcdir)/test-keyboard-shortcuts.c $(top_builddir)/xmms/xmms
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ $(srcdir)/test-keyboard-shortcuts.c $(LDLIBS) -lX11

test-pluginenum: CFLAGS += -ffunction-sections -fdata-sections -DBUILD_PLUGIN_DIR=\"$(CURDIR)/test-plugins\" -DPLUGIN_DIR=\"/nonexistent\" -DPLUGINSUBS='"Output","Input"'
test-pluginenum: LDFLAGS += -Wl,--gc-sections
test-pluginenum: $(srcdir)/test-pluginenum.c $(top_srcdir)/xmms/pluginenum.c $(top_srcdir)/xmms/pluginenum.h test-plugins/Input/fixture/.libs/libfixture-input.so test-plugins/Output/fixture/.libs/libfixture-output.so
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ $(srcdir)/test-pluginenum.c $(top_srcdir)/xmms/pluginenum.c $(LDLIBS) -ldl

test-plugins/Input/fixture/.libs/libfixture-input.so: $(srcdir)/fixture-input-plugin.c
	mkdir -p test-plugins/Input/fixture/.libs
	$(CC) $(CPPFLAGS) $(CFLAGS) -fPIC -shared -o $@ $<

test-plugins/Output/fixture/.libs/libfixture-output.so: $(srcdir)/fixture-output-plugin.c
	mkdir -p test-plugins/Output/fixture/.libs
	$(CC) $(CPPFLAGS) $(CFLAGS) -fPIC -shared -o $@ $<

test-outputplugin: CFLAGS += -DBUILD_PLUGIN_DIR=\"$(CURDIR)/test-outputplugin-plugins\" -DPLUGIN_DIR=\"/nonexistent\"
test-outputplugin: $(srcdir)/test-outputplugin.c $(top_srcdir)/xmms/outputplugin.c $(top_srcdir)/xmms/outputplugin.h test-outputplugin-plugins/Output/alsa/.libs/libALSA.so
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ $(srcdir)/test-outputplugin.c $(top_srcdir)/xmms/outputplugin.c $(LDLIBS)

test-outputplugin-plugins/Output/alsa/.libs/libALSA.so:
	mkdir -p test-outputplugin-plugins/Output/alsa/.libs
	touch $@

test-alsa-pcm-state: $(srcdir)/test-alsa-pcm-state.c $(top_srcdir)/Output/alsa/pcm-state.c $(top_srcdir)/Output/alsa/pcm-state.h
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ $(srcdir)/test-alsa-pcm-state.c $(top_srcdir)/Output/alsa/pcm-state.c $(LDLIBS)

test-alsa-volume: CFLAGS += -ffunction-sections -fdata-sections $(shell pkg-config --cflags alsa)
test-alsa-volume: LDFLAGS += -Wl,--gc-sections
test-alsa-volume: $(srcdir)/test-alsa-volume.c $(top_srcdir)/Output/alsa/audio.c $(top_srcdir)/Output/alsa/alsa.h
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ $(srcdir)/test-alsa-volume.c $(top_srcdir)/Output/alsa/audio.c $(LDLIBS) $(shell pkg-config --libs alsa) -lm -lpthread

test-mpg123-file-duration: CFLAGS += -ffunction-sections -fdata-sections
test-mpg123-file-duration: LDFLAGS += -Wl,--gc-sections
test-mpg123-file-duration: $(srcdir)/test-mpg123-file-duration.c $(top_srcdir)/Input/mpg123/mpg123.c $(top_srcdir)/Input/mpg123/mpg123.h $(top_srcdir)/Input/mpg123/common.c $(top_srcdir)/Input/mpg123/dxhead.c $(top_srcdir)/Input/mpg123/dxhead.h
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ $(srcdir)/test-mpg123-file-duration.c $(top_srcdir)/Input/mpg123/mpg123.c $(top_srcdir)/Input/mpg123/common.c $(top_srcdir)/Input/mpg123/dxhead.c $(LDLIBS) -lm -lpthread

test-mpg123-stream-position: $(srcdir)/test-mpg123-stream-position.c $(top_srcdir)/Input/mpg123/stream-position.c $(top_srcdir)/Input/mpg123/stream-position.h
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ $(srcdir)/test-mpg123-stream-position.c $(top_srcdir)/Input/mpg123/stream-position.c $(LDLIBS)

test-c-lint:
	$(srcdir)/test-c-lint.sh $(top_srcdir)

test-intl-generated-sources:
	$(srcdir)/test-intl-generated-sources.sh $(top_srcdir)

test-package-recipes:
	$(srcdir)/test-package-recipes.sh $(top_srcdir)

test-plugin-linkage:
	$(srcdir)/test-plugin-linkage.sh $(top_builddir)

test-release-tools:
	$(srcdir)/test-release-tools.sh $(top_srcdir)

clean:
	rm -rf test-xentry test-filebrowser test-font-load test-popup-position test-keyboard-shortcuts test-pluginenum test-outputplugin test-alsa-pcm-state test-alsa-volume test-mpg123-file-duration test-mpg123-stream-position test-plugins test-outputplugin-plugins
