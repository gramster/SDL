##############################################
# Makefile for the SDL compiler/interpreter  #
# by Graham Wheeler			     #
# Data Network Architectures Laboratory	     #
# University of Cape Town		     #
# (c) 1994, All Rights Reserved		     #
##############################################

###########################################################
# Uncomment these for DOS
###########################################################

#CC=bcc
#FLAGS=-v -ml -ls
#DEFINES=
#CFLAGS=$(FLAGS) $(DEFINES) -H
#LFLAGS=$(FLAGS)
#LIBS=

# Don't change any below here

#RM=del
#CP=copy
#RENAME=ren
#ZIP=zip
#CSUF=.cpp
#OSUF=.obj
#XSUF=.exe
#SDLPP=
#SDLCC=
#SDLC1=
#SDLC2=
#SDLC3=
#SDLDC=
#SDLGR=
#SDLDIS=
#SDLCPP=
#SDLRUN=
#PRE=rse
#SUF= | tee /A sdl.err

###########################################################
# Uncomment these for UNIX
###########################################################

CC=gcc
FLAGS=-g -traditional
DEFINES=
CFLAGS=$(FLAGS) $(DEFINES) 
LFLAGS=$(FLAGS)
LIBS= -lg++

# Don't change any below here

RM=rm
CP=cp
RENAME=mv
ZIP=zip
CSUF=.cc
OSUF=.o
XSUF=
SDLPP=-o sdlpp
SDLCC=-o sdlcc
SDLC1=-o sdlc1
SDLC2=-o sdlc2
SDLC3=-o sdlc3
SDLDC=-o sdldc
SDLGR=-o sdlgr
SDLDIS=-o sdldis
SDLCPP=-o sdl2cpp
SDLRUN=-o sdlrun
PRE=
SUF= 2>&1 | tee -a sdl.err

########################################################################

OBJPP=sdlpp$(OSUF) sdlast$(OSUF)
OBJCC=sdlcc$(OSUF)
OBJ1=sdlc1$(OSUF) sdlparse$(OSUF) sdlast$(OSUF)
OBJ2=sdlc2$(OSUF) sdlast$(OSUF) sdlcheck$(OSUF) sdlemit$(OSUF) sdladdr$(OSUF) sdlcode$(OSUF)
OBJDC=sdldc$(OSUF) sdlast$(OSUF) sdlprint$(OSUF) 
OBJGR=sdlgr$(OSUF) sdlast$(OSUF)
OBJCPP=sdl2cpp$(OSUF) sdlast$(OSUF)
OBJ3=sdlc3$(OSUF) sdlparse$(OSUF) sdlast$(OSUF) sdlprint$(OSUF)
OBJDIS=sdldis$(OSUF) sdlcode$(OSUF)
OBJRUN=sdlrun$(OSUF) sdlcode$(OSUF) smachine$(OSUF) sdlast$(OSUF) sdlprint$(OSUF)

default: cleanerr $(OBJ1) $(OBJ2) $(OBJRUN) sdlcc$(XSUF) sdlpp$(XSUF) sdlc1$(XSUF) sdlc2$(XSUF) sdlrun$(XSUF) sdl2cpp$(XSUF)

all: cleanerr $(OBJ1) $(OBJ2) $(OBJDC) $(OBJ3) $(OBJRUN) sdlcc$(XSUF) sdlpp$(XSUF) sdlc1$(XSUF) sdlc2$(XSUF) sdlrun$(XSUF) sdldc$(XSUF) sdlc3$(XSUF) sdldis$(XSUF) sdl2cpp$(XSUF)

stress: sdlast$(OSUF) sdladdr$(OSUF) smachine$(OSUF) sdlprint$(OSUF)

cleanerr:
	-$(RM) sdl.err

zip:
	-$(ZIP) -u sdlcc *.cpp *.cc *.h *.inl *.sdl
	-$(ZIP) -u sdlcc makefile.dos makefile.bsd TODO change.log user.tex

dos2unix:
	-$(RENAME) todo TODO
	-$(RENAME) sdladdr.cpp sdladdr.cc
	-$(RENAME) sdlast.cpp sdlast.cc
	-$(RENAME) sdlc1.cpp sdlc1.cc
	-$(RENAME) sdlc2.cpp sdlc2.cc
	-$(RENAME) sdlcc.cpp sdlcc.cc
	-$(RENAME) sdlcheck.cpp sdlcheck.cc
	-$(RENAME) sdlcode.cpp sdlcode.cc
	-$(RENAME) sdldc.cpp sdldc.cc
	-$(RENAME) sdldis.cpp sdldis.cc
	-$(RENAME) sdlemit.cpp sdlemit.cc
	-$(RENAME) sdlparse.cpp sdlparse.cc
	-$(RENAME) sdlpp.cpp sdlpp.cc
	-$(RENAME) sdlprint.cpp sdlprint.cc
	-$(RENAME) sdlrun.cpp sdlrun.cc
	-$(RENAME) smachine.cpp smachine.cc
	-$(RENAME) sdl2cpp.cpp sdl2cpp.cc

unix2dos:
	-$(RENAME) sdladdr.cc sdladdr.cpp
	-$(RENAME) sdlast.cc sdlast.cpp
	-$(RENAME) sdlc1.cc sdlc1.cpp
	-$(RENAME) sdlc2.cc sdlc2.cpp
	-$(RENAME) sdlcc.cc sdlcc.cpp
	-$(RENAME) sdlcheck.cc sdlcheck.cpp
	-$(RENAME) sdlcode.cc sdlcode.cpp
	-$(RENAME) sdldc.cc sdldc.cpp
	-$(RENAME) sdldis.cc sdldis.cpp
	-$(RENAME) sdlemit.cc sdlemit.cpp
	-$(RENAME) sdlparse.cc sdlparse.cpp
	-$(RENAME) sdlpp.cc sdlpp.cpp
	-$(RENAME) sdlprint.cc sdlprint.cpp
	-$(RENAME) sdlrun.cc sdlrun.cpp
	-$(RENAME) smachine.cc smachine.cpp

# Driver

sdlcc$(XSUF): $(OBJCC)
	$(PRE) $(CC) $(LFLAGS) $(SDLCC) $(OBJCC) $(LIBS) $(SUF)

sdlcc$(OSUF): sdlcc$(CSUF)
	$(PRE) $(CC) $(CFLAGS) -c sdlcc$(CSUF) $(SUF)

# Preprocessor

sdlpp$(XSUF): $(OBJPP)
	$(PRE) $(CC) $(LFLAGS) $(SDLPP) $(OBJPP) $(LIBS) $(SUF)

sdlpp$(OSUF): sdlpp$(CSUF) sdlast.h rwtable.cpp
	$(PRE) $(CC) $(CFLAGS) -c sdlpp$(CSUF) $(SUF)

# Compiler - first pass

sdlc1$(XSUF): $(OBJ1)
	$(PRE) $(CC) $(LFLAGS) $(SDLC1) $(OBJ1) $(LIBS) $(SUF)

sdlc3$(XSUF): $(OBJ3)
	$(PRE) $(CC) $(LFLAGS) $(SDLC3) $(OBJ3) $(LIBS) $(SUF)

sdlc3$(OSUF): sdlc1$(OSUF)
	$(CP) sdlc1$(OSUF) sdlc3$(OSUF)

sdlc1$(OSUF): sdlc1$(CSUF) sdlast.h sdlc1.h
	$(PRE) $(CC) $(CFLAGS) -c sdlc1$(CSUF) $(SUF)

sdlparse$(OSUF): sdlparse$(CSUF) sdlast.h sdlc1.h rwtable.cpp
	$(PRE) $(CC) $(CFLAGS) -c sdlparse$(CSUF) $(SUF)

sdlast$(OSUF): sdlast$(CSUF) sdlast.h
	$(PRE) $(CC) $(CFLAGS) -c sdlast$(CSUF) $(SUF)

# Compiler - second pass

sdlc2$(XSUF): $(OBJ2)
	$(PRE) $(CC) $(LFLAGS) $(SDLC2) $(OBJ2) $(LIBS) $(SUF)

sdlc2$(OSUF): sdlc2$(CSUF) sdlast.h sdlc2.h
	$(PRE) $(CC) $(CFLAGS) -c sdlc2$(CSUF) $(SUF)

sdlcheck$(OSUF): sdlcheck$(CSUF) sdlast.h sdlc2.h
	$(PRE) $(CC) $(CFLAGS) -c sdlcheck$(CSUF) $(SUF)

sdladdr$(OSUF): sdladdr$(CSUF) sdlast.h sdlc2.h sdlcode.h
	$(PRE) $(CC) $(CFLAGS) -c sdladdr$(CSUF) $(SUF)

sdlemit$(OSUF): sdlemit$(CSUF) sdlast.h sdlc2.h sdlcode.h
	$(PRE) $(CC) $(CFLAGS) -c sdlemit$(CSUF) $(SUF)

sdlcode$(OSUF): sdlcode$(CSUF) sdlcode.h
	$(PRE) $(CC) $(CFLAGS) -c sdlcode$(CSUF) $(SUF)

# Decompiler

sdldc$(XSUF): $(OBJDC)
	$(PRE) $(CC) $(LFLAGS) $(SDLDC) $(OBJDC) $(LIBS) $(SUF)

sdldc$(OSUF): sdldc$(CSUF) sdlast.h
	$(PRE) $(CC) $(CFLAGS) -c sdldc$(CSUF) $(SUF)

sdlprint$(OSUF): sdlprint$(CSUF) sdlast.h
	$(PRE) $(CC) $(CFLAGS) -c sdlprint$(CSUF) $(SUF)

sdlast.h: sdlast.inl sdl.h
	touch sdlast.h

# Disassembler

sdldis$(XSUF): $(OBJDIS)
	$(PRE) $(CC) $(LFLAGS) $(SDLDIS) $(OBJDIS) $(LIBS) $(SUF)

sdldis$(OSUF): sdldis$(CSUF) sdlcode.h
	$(PRE) $(CC) $(CFLAGS) -c sdldis$(CSUF) $(SUF)

# C++ convertor

sdl2cpp$(XSUF): $(OBJCPP)
	$(PRE) $(CC) $(LFLAGS) $(SDLCPP) $(OBJCPP) $(LIBS) $(SUF)

sdl2cpp$(OSUF): sdl2cpp$(CSUF) sdlast.h
	$(PRE) $(CC) $(CFLAGS) -c sdl2cpp$(CSUF) $(SUF)

# Interpreter

sdlrun$(XSUF): $(OBJRUN)
	$(PRE) $(CC) $(LFLAGS) $(SDLRUN) $(OBJRUN) $(LIBS) $(SUF)

sdlrun$(OSUF): sdlrun$(CSUF) sdlcode.h smachine.h
	$(PRE) $(CC) $(CFLAGS) -c sdlrun$(CSUF) $(SUF)

smachine$(OSUF): smachine$(CSUF) sdlcode.h smachine.h
	$(PRE) $(CC) $(CFLAGS) -c smachine$(CSUF) $(SUF)

# Clean up

clean:
	-$(RM) *.sym
	-$(RM) *$(OSUF)
	-$(RM) *$(XSUF)
	-$(RM) *.map

