###############################################################################
#
# ircbot (libircbot) Makefile
#
# Author:
#	COPYRIGHT (C) Jason Volk 2014-2016
#	COPYRIGHT (C) Svetlana Tkachenko 2014
#
# DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
#
# The production build process should work by simply typing `make`.
# No executables are produced by this process, only libraries.
#
# For developers, export IRCBOT_DEVELOPER=1 which adjusts the CCFLAGS for non-
# production debug builds.
#
###############################################################################


###############################################################################
#
# IRCBOT options
#

IRCBOT_CC       := g++
IRCBOT_VERSTR   := $(shell git describe --tags)
IRCBOT_CCFLAGS  := -std=c++14 -Istldb -DIRCBOT_VERSION=\"$(IRCBOT_VERSTR)\" -fstack-protector
IRCBOT_WFLAGS   = -pedantic                             \
                  -Wall                                 \
                  -Wextra                               \
                  -Wcomment                             \
                  -Waddress                             \
                  -Winit-self                           \
                  -Wuninitialized                       \
                  -Wunreachable-code                    \
                  -Wvolatile-register-var               \
                  -Wvariadic-macros                     \
                  -Woverloaded-virtual                  \
                  -Wpointer-arith                       \
                  -Wlogical-op                          \
                  -Wcast-align                          \
                  -Wcast-qual                           \
                  -Wstrict-aliasing=2                   \
                  -Wstrict-overflow                     \
                  -Wwrite-strings                       \
                  -Wformat-y2k                          \
                  -Wformat-security                     \
                  -Wformat-nonliteral                   \
                  -Wfloat-equal                         \
                  -Wdisabled-optimization               \
                  -Wno-missing-field-initializers       \
                  -Wmissing-format-attribute            \
                  -Wno-unused-parameter                 \
                  -Wno-unused-label                     \
                  -Wno-unused-variable                  \
                  -Wsuggest-attribute=format



###############################################################################
#
# Composition of IRCBOT options and user environment options
#


ifdef IRCBOT_DEVELOPER
    IRCBOT_CCFLAGS += -ggdb -O0
else
    IRCBOT_CCFLAGS += -DNDEBUG -D_FORTIFY_SOURCE=1 -O3
endif


IRCBOT_CCFLAGS += $(IRCBOT_WFLAGS) $(CCFLAGS)



###############################################################################
#
# Final build composition


all: libircbot.a


libircbot.a: sendq.o recvq.o bot.o
	ar rc $@ $^
	ranlib $@


libircbot.so: sendq.o recvq.o bot.o
	$(IRCBOT_CC) -o $@ $(IRCBOT_CCFLAGS) -shared $<


recvq.o: recvq.cpp *.h
	$(IRCBOT_CC) -c -o $@ $(IRCBOT_CCFLAGS) $<

sendq.o: sendq.cpp *.h
	$(IRCBOT_CC) -c -o $@ $(IRCBOT_CCFLAGS) $<

bot.o: bot.cpp *.h
	$(IRCBOT_CC) -c -o $@ $(IRCBOT_CCFLAGS) $<


clean:
	rm -f *.o *.a *.so
