/// \file       hardware_physmem.c
/// \brief      Get the total amount of physical memory (RAM)
//  Author:     Jonathan Nieder
//  This file has been put into the public domain. You can do whatever you want with this file.
//
#include "common.h"
#pragma hdrstop
#include "tuklib_physmem.h"

 uint64 lzma_physmem(void)
{
	// It is simpler to make lzma_physmem() a wrapper for
	// tuklib_physmem() than to hack appropriate symbol visibility
	// support for the tuklib modules.
	return tuklib_physmem();
}
