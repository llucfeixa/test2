PF_MK ?= armv8a_64_v2xcast
ifeq ($(PF_MK), )
	FAKEVAR := $(info  Users shall select the platform at the first execution)
	PFNO := $(shell read -p "1. x86_64, 2. armv7_hf, 3. armv8_64 (type range: 1 ~ 3): " PF_MK; echo $$PF_MK)
	ifeq ($(PFNO), 3)
		PF_MK := armv8a_64_v2xcast
	else ifeq ($(PFNO), 2)
		PF_MK := armv7a_hf_v2xcast
	else ifeq ($(PFNO), 1)
		PF_MK := x86_64_v2xcast
	else
		FAKEVAR := $(info  Wrong characters)
		exit 1
	endif
	FAKEVAR := $(shell sed -i '1c PF_MK ?= $(PF_MK)' Makefile)
endif

ifeq ($(PF_MK), armv8a_64_v2xcast)
	CC := aarch64-linux-gnu-gcc
	EXTRA_CFLAGS := -std=gnu99 -Wall -Wextra

else ifeq ($(PF_MK), armv7a_hf_v2xcast)
	CC := arm-linux-gnueabihf-gcc
	EXTRA_CFLAGS := -std=gnu99 -Wall -Wextra -march=armv7-a -mfloat-abi=hard -mfpu=vfpv3-d16 -mthumb

else ifeq ($(PF_MK), x86_64_v2xcast)
	CC := gcc
	EXTRA_CFLAGS := -std=gnu99 -Wall -Wextra -fno-strict-aliasing -pipe -m64 -msse -msse2 -D__X86__
endif

PYTHON_VERSION = 3.10
PYTHON_INCLUDE_DIR = /usr/include/python$(PYTHON_VERSION)
INC_PATH ?= $(realpath /inc)
LIB_PATH ?= $(realpath /lib)
LIBS ?= -leu_v2xcast -pthread -lrt -lm -lzmq
LDFLAGS :=-g -L$(LIB_PATH)
CFLAGS += $(EXTRA_CFLAGS) -Werror-implicit-function-declaration -g -I$(INC_PATH) -I$(PYTHON_INCLUDE_DIR)
export CFLAGS INC_PATH LIB_PATH LIBS CC

SRC = unex_lib_V2X.c
OBJ = $(SRC:.c=.o)

EXTENSION = unex_lib_V2X.so

all: $(EXTENSION)

$(EXTENSION): $(OBJ)
	$(CC) -shared -o $@ $^ $(LDFLAGS) $(LIBS)

%.o: %.c
	$(CC) -fPIC -c $< -o $@ $(CFLAGS)

clean:
	rm -f $(OBJ) $(EXTENSION)

.PHONY: all clean

reset:
	 @sed -i '1c PF_MK ?= ' Makefile;
	 @echo Configuration has been reset!
