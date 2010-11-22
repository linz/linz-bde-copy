###############################################################################
# Makefile for bde_copy using a GCC style platform
###############################################################################
#
# Copyright (C) 2010 LAND INFORMATION NEW ZEALAND
#
# This program is released under the terms of the license contained
# in the file LICENSE.
#
###############################################################################
# build type either debug or release
TYPE = release

CC = @gcc
AR = @ar
CP = @cp
MD = -mkdir
RM = @rm -f

ifeq ($(TYPE),debug)
LDFLAGS = -lm -lstdc++ -lz
CCPARAM = -Wall -g
MACROS =
endif

ifeq ($(TYPE), release)
LDFLAGS = -lm -lstdc++ -s -lz
CCPARAM = -O2
MACROS =
endif

SRCDIR = ..
DISTDIR = $(TYPE)/distrib
OBJ = $(TYPE)/obj
BDECOPYEXE = $(TYPE)/bde_copy

CCFLAGS := $(CCPARAM) $(foreach INC,$(INCPATH),-I$(INC)) \
	$(foreach MACRO,$(MACROS),-D$(MACRO))

OBJDIRS = \
	$(TYPE) \
	$(OBJ)

OBJS = \
	$(OBJ)/bde_copy_funcs.o    \
	$(OBJ)/bde_copy_gzip.o    \
	$(OBJ)/bde_copy_utils.o    \
	$(OBJ)/bde_copy.o

DISTFILES = \
	../bde_copy.cfg \
	../bde_copy.cfg.clean \
	../bde_copy.cfg.mssql \
	../bde_copy.cfg.pgis \
	../bde_copy.cfg.utf8 \
	../bde_copy.help

.PHONY: all clean

bdecopy_all: \
	makedirs \
	$(BDECOPYEXE)

all: bdecopy_all

$(OBJ)/%.o: $(SRCDIR)/%.cpp
	@echo Compiling $<...
	$(CC) $(CCFLAGS) -c $< -o $@

$(sort $(OBJDIRS)):
	$(MD) $@

makedirs: $(sort $(OBJDIRS))

$(BDECOPYEXE): makedirs $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) -o $(BDECOPYEXE)

$(sort $(DISTDIR)):
	$(MD) $@

dist: bdecopy_all $(sort $(DISTDIR))
	@echo Making distribution.
	$(CP) $(BDECOPYEXE) $(DISTDIR)
	$(foreach FILE,$(DISTFILES),dos2unix -n $(FILE) $(DISTDIR)/$(notdir $(FILE)); )
	@( cd $(DISTDIR); \
	tar czf bdecopy-$(TYPE).tar.gz *; )

clean:
	@echo Deleting object tree $(OBJ)...
	$(RM) -r $(OBJ)

clean_all: clean
	$(RM) -r $(TYPE)
