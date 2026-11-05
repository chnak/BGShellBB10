TEMPLATE = app
TARGET = BGShellBB10

CONFIG += qt thread warn_on debug_and_release cascades

SOURCES += *.cpp
HEADERS += *.h

# Term50's pinyin engine (plain C, not matched by *.cpp)
SOURCES += cjk_ime.c
LIBS += -lbbplatform -lscreen

device {
	CONFIG(release, debug|release) {
		DESTDIR = o.le-v7
		# The QNX mkspec appends -Os at the end of the flag list, so the release
		# build optimises for size (-Os after -O2 wins).  That is fine on a
		# Cortex-A9 - smaller code, friendlier to the i-cache.  What is *not*
		# optional is -fno-strict-aliasing: the Konsole code base type-puns
		# Character/int in a few places (20 -Wstrict-aliasing warnings at -Wall)
		# and the optimiser is otherwise free to miscompile them.
		QMAKE_CFLAGS_RELEASE   += -fno-strict-aliasing
		QMAKE_CXXFLAGS_RELEASE += -fno-strict-aliasing
	}
	CONFIG(debug, debug|release) {
		DESTDIR = o.le-v7-g
	}
}

simulator {
	CONFIG(release, debug|release) {
		DESTDIR = o
	}
	CONFIG(debug, debug|release) {
		DESTDIR = o-g
	}
}

OBJECTS_DIR = $${DESTDIR}/.obj
MOC_DIR = $${DESTDIR}/.moc
RCC_DIR = $${DESTDIR}/.rcc
UI_DIR = $${DESTDIR}/.ui

suredelete.target = sureclean
suredelete.commands = $(DEL_FILE) $${MOC_DIR}/*; $(DEL_FILE) $${RCC_DIR}/*; $(DEL_FILE) $${UI_DIR}/*
suredelete.depends = distclean

QMAKE_EXTRA_TARGETS += suredelete
