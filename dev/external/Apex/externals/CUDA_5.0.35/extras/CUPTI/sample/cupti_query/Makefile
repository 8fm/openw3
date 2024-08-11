#
# Copyright 2011-2012 NVIDIA Corporation. All rights reserved
# 
INCLUDES=-I../../include

ifndef OS
 OS   := $(shell uname)
 HOST_ARCH := $(shell uname -m)
endif

ifeq ($(OS),Windows_NT)
    export PATH := $(PATH):../../libWin32:../../libx64
    LIBS= -lcuda -L ../../libWin32 -L ../../libx64 -lcupti
    OBJ = obj
else
    ifeq ($(OS), Darwin)
        export DYLD_LIBRARY_PATH := $(DYLD_LIBRARY_PATH):../../lib
        LIBS= -Xlinker -framework -Xlinker cuda -L ../../lib -lcupti
    else
        export LD_LIBRARY_PATH := $(LD_LIBRARY_PATH):../../lib:../../lib64
        LIBS= -lcuda -L ../../lib -L ../../lib64 -lcupti
    endif
    OBJ = o
endif

cupti_query: cupti_query.$(OBJ)
	nvcc -o $@ cupti_query.$(OBJ) $(LIBS)

cupti_query.$(OBJ): cupti_query.cpp
	nvcc -c $(INCLUDES) $<

run: cupti_query
	./$<

clean:
	rm -f cupti_query cupti_query.$(OBJ)
