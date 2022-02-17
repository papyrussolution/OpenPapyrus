// IDAllocator.h code is copyrighted (C) 2010 by Dave Brotherstone
// This file is part of Notepad++ project
// Copyright (C)2021 Don HO <don.h@free.fr>
// @license GNU GPL
//
#include <npp-internal.h>
#pragma hdrstop

IDAllocator::IDAllocator(int start, int maximumID) : _start(start), _nextID(start), _maximumID(maximumID)
{
}

int IDAllocator::allocate(int quantity)
{
	int retVal = -1;
	if(_nextID + quantity <= _maximumID && quantity > 0) {
		retVal = _nextID;
		_nextID += quantity;
	}
	return retVal;
}
