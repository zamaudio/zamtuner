#!/usr/bin/make -f

# these can be overridden using make variables. e.g.
#   make CFLAGS=-O2
#   make install DESTDIR=$(CURDIR)/debian/meters.lv2 PREFIX=/usr
#
OPTIMIZATIONS ?= -msse -msse2 -mfpmath=sse -ffast-math -fomit-frame-pointer -O3 -fno-finite-math-only
PREFIX ?= /usr/local
CFLAGS ?= -Wall -Wno-unused-function -g
LIBDIR ?= lib

EXTERNALUI?=yes
KXURI?=yes

override CFLAGS += -g $(OPTIMIZATIONS)
BUILDDIR=./
###############################################################################
LIB_EXT=.so

LV2DIR ?= $(PREFIX)/$(LIBDIR)/lv2
LOADLIBES=-lm -lfftw3f

LV2NAME=zamtuner
BUNDLE=zamtuner.lv2

LV2GTK1=zamtuner_gtk

###############################################################################

LV2UIREQ=
GLUICFLAGS=-I.
GTKUICFLAGS=-I.

UNAME=$(shell uname)
ifeq ($(UNAME),Darwin)
  LV2LDFLAGS=-dynamiclib
  LIB_EXT=.dylib
  UI_TYPE=ui:CocoaUI
  PUGL_SRC=robtk/pugl/pugl_osx.m
  PKG_LIBS=
  GLUILIBS=-framework Cocoa -framework OpenGL
  BUILDGTK=no
else
  LV2LDFLAGS=-Wl,-Bstatic -Wl,-Bdynamic
  LIB_EXT=.so
  UI_TYPE=ui:X11UI
  PUGL_SRC=robtk/pugl/pugl_x11.c
  PKG_LIBS=glu gl
  GLUILIBS=-lX11
  GLUICFLAGS+=`pkg-config --cflags glu`
endif

ifeq ($(EXTERNALUI), yes)
  ifeq ($(KXURI), yes)
    UI_TYPE=kx:Widget
    LV2UIREQ+=lv2:requiredFeature kx:Widget;
    override CFLAGS += -DXTERNAL_UI
  else
    LV2UIREQ+=lv2:requiredFeature ui:external;
    override CFLAGS += -DXTERNAL_UI
    UI_TYPE=ui:external
  endif
endif

ifeq ($(BUILDOPENGL)$(BUILDGTK), nono)
  $(error at least one of gtk or openGL needs to be enabled)
endif

targets=$(BUILDDIR)$(LV2NAME)$(LIB_EXT)

ifneq ($(BUILDGTK), no)
targets+=$(BUILDDIR)$(LV2GTK1)$(LIB_EXT)
endif

###############################################################################
# check for build-dependencies

ifeq ($(shell pkg-config --exists lv2 || echo no), no)
  $(error "LV2 SDK was not found")
endif

ifeq ($(shell pkg-config --exists glib-2.0 gtk+-2.0 pango cairo $(PKG_LIBS) || echo no), no)
  $(error "This plugin requires cairo, pango, openGL, glib-2.0 and gtk+-2.0")
endif

# check for LV2 idle thread
ifeq ($(shell pkg-config --atleast-version=1.4.2 lv2 && echo yes), yes)
  GLUICFLAGS+=-DHAVE_IDLE_IFACE
  GTKUICFLAGS+=-DHAVE_IDLE_IFACE
  LV2UIREQ+=lv2:requiredFeature ui:idleInterface; lv2:extensionData ui:idleInterface;
endif

override CFLAGS += -fPIC
override CFLAGS += `pkg-config --cflags lv2`

###############################################################################

IM=gui/img/
RW=robtk/
RT=$(RW)rtk/
WD=$(RW)widgets/robtk_

UIIMGS=$(IM)meter.c
GTKUICFLAGS+=`pkg-config --cflags gtk+-2.0 cairo pango`
GTKUILIBS+=`pkg-config --libs gtk+-2.0 cairo pango`

GLUICFLAGS+=`pkg-config --cflags cairo pango`
GLUILIBS+=`pkg-config --libs cairo pango pangocairo $(PKG_LIBS)`

ifeq ($(GLTHREADSYNC), yes)
  GLUICFLAGS+=-DTHREADSYNC
endif
ifeq ($(GTKRESIZEHACK), yes)
  GLUICFLAGS+=-DUSE_GTK_RESIZE_HACK
  GLUICFLAGS+=$(GTKUICFLAGS)
  GLUILIBS+=$(GTKUILIBS)
endif

DSPSRC=zamtunerdsp.cc

DSPDEPS=$(DSPSRC) pitch_detector.o fft.o circular_buffer.o

UITOOLKIT=$(WD)checkbutton.h $(WD)dial.h $(WD)label.h $(WD)pushbutton.h\
          $(WD)radiobutton.h $(WD)scale.h $(WD)separator.h $(WD)spinner.h \
          $(WD)xyplot.h

ROBGL= Makefile $(UITOOLKIT) $(RW)ui_gl.c $(PUGL_SRC) \
  $(RW)gl/common_cgl.h $(RW)gl/layout.h $(RW)gl/robwidget_gl.h $(RW)robtk.h \
	$(RT)common.h $(RT)style.h \
  $(RW)gl/xternalui.c $(RW)gl/xternalui.h

ROBGTK = Makefile $(UITOOLKIT) $(RW)ui_gtk.c \
  $(RW)gtk2/common_cgtk.h $(RW)gtk2/robwidget_gtk.h $(RW)robtk.h \
	$(RT)common.h $(RT)style.h


###############################################################################
# build target definitions
default: all

all: $(targets)

$(BUILDDIR)$(LV2NAME)$(LIB_EXT): $(DSPDEPS) Makefile
	@mkdir -p $(BUILDDIR)
	$(CXX) $(CPPFLAGS) $(CFLAGS) $(CXXFLAGS) \
	  -o $(BUILDDIR)$(LV2NAME)$(LIB_EXT) $(LV2NAME).cc $(DSPDEPS) \
	  -shared $(LV2LDFLAGS) $(LDFLAGS) $(LOADLIBES)

$(BUILDDIR)$(LV2GTK1)$(LIB_EXT): $(ROBGTK) \
	$(UIIMGS) gui/needle.c gui/meterimage.c
	@mkdir -p $(BUILDDIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -std=c99  $(GTKUICFLAGS) \
	  -DPLUGIN_SOURCE="\"gui/needle.c\"" \
	  -o $(BUILDDIR)$(LV2GTK1)$(LIB_EXT) $(RW)ui_gtk.c \
	  -shared $(LV2LDFLAGS) $(LDFLAGS) $(GTKUILIBS)

###############################################################################
# install/uninstall/clean target definitions

install: all
	install -d $(DESTDIR)$(LV2DIR)/$(BUNDLE)
	install -m755 $(targets) $(DESTDIR)$(LV2DIR)/$(BUNDLE)
	install -m644 $(BUILDDIR)manifest.ttl $(BUILDDIR)$(LV2NAME).ttl $(DESTDIR)$(LV2DIR)/$(BUNDLE)

uninstall:
	rm -f $(DESTDIR)$(LV2DIR)/$(BUNDLE)/manifest.ttl
	rm -f $(DESTDIR)$(LV2DIR)/$(BUNDLE)/$(LV2NAME).ttl
	rm -f $(DESTDIR)$(LV2DIR)/$(BUNDLE)/$(LV2NAME)$(LIB_EXT)
	rm -f $(DESTDIR)$(LV2DIR)/$(BUNDLE)/$(LV2GUI1)$(LIB_EXT)
	-rmdir $(DESTDIR)$(LV2DIR)/$(BUNDLE)

clean:
	rm -f $(BUILDDIR)$(LV2NAME)$(LIB_EXT) \
	  $(BUILDDIR)$(LV2GTK1)$(LIB_EXT) *.o  *.dSYM

distclean: clean
	rm -f

.PHONY: clean all install uninstall distclean
