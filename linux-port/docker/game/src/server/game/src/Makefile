CC = clang
CXX = clang++

GAME_VERSION := $(shell cat ../../__REVISION__)
LBITS := $(shell getconf LONG_BIT)

GccMajorVersion := $(shell expr `$(CXX) -dumpversion | cut -f1 -d.`)
GccMinorVersion := $(shell expr `$(CXX) -dumpversion | cut -f2 -d.`)
GccMinorEGT8 := $(shell expr $(GccMinorVersion) \>= 8)

INCDIR =
LIBDIR =
BINDIR = ..
OBJDIR = .obj
$(shell if [ ! -d $(OBJDIR) ]; then mkdir $(OBJDIR); fi)

## LIST OF CONSTANTS BEGIN
ENABLE_GOOGLE_TEST = 0
ENABLE_LUA_5_VERSION = 0
ENABLE_GCC_AUTODEPEND = 1
ENABLE_STATIC = 0
## LIST OF CONSTANTS END

# Depend Path File
ifneq ($(ENABLE_GCC_AUTODEPEND), 1)
DEPFILE = Depend
endif

# Standard Libraries
LIBS = -lm -lmd

# Project Flags
CFLAGS  = -m32 -g -Wall -O2 -pipe -fexceptions -fno-strict-aliasing -pthread -D_THREAD_SAFE -DNDEBUG
CFLAGS += -Wno-deprecated-declarations -Wno-nonnull-compare -Wno-deprecated-declarations -Wno-array-bounds -Wno-address
CFLAGS += -Wno-int-in-bool-context -Wno-format-truncation -Wno-stringop-truncation -Wno-sign-compare -Wno-deprecated-enum-enum-conversion
CFLAGS += -w
# The playerbot overlay is written against r40250; this tells it which
# engine it is on (playerbot_engine_compat.h).
CFLAGS += -DPLAYERBOT_ENGINE_MT2009
CXXFLAGS = -std=c++20

# for clang
ifneq '' '$(findstring clang++,$(CXX))'
CFLAGS += -Wno-invalid-source-encoding -Wno-deprecated-anon-enum-enum-conversion -Wno-unknown-warning-option
# for gcc
else ifneq '' '$(findstring g++,$(CXX))'
# for 32bit on 64bit
ifeq ($(LBITS),64)
CFLAGS += -L/usr/local/lib32/gcc12
CFLAGS += -Wl,-rpath=/usr/local/lib32/gcc12
else
# for 32bit on 32bit
CXXFLAGS += -Wl,-rpath=/usr/local/lib/gcc12
endif
endif

# MySQL
INCDIR += -I/usr/local/include/mysql
ifeq ($(LBITS),64)
LIBS += ../../../Extern/lib/libmysqlclient.a -lz
else
LIBS += /usr/local/lib/mysql/libmysqlclient.a /usr/lib/libz.a
endif

ifeq ($(ENABLE_STATIC), 1)
CFLAGS += -static
endif

ifeq ($(GccMinorEGT8), 1)
CFLAGS += -Wno-unused-local-typedefs
endif

# FreeBSD stack protector
CFLAGS += -fstack-protector-all

# Version defines
CFLAGS += -D__USER__=\"$(USER)\" -D__HOSTNAME__=\"$(HOSTNAME)\" -D__PWD__=\"$(PWD)\" -D__GAME_VERSION__=\"$(GAME_VERSION)\"

# Boost
INCDIR += -I../../../Extern/include/boost

# DevIL
INCDIR += -I../../../Extern/include/IL
LIBS += ../../../Extern/lib/libIL.a

# CryptoPP
LIBS += ../../../Extern/lib/libcryptopp.a

# GTest
ifeq ($(ENABLE_GOOGLE_TEST), 1)
LIBS += /usr/local/lib/libgtest.a
CFLAGS += -DENABLE_GOOGLE_TEST
endif

# OpenSSL
INCDIR += -I/usr/include
ifeq ($(LBITS),64)
LIBS += /usr/lib32/libcrypto.a /usr/lib32/libssl.a
else
LIBS += -lssl -lcrypto
endif

# Lua
CFLAGS += -DENABLE_LUA_5_VERSION=ENABLE_LUA_5_VERSION
ifeq ($(ENABLE_LUA_5_VERSION), 2)
INCDIR += -I../../liblua/5.2/install/include
LIBDIR += -L../../liblua/5.2/install/lib
LIBS += ../../liblua/5.2/install/lib/liblua.a
else
INCDIR += -I../../liblua/5.0/include
LIBDIR += -L../../liblua/5.0/lib
LIBS += ../../liblua/5.0/lib/liblua.a ../../liblua/5.0/lib/liblualib.a
endif

# Project Libraries
INCDIR += -I../../../Extern/include
INCDIR += -I/usr/local/include
LIBDIR += -L/usr/local/lib

LIBDIR += -L../../libthecore/lib -L../../libpoly -L../../libsql -L../../libgame/lib
LIBS += -lthecore -lpoly -lsql -lgame

# AUTO_DETECT_SOURCE_FILES BEGIN
# Automatyczne wykrycie wszystkich plików .cpp z wyłączeniem main.cpp
CPPFILE = $(filter-out main.cpp, $(wildcard *.cpp))

# Automatyczne wykrycie wszystkich plików .c
CFILE = $(wildcard *.c)

# main.cpp jest traktowany osobno
MAINCPP = main.cpp
# AUTO_DETECT_SOURCE_FILES END

# PROJECT_OBJ_FILES BEGIN
COBJS	= $(CFILE:%.c=$(OBJDIR)/%.o)
CPPOBJS	= $(CPPFILE:%.cpp=$(OBJDIR)/%.o)
ifeq ($(ENABLE_GCC_AUTODEPEND), 1)
CDEPS	= $(COBJS:%.o=%.d)
CPPDEPS	= $(CPPOBJS:%.o=%.d)
endif

MAINOBJ = $(MAINCPP:%.cpp=$(OBJDIR)/%.o)
ifeq ($(ENABLE_GCC_AUTODEPEND), 1)
MAINDEPS = $(MAINOBJ:%.o=%.d)
endif

# PROJECT_OBJ_FILES END

# Target Paths
MAIN_TARGET = $(BINDIR)/game_r$(GAME_VERSION)

default: $(MAIN_TARGET)

$(OBJDIR)/%.o: %.c
	@echo compiling $<
	@$(CC) $(CFLAGS) $(INCDIR) -c $< -o $@
ifeq ($(ENABLE_GCC_AUTODEPEND), 1)
	@$(CC) -MM -MG -MP $(CFLAGS) $(INCDIR) -c $< -o $(OBJDIR)/$*.d
	@sed -i '' -e's/$*.o:/$(OBJDIR)\/$*.o:/g' $(OBJDIR)/$*.d
endif

$(OBJDIR)/%.o: %.cpp
	@echo compiling $<
	@$(CXX) $(CFLAGS) $(CXXFLAGS) $(INCDIR) -c $< -o $@
ifeq ($(ENABLE_GCC_AUTODEPEND), 1)
	@$(CXX) -MM -MG -MP $(CFLAGS) $(CXXFLAGS) $(INCDIR) -c $< -o $(OBJDIR)/$*.d
	@sed -i '' -e's/$*.o:/$(OBJDIR)\/$*.o:/g' $(OBJDIR)/$*.d
endif

limit_time:
	@echo update limit time
	@python update_limit_time.py

$(MAIN_TARGET): $(CPPOBJS) $(COBJS) $(MAINOBJ)
	@echo linking $(MAIN_TARGET)
	@$(CXX) $(CFLAGS) $(CXXFLAGS) $(LIBDIR) $(COBJS) $(CPPOBJS) $(MAINOBJ) -rdynamic -Wl,--start-group $(LIBS) -Wl,--end-group -o $(MAIN_TARGET)

symlink:
	@ln -fs game_r$(GAME_VERSION) $(BINDIR)/game_symlink

strip:
	@cp $(MAIN_TARGET) $(BINDIR)/game_r
	@strip $(BINDIR)/game_r

clean:
	@echo cleaning $(MAIN_TARGET) $(OBJDIR)
	@rm -f $(COBJS) $(CPPOBJS) $(MAINOBJ)
ifeq ($(ENABLE_GCC_AUTODEPEND), 1)
	@rm -f $(CDEPS) $(CPPDEPS) $(MAINDEPS)
endif
	@rm -f $(BINDIR)/game_r*

dep:
ifeq ($(ENABLE_GCC_AUTODEPEND), 1)
	@echo "Note: gcc autodepend is autodetected, so target dep skipped"
else
	makedepend -f $(DEPFILE) $(INCDIR) -I/usr/include/c++/3.3 -I/usr/include/c++/4.2 -p$(OBJDIR)/ $(CPPFILE) $(CFILE) $(MAINCPP) 2> /dev/null > $(DEPFILE)
endif

# Dodatkowy target do wyświetlenia wykrytych plików (do debugowania)
show-files:
	@echo "Detected .c files:"
	@echo $(CFILE)
	@echo "Detected .cpp files (excluding main.cpp):"
	@echo $(CPPFILE)
	@echo "Main file:"
	@echo $(MAINCPP)
	@echo "Object files to be created:"
	@echo $(COBJS) $(CPPOBJS) $(MAINOBJ)

# AUTO_DEPEND_CHECK BEGIN
ifeq ($(ENABLE_GCC_AUTODEPEND), 1)
sinclude $(CDEPS)
sinclude $(CPPDEPS)
sinclude $(MAINDEPS)
else
sinclude $(DEPFILE)
endif
# AUTO_DEPEND_CHECK END