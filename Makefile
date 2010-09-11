CFLAGS		:=	-g -pedantic -std=c99
LDFLAGS		:=	-L$(ROOT)${prefix}/lib -Wl,-rpath-link,$(ROOT)${prefix}/lib -Wl,--allow-shlib-undefined -L. -lz
SOURCES		:=	nvram.c misc.c
OBJECTS		:=	$(SOURCES:.c=.o)
EXECUTABLE	:=	bootie-config

.PHONY		:=	clean

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS) 
	$(CC) $(OBJECTS) $(ARCHIVES) $(LDFLAGS) -o $@

.c.o:
	$(CC) $(CFLAGS) $(LDFLAGS) -c $<

clean:
	rm -f *.o; rm -f $(EXECUTABLE)
