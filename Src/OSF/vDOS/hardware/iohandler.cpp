#include "vDos.h"
#include "inout.h"

IO_WriteHandler * io_writehandlers[IO_MAX];
IO_ReadHandler * io_readhandlers[IO_MAX];

static Bit8u IO_ReadBlocked(Bitu /*port*/)
	{
	return 0xff;
	}

static void IO_WriteBlocked(Bitu /*port*/, Bitu /*val*/)
	{
	}

void IO_RegisterReadHandler(Bitu port, IO_ReadHandler * handler)
	{
	io_readhandlers[port] = handler;
	}

void IO_RegisterWriteHandler(Bitu port, IO_WriteHandler * handler)
	{
	io_writehandlers[port] = handler;
	}

void IO_FreeReadHandler(Bitu port)
	{
	io_readhandlers[port] = IO_ReadBlocked;
	}

void IO_FreeWriteHandler(Bitu port)
	{
	io_writehandlers[port] = IO_WriteBlocked;
	}

void IO_WriteB(Bitu port, Bitu val)
	{
	if (port < IO_MAX)
		io_writehandlers[port](port, val);
	}

void IO_WriteW(Bitu port, Bitu val)
	{
	for (int i = 0; i < 2; i++)
		{
		IO_WriteB(port, val&0xff);
		port++;
		val >>= 8;
		}
	}

void IO_WriteD(Bitu port, Bitu val)
	{
	for (int i = 0; i < 4; i++)
		{
		IO_WriteB(port, val&0xff);
		port++;
		val >>= 8;
		}
	}

Bit8u IO_ReadB(Bitu port)
	{
	return port < IO_MAX ? io_readhandlers[port](port) : -1;
	}

Bit16u IO_ReadW(Bitu port)
	{
	Bit16u ret = 0;
	for (int i = 0; i < 2; i++)
		ret = (ret<<8)|IO_ReadB(port++);
	return ret;
	}

Bit32u IO_ReadD(Bitu port)
	{
	Bit32u ret = 0;
	for (int i = 0; i < 4; i++)
		ret = (ret<<8)|IO_ReadB(port++);
	return ret;
	}

void IO_Init()
	{
	for (int port = 0; port < IO_MAX; port++)
		{
		IO_FreeReadHandler(port);
		IO_FreeWriteHandler(port);
		}
	}
