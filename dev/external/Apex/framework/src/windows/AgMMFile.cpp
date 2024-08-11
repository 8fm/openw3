// This code contains NVIDIA Confidential Information and is disclosed to you
// under a form of NVIDIA software license agreement provided separately to you.
//
// Notice
// NVIDIA Corporation and its licensors retain all intellectual property and
// proprietary rights in and to this software and related documentation and
// any modifications thereto. Any use, reproduction, disclosure, or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA Corporation is strictly prohibited.
//
// ALL NVIDIA DESIGN SPECIFICATIONS, CODE ARE PROVIDED "AS IS.". NVIDIA MAKES
// NO WARRANTIES, EXPRESSED, IMPLIED, STATUTORY, OR OTHERWISE WITH RESPECT TO
// THE MATERIALS, AND EXPRESSLY DISCLAIMS ALL IMPLIED WARRANTIES OF NONINFRINGEMENT,
// MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE.
//
// Information and code furnished is believed to be accurate and reliable.
// However, NVIDIA Corporation assumes no responsibility for the consequences of use of such
// information or for any infringement of patents or other rights of third parties that may
// result from its use. No license is granted by implication or otherwise under any patent
// or patent rights of NVIDIA Corporation. Details are subject to change without notice.
// This code supersedes and replaces all information previously supplied.
// NVIDIA Corporation products are not authorized for use as critical
// components in life support devices or systems without express written approval of
// NVIDIA Corporation.
//
// Copyright (c) 2008-2013 NVIDIA Corporation. All rights reserved.
// Copyright (c) 2004-2008 AGEIA Technologies, Inc. All rights reserved.
// Copyright (c) 2001-2004 NovodeX AG. All rights reserved.  

#include "AgMMFile.h"

using namespace physx;

AgMMFile::AgMMFile():
mAddr(0), mSize(0), mFileH(0)
{}

AgMMFile::AgMMFile(char *name, unsigned int size, bool &alreadyExists)
{
	this->create(name, size, alreadyExists);
}

void AgMMFile::create(char *name, unsigned int size, bool &alreadyExists)
{
	alreadyExists = false;
	mSize = size;

	mFileH = CreateFileMapping(INVALID_HANDLE_VALUE,	// use paging file
		NULL,											// default security
		PAGE_READWRITE,									// read/write access
		0,												// buffer size (upper 32bits)
		mSize,											// buffer size (lower 32bits)
		name);											// name of mapping object
	if (mFileH == NULL || mFileH == INVALID_HANDLE_VALUE)
	{
		mSize=0;
		mAddr=0;
		return;
	}

	if (ERROR_ALREADY_EXISTS == GetLastError())
	{
		alreadyExists = true;
	}

	mAddr = MapViewOfFile(mFileH,		// handle to map object
		FILE_MAP_READ|FILE_MAP_WRITE,	// read/write permission
		0,                   
		0,                   
		mSize);

	if (mFileH == NULL || mAddr == NULL)
	{
		mSize=0;
		mAddr=0;
		return;
	}
}

void AgMMFile::destroy()
{
	if (!mAddr || !mFileH || !mSize)
		return;

	UnmapViewOfFile(mAddr);
	CloseHandle(mFileH);

	mAddr = 0;
	mFileH = 0;
	mSize = 0;
}

AgMMFile::~AgMMFile()
{
	destroy();
}
