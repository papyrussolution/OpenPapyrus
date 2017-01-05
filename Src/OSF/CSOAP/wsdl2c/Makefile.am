bin_PROGRAMS=wsdl2c

INCLUDES=$(LIBXML_CFLAGS) -I$(top_srcdir)
wsdl2cl_LDFLAG=$(LIBXML_LIBS)

xsd2c_root= $(top_srcdir)/xsd2c

xsd2c_objs=../xsd2c/util.o \
../xsd2c/formatter.o \
../xsd2c/obj.o \
../xsd2c/xsd2c.o \
../xsd2c/tr.o \
../xsd2c/Enumeration.o

wsdl2c_SOURCES=CallVar.c CallFunc.c CallList.c codewriter.c wsdl2c.c 
wsdl2c_LDFLAGS=$(wsdl2cl_LDFLAG) $(xsd2c_objs)

