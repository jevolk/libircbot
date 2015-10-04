#
#  COPYRIGHT 2014 (C) Jason Volk
#  COPYRIGHT 2014 (C) Svetlana Tkachenko
#
#  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
#

TARGET = libircbot.a

CC = g++
VERSTR = $(shell git describe --tags)
CCFLAGS += -std=c++14 -Istldb -DIRCBOT_VERSION=\"$(VERSTR)\" -fstack-protector
WFLAGS = -pedantic                             \
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


all: $(TARGET)


$(TARGET): sendq.o recvq.o bot.o
	ar rc $@ $^
	ranlib $@

recvq.o: recvq.cpp *.h
	$(CC) -c -o $@ $(CCFLAGS) $(WFLAGS) $<

sendq.o: sendq.cpp *.h
	$(CC) -c -o $@ $(CCFLAGS) $(WFLAGS) $<

bot.o: bot.cpp *.h
	$(CC) -c -o $@ $(CCFLAGS) $(WFLAGS) $<


clean:
	rm -f *.o *.a
