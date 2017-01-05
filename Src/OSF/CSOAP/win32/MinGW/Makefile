#  mingw32-make

#---------------------------------------------------------
# LIBXML2
#---------------------------------------------------------
# This should be set as
# mingw32-make LIBXML2_DIR=<path> 
# 
#LIBXML2_DIR=../../../libxml2-2.6.23.win32

#---------------------------------------------------------
# LIBSOAP
#---------------------------------------------------------
LIBSOAP_DIR=../../libcsoap

LIBSOAP_SOURCE=\
 $(LIBSOAP_DIR)/soap-client.c \
 $(LIBSOAP_DIR)/soap-ctx.c \
 $(LIBSOAP_DIR)/soap-env.c \
 $(LIBSOAP_DIR)/soap-fault.c \
 $(LIBSOAP_DIR)/soap-router.c \
 $(LIBSOAP_DIR)/soap-server.c \
 $(LIBSOAP_DIR)/soap-service.c \
 $(LIBSOAP_DIR)/soap-xml.c

LIBSOAP_HEADER = $(addsuffix .c, $(basename $(LIBSOAP_SOURCE)))
LIBSOAP_OBJECTS = $(addsuffix .o, $(basename $(LIBSOAP_SOURCE)))

#---------------------------------------------------------
# NANOHTTP
#---------------------------------------------------------
NANOHTTP_DIR=../../nanohttp

NANOHTTP_SOURCE=\
 $(NANOHTTP_DIR)\nanohttp-base64.c \
 $(NANOHTTP_DIR)\nanohttp-client.c \
 $(NANOHTTP_DIR)\nanohttp-common.c \
 $(NANOHTTP_DIR)\nanohttp-mime.c \
 $(NANOHTTP_DIR)\nanohttp-request.c \
 $(NANOHTTP_DIR)\nanohttp-response.c \
 $(NANOHTTP_DIR)\nanohttp-server.c \
 $(NANOHTTP_DIR)\nanohttp-socket.c \
 $(NANOHTTP_DIR)\nanohttp-ssl.c \
 $(NANOHTTP_DIR)\nanohttp-stream.c
 
NANOHTTP_HEADER = $(addsuffix .h, $(basename $(NANOHTTP_SOURCE)))
NANOHTTP_OBJECTS = $(addsuffix .o, $(basename $(NANOHTTP_SOURCE)))

CC=gcc.exe
AR=ar.exe
INCLUDE=-I. -I../include -I../.. -I$(LIBXML2_DIR)/include -I$(ICONV_DIR)/include


.c.o:
	$(CC) -Wall $(INCLUDE) -DHAVE_CONFIG_H -c $? -o $(addsuffix .o, $(basename $?))

all: nanohttp libsoap simpleclient simpleserver
	echo OK

nanohttp: $(NANOHTTP_OBJECTS)
	$(AR) cru libnanohttp.a $?

libsoap: $(LIBSOAP_OBJECTS)
	$(AR) cru libsoap.a $?

simpleclient: ../../examples/csoap/simpleclient.o
	$(CC) -o simpleclient $? -L$(LIBXML2_DIR)/lib -L. -lsoap -lnanohttp -lxml2 -lws2_32

simpleserver: ../../examples/csoap/simpleserver.o
	$(CC) -o simpleserver $? -L$(LIBXML2_DIR)/lib -L. -lsoap -lnanohttp -lxml2 -lws2_32
