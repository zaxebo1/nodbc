#LDFLAGS = -Wl,-O1 -Wl,--hash-style=gnu -Wl,--sort-common
LIBPATH = -L/usr/local/lib/neko
INCPATH = -I/usr/include/odbc++ -I/usr/local/src/Neko/neko/vm
LIBS = -lodbc++ -lneko
OBJECTS=nodbc.o
SRC=nodbc.cxx

all: nODBC.ndll
	mv nODBC.ndll ../ndll/Linux

nODBC.ndll: $(OBJECTS)
	gcc -Wfatal-errors -fPIC $(LIBPATH) $(INCPATH) $(LDFLAGS) -Wall -shared -o nODBC.ndll $(LIBS) $(OBJECTS)

nodbc.o: nodbc.cxx
	gcc -Wfatal-errors -fPIC -c $(INCPATH) -Wall -o nodbc.o nodbc.cxx

clean:
	rm *o





