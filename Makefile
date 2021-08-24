#CQ_SRC = src/circular_queue.c src/a.c
CQ_SRC = src/circular_queue.c src/demo.c
CQ_INC = include/circular_queue.h
CQ_INC_DIR = include
CQ_OBJ = $(CQ_SRC:%c=%o)


UTILS_SRC = src/circular_queue_utils.c
UTILS_INC = include/circular_queue.h
UTILS_INC_DIR = include
UTILS_OBJ = $(UTILS_SRC:%c=%o)

CQ_LIBNAME = libcircqueue
UTILS_LIBNAME = libcircqueue_utils
CQ_TEST = circular_queue_test

CQ_TEST_SRC = src/circular_queue.c src/test.c

LDLIBS = -lm -pthread

COMPILER_DIR = out

LIBVERSION = 0.0.1
CQ_SOVERSION = 1
UTILS_SOVERSION = 1

CQ_SO_LDFLAG=-Wl,-soname=$(CQ_LIBNAME).so.$(CQ_SOVERSION)
UTILS_SO_LDFLAG=-Wl,-soname=$(UTILS_LIBNAME).so.$(UTILS_SOVERSION)

PREFIX ?= /usr/local
INCLUDE_PATH ?= include/circular_queue
LIBRARY_PATH ?= lib

INSTALL_INCLUDE_PATH = $(DESTDIR)$(PREFIX)/$(INCLUDE_PATH)
INSTALL_LIBRARY_PATH = $(DESTDIR)$(PREFIX)/$(LIBRARY_PATH)

INSTALL ?= cp -a

# validate gcc version for use fstack-protector-strong
MIN_GCC_VERSION = "4.9"
GCC_VERSION := "`$(CC) -dumpversion`"
IS_GCC_ABOVE_MIN_VERSION := $(shell expr "$(GCC_VERSION)" ">=" "$(MIN_GCC_VERSION)")
ifeq "$(IS_GCC_ABOVE_MIN_VERSION)" "1"
    CFLAGS += -fstack-protector-strong
else
    CFLAGS += -fstack-protector
endif

# -std=c99 -pedantic

R_CFLAGS = -fPIC -Wall -Werror -Wstrict-prototypes -Wwrite-strings -Wshadow -Winit-self -Wcast-align -Wformat=2 -Wmissing-prototypes -Wstrict-overflow=2 -Wcast-qual -Wc++-compat -Wundef -Wswitch-default -Wconversion -Os $(CFLAGS)

uname := $(shell sh -c 'uname -s 2>/dev/null || echo false')

#library file extensions
SHARED = so
STATIC = a

## create dynamic (shared) library on Darwin (base OS for MacOSX and IOS)
ifeq (Darwin, $(uname))
	SHARED = dylib
	CQ_SO_LDFLAG = ""
	UTILS_SO_LDFLAG = ""
endif

#circular_queue library names
CQ_SHARED = $(CQ_LIBNAME).$(SHARED)
CQ_SHARED_VERSION = $(CQ_LIBNAME).$(SHARED).$(LIBVERSION)
CQ_SHARED_SO = $(CQ_LIBNAME).$(SHARED).$(CQ_SOVERSION)
CQ_STATIC = $(CQ_LIBNAME).$(STATIC)

#circular_queue_utils library names
UTILS_SHARED = $(UTILS_LIBNAME).$(SHARED)
UTILS_SHARED_VERSION = $(UTILS_LIBNAME).$(SHARED).$(LIBVERSION)
UTILS_SHARED_SO = $(UTILS_LIBNAME).$(SHARED).$(UTILS_SOVERSION)
UTILS_STATIC = $(UTILS_LIBNAME).$(STATIC)

SHARED_CMD = $(CC) -shared -o

.PHONY: all shared static tests clean install

all: create_dir shared static tests
	$(warning "abcd")
create_dir: 
	mkdir -p $(COMPILER_DIR)
shared: $(CQ_SHARED) $(UTILS_SHARED)
	$(warning "abcd")

static: $(CQ_STATIC) $(UTILS_STATIC)

tests: $(CQ_TEST)

test: tests
	./$(CQ_TEST)

.c.o:
	$(warning "abcd" $^)
	$(warning "abcd" $@)
	$(warning "abcd" $<)
	$(CC) -c $(R_CFLAGS) $< -o $(<:%c=%o) -I./$(CQ_INC_DIR)

#tests
#circular_queue
$(CQ_TEST): $(CQ_TEST_SRC) $(CQ_INC)
	$(CC) $(R_CFLAGS) $(CQ_TEST_SRC)  -o $(COMPILER_DIR)/$@ $(LDLIBS) -I./$(CQ_INC_DIR)

#static libraries
#circular_queue
$(CQ_STATIC): $(CQ_OBJ)
	$(AR) rcs $(COMPILER_DIR)/$@ $<
#circular_queue_utils
$(UTILS_STATIC): $(UTILS_OBJ)
	$(AR) rcs $(COMPILER_DIR)/$@ $<
#shared libraries .so.1.0.0
#circular_queue
$(CQ_SHARED_VERSION): $(CQ_OBJ)
	$(warning "abcd")
	echo "abcd" $<
	$(CC) -shared -o $(COMPILER_DIR)/$@ $^ $(CQ_SO_LDFLAG) $(LDFLAGS)
#circular_queue_utils
$(UTILS_SHARED_VERSION): $(UTILS_OBJ)
	$(CC) -shared -o $(COMPILER_DIR)/$@ $^ $(UTILS_SO_LDFLAG) $(LDFLAGS)

#objects
#circular_queue
$(CQ_SRC): $(CQ_INC)
$(CQ_OBJ): $(CQ_SRC)
$(warning "abcd")
$(warning $(CQ_OBJ))
#circular_queue_utils
$(UTILS_OBJ): $(UTILS_SRC) $(UTILS_INC)
$(warning "abcd")

#links .so -> .so.1 -> .so.1.0.0
#circular_queue
$(CQ_SHARED_SO): $(CQ_SHARED_VERSION)
	$(warning "abcd")
	cd $(COMPILER_DIR) && pwd && ln -s $(CQ_SHARED_VERSION) $(CQ_SHARED_SO)
$(CQ_SHARED): $(CQ_SHARED_SO)
	cd $(COMPILER_DIR) && ln -s $(CQ_SHARED_SO) $(CQ_SHARED)	
#circular_queue_utils
$(UTILS_SHARED_SO): $(UTILS_SHARED_VERSION)
	cd $(COMPILER_DIR) && ln -s $(UTILS_SHARED_VERSION) $(UTILS_SHARED_SO)
$(UTILS_SHARED): $(UTILS_SHARED_SO)
	cd $(COMPILER_DIR) && ln -s $(UTILS_SHARED_SO) $(UTILS_SHARED)
#install
#circular_queue
install-circular_queue:
	mkdir -p $(INSTALL_LIBRARY_PATH) $(INSTALL_INCLUDE_PATH)
	$(INSTALL) $(CQ_INC) $(INSTALL_INCLUDE_PATH)
	$(INSTALL) $(COMPILER_DIR)/$(CQ_SHARED) $(COMPILER_DIR)/$(CQ_SHARED_SO) $(COMPILER_DIR)/$(CQ_SHARED_VERSION) $(INSTALL_LIBRARY_PATH)
#circular_queue_utils
install-utils: install-circular_queue
	$(INSTALL) $(UTILS_INC) $(INSTALL_INCLUDE_PATH)
	$(INSTALL) $(COMPILER_DIR)/$(UTILS_SHARED) $(COMPILER_DIR)/$(UTILS_SHARED_SO) $(COMPILER_DIR)/$(UTILS_SHARED_VERSION) $(INSTALL_LIBRARY_PATH)

install: install-circular_queue install-utils

#uninstall
#circular_queue
uninstall-circular_queue: uninstall-utils
	$(RM) $(INSTALL_LIBRARY_PATH)/$(CQ_SHARED)
	$(RM) $(INSTALL_LIBRARY_PATH)/$(CQ_SHARED_VERSION)
	$(RM) $(INSTALL_LIBRARY_PATH)/$(CQ_SHARED_SO)
	rmdir $(INSTALL_LIBRARY_PATH)
	$(RM) $(INSTALL_INCLUDE_PATH)/circular_queue.h
	rmdir $(INSTALL_INCLUDE_PATH)
#circular_queue_utils
uninstall-utils:
	$(RM) $(INSTALL_LIBRARY_PATH)/$(UTILS_SHARED)
	$(RM) $(INSTALL_LIBRARY_PATH)/$(UTILS_SHARED_VERSION)
	$(RM) $(INSTALL_LIBRARY_PATH)/$(UTILS_SHARED_SO)
	$(RM) $(INSTALL_INCLUDE_PATH)/circular_queue_utils.h

uninstall: uninstall-utils uninstall-circular_queue

clean:
	$(RM) $(CQ_OBJ) $(UTILS_OBJ) #delete object files
	$(RM) $(COMPILER_DIR)/$(CQ_SHARED) $(COMPILER_DIR)/$(CQ_SHARED_VERSION) $(COMPILER_DIR)/$(CQ_SHARED_SO) $(COMPILER_DIR)/$(CQ_STATIC) #delete circular_queue
	$(RM) $(COMPILER_DIR)/$(UTILS_SHARED) $(COMPILER_DIR)/$(UTILS_SHARED_VERSION) $(COMPILER_DIR)/$(UTILS_SHARED_SO) $(COMPILER_DIR)/$(UTILS_STATIC) #delete circular_queue_utils
	$(RM) $(COMPILER_DIR)/$(CQ_TEST)  #delete test
