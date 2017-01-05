#ifndef vDOS_INOUT_H
#define vDOS_INOUT_H

#define IO_MAX (1024)

typedef Bit8u IO_ReadHandler(Bitu port);
typedef void IO_WriteHandler(Bitu port, Bitu val);

void IO_RegisterReadHandler(Bitu port, IO_ReadHandler * handler);
void IO_RegisterWriteHandler(Bitu port, IO_WriteHandler * handler);

void IO_FreeReadHandler(Bitu port);
void IO_FreeWriteHandler(Bitu port);

void IO_WriteB(Bitu port, Bitu val);
void IO_WriteW(Bitu port, Bitu val);
void IO_WriteD(Bitu port, Bitu val);

Bit8u IO_ReadB(Bitu port);
Bit16u IO_ReadW(Bitu port);
Bit32u IO_ReadD(Bitu port);

#endif
