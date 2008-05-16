# Low: a drop-in replacement for yum

CFLAGS = -Wall -Wstrict-prototypes -Wmissing-prototypes -g
LDLIBS = -g -lrpm -lcurl

all: low

low: main.o low.o 

clean:
	rm -f *.o low
