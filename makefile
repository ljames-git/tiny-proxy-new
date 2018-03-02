PWD := `pwd`
SRC := $(wildcard *.c) $(wildcard *.cpp)
OBJ := $(patsubst %.cpp, %.o, $(patsubst %.c, %.o, $(SRC)))
DEP := $(patsubst %.cpp, %.d, $(patsubst %.c, %.d, $(SRC)))
GXX := g++
TARGET := tiny_proxy
CINCLUDE := -I. -Ilibcurl/include
CFLAGS := -g -c -Wall -O0 $(CINCLUDE)
MACROS := -DOPEN_LOG_INFO
LDFLAGS := 
LDLIBS := -lpthread
SUBDIRS := $(shell ls -F | grep /$$ | sed 's/\///g')
STATICLIBS := $(foreach n, $(SUBDIRS), $(n)/$(n).a)



CFLAGS += $(MACROS)

.PHONY: all libs

all: libs $(TARGET)
	
clean:  
	for i in $(SUBDIRS); do make -C $$i clean; done
	rm -f $(DEP) $(OBJ) $(TARGET)

everything:
	@make clean
	@make all

libs:
	for i in $(SUBDIRS); do make -C $$i all; done

$(TARGET): $(OBJ) $(STATICLIBS)
	$(GXX) $(LDFLAGS) $^ $(LDLIBS) -o $@

include $(DEP)

%.o: %.cpp
	$(GXX) -o $@ $(CFLAGS) $<

%.d: %.cpp
	set -e 
	rm -f $@
	$(GXX) -MM $(CINCLUDE) $(MACROS) $< | sed "s,\($*\)\.o[ :]*,\1.o $@ : ,g" > $@

%.d: %.c
	set -e 
	rm -f $@
	$(GXX) -MM $(CINCLUDE) $(MACROS) $< | sed "s,\($*\)\.o[ :]*,\1.o $@ : ,g" > $@

.PHONY: clean everything
