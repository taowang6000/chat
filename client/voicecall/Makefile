CC = gcc
CFLAGS = -O3

#alsa_cflags := $(shell pkg-config --libs alsa)
alsa_libs   := $(shell pkg-config --libs   alsa)
opus_cflags := $(shell pkg-config --cflags opus)
opus_libs   := $(shell pkg-config --libs   opus)
ogg_cflags  := $(shell pkg-config --cflags ogg)
ogg_libs    := $(shell pkg-config --libs   ogg)

CFLAGS += $(opus_cflags) $(ogg_cflags)

objects = voice.o common.o audio.o readwrite.o xtea.o oggopus.o

INCLUDEDIR=./src/include

INCDIR=-I$(INCLUDEDIR)

SUBS=$(objects:%.o=obj/%.o)

CFLAGS += $(INCDIR)

COMPILE=$(CC) $(CFLAGS) -c 

../voice: $(SUBS)
	$(CC) $(CFLAGS) -o ../voice $(SUBS) $(alsa_libs) $(opus_libs) $(ogg_libs) -lm 

obj/%.o: src/%.c
	$(COMPILE) -o $@ $<
clean:
	rm -f voice $(SUBS)

