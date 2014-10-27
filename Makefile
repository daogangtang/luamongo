CC= g++
CFLAGS= -Wall -g -O2 -shared -fPIC -I/usr/local/include/mongo `pkg-config --cflags lua5.1` `pkg-config --cflags libmongo-client`
AR= ar rcu
RANLIB= ranlib
RM= rm -f
LIBS=`pkg-config --libs lua5.1` -lmongoclient -lssl -lboost_thread -lboost_filesystem
OUTLIB=mongo.so

LDFLAGS= $(LIBS)

OBJS = main.o mongo_bsontypes.o mongo_dbclient.o mongo_replicaset.o mongo_connection.o mongo_cursor.o mongo_gridfile.o mongo_gridfs.o mongo_gridfschunk.o mongo_query.o utils.o mongo_cxx_extension.o mongo_gridfilebuilder.o

UNAME = `uname`
PLAT = DetectOS

all: $(PLAT)

DetectOS:
	@make $(UNAME)

Linux:
	@make -f Makefile.linux

Darwin:
	@make -f Makefile.macports

clean:
	$(RM) $(OBJS) $(OUTLIB)

.PHONY: all 
