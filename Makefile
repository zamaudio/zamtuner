#!/usr/bin/make -f

PREFIX ?= /usr
LIBDIR ?= lib
LV2DIR ?= $(PREFIX)/$(LIBDIR)/lv2

#OPTIMIZATIONS ?= -msse -msse2 -mfpmath=sse -ffast-math -fomit-frame-pointer -O3 -fno-finite-math-only
OPTIMIZATIONS ?= -ffast-math -fomit-frame-pointer -O3 -fno-finite-math-only

#OPTIMIZATIONS ?=

LDFLAGS ?= -Wl,--as-needed -lfftw3f
CXXFLAGS ?= $(OPTIMIZATIONS) -Wall
CFLAGS ?= $(OPTIMIZATIONS) -Wall

###############################################################################
BUNDLE = zamtuner.lv2

CXXFLAGS += -fPIC -DPIC
CFLAGS += -fPIC -DPIC

UNAME=$(shell uname)
ifeq ($(UNAME),Darwin)
  LIB_EXT=.dylib
  LDFLAGS += -dynamiclib
else
  LDFLAGS += -shared -Wl,-Bstatic -Wl,-Bdynamic
  LIB_EXT=.so
endif


ifeq ($(shell pkg-config --exists lv2 lv2-plugin || echo no), no)
  $(error "LV2 SDK was not found")
else
  LV2FLAGS=`pkg-config --cflags --libs lv2 lv2-plugin fftw3`
endif

$(BUNDLE): manifest.ttl zamtuner.ttl zamtuner$(LIB_EXT)
#zamtuner_gui$(LIB_EXT)
	rm -rf $(BUNDLE)
	mkdir $(BUNDLE)
	cp manifest.ttl zamtuner.ttl zamtuner$(LIB_EXT) $(BUNDLE)

zamtuner$(LIB_EXT): zamtuner.c pitch_detector.c fft.c circular_buffer.c
	$(CXX) -o zamtuner$(LIB_EXT) \
		$(CXXFLAGS) \
		circular_buffer.c \
		pitch_detector.c \
		fft.c \
		zamtuner.c \
		$(LV2FLAGS) $(LDFLAGS)

install: $(BUNDLE)
	install -d $(DESTDIR)$(LV2DIR)/$(BUNDLE)
	install -t $(DESTDIR)$(LV2DIR)/$(BUNDLE) $(BUNDLE)/*

uninstall:
	rm -rf $(DESTDIR)$(LV2DIR)/$(BUNDLE)

clean:
	rm -rf $(BUNDLE) zamtuner$(LIB_EXT)

.PHONY: clean install uninstall
