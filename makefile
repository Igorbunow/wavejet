LIBS = `pkg-config --libs gtk+-2.0 gthread-2.0` -lpng
CFLAGS = -Wall -pedantic
GFLAGS = $(CFLAGS) `pkg-config --cflags gtk+-2.0 gthread-2.0`
PREFIX = /usr/local
DEBUG = -g

OBJECTS = main.o cbk.o gui.o scp.o numbers.o png.o data.c prf.o

#ifeq ($(origin DEBUG), undefined)
#	DEBUG =
#else
#	DEBUG = -g -O0 -DDEBUG
#endif

wavejet: $(OBJECTS)
	gcc $(DEBUG) -o wavejet $(OBJECTS) $(LIBS)
install: wavejet
	strip wavejet
	cp -i wavejet $(PREFIX)/bin/wavejet
	if [ ! -d $(PREFIX)/man/man1 ]; then \
	 mkdir $(PREFIX)/man/man1; \
	fi
	cp -i wavejet.1 $(PREFIX)/man/man1/wavejet.1
uninstall:
	rm -i $(PREFIX)/bin/wavejet
	rm -i $(PREFIX)/man/man1/wavejet.1
main.o: main.c cbk.h gui.h scp.h
	gcc -c $(DEBUG) $(GFLAGS) -o $@ $<
cbk.o: cbk.c cbk.h
	gcc -c $(DEBUG) $(GFLAGS) -o $@ $<
gui.o: gui.c gui.h
	gcc -c $(DEBUG) $(GFLAGS) -o $@ $<
png.o: png.c png.h
	gcc -c $(DEBUG) $(GFLAGS) -o $@ $<
scp.o: scp.c scp.h
	gcc -c $(DEBUG) $(CFLAGS) -o $@ $<
numbers.o: numbers.c numbers.h
	gcc -c $(DEBUG) -o $@ $<
prf.o: prf.c prf.h
	gcc -c $(DEBUG) $(CFLAGS) -o $@ $<
clean:
	rm -f *.o wavejet
