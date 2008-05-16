# Low: a drop-in replacement for yum

CFLAGS = -Wall -Wstrict-prototypes -Wmissing-prototypes -g \
		 `pkg-config --cflags glib-2.0`
LDLIBS = -g -lrpm -lcurl `pkg-config --libs glib-2.0`

all: low

low: main.o low.o low-repo-rpmdb.o low-package-rpmdb.o low-repo-sqlite.o \
	low-package-sqlite.o low-config.o

clean:
	rm -f *.o low
