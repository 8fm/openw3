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
        LIBS= -lcuda -L ../../lib -L ../../lib64 -lcupti -lpthread
    endif
    OBJ = o
endif

event_sampling: event_sampling.$(OBJ)
	nvcc -o $@ event_sampling.$(OBJ) $(LIBS)

event_sampling.$(OBJ): event_sampling.cu
	nvcc -c $(INCLUDES) $<

run: event_sampling
	./$<

clean:
	rm -f event_sampling event_sampling.$(OBJ)
