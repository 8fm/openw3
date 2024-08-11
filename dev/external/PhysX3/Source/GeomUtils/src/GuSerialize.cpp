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

#include "PsIntrinsics.h"
#include "PsUtilities.h"
#include "GuSerialize.h"
using namespace physx;

PX_INLINE char	littleEndian()	{ int i = 1; return *((char*)&i);	}

void physx::saveChunk(PxI8 a, PxI8 b, PxI8 c, PxI8 d, PxOutputStream& stream)
{
	stream.write(&a, sizeof(PxI8));
	stream.write(&b, sizeof(PxI8));
	stream.write(&c, sizeof(PxI8));
	stream.write(&d, sizeof(PxI8));
}

void physx::readChunk(PxI8& a, PxI8& b, PxI8& c, PxI8& d, PxInputStream& stream)
{
	stream.read(&a, sizeof(PxI8));
	stream.read(&b, sizeof(PxI8));
	stream.read(&c, sizeof(PxI8));
	stream.read(&d, sizeof(PxI8));	
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

PxU16 physx::readWord(bool mismatch, PxInputStream& stream)
{
	PxU16 d ;
	stream.read(&d, sizeof(PxU16));
	
	if(mismatch)	d = flip(&d);
	return d;
}

PxU32 physx::readDword(bool mismatch, PxInputStream& stream)
{
	PxU32 d;
	stream.read(&d, sizeof(PxU32));
	if(mismatch)	d = flip(&d);
	return d;
}

PxF32 physx::readFloat(bool mismatch, PxInputStream& stream)
{
	PxU32 d;
	stream.read(&d, sizeof(PxU32));
	if(mismatch)	d = flip(&d);
	return PX_FR(d);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void physx::writeWord(PxU16 value, bool mismatch, PxOutputStream& stream)
{
	if(mismatch)	value = flip(&value);
	stream.write(&value, sizeof(PxU16));
}

void physx::writeDword(PxU32 value, bool mismatch, PxOutputStream& stream)
{
	if(mismatch)	value = flip(&value);
	stream.write(&value, sizeof(PxU32));
}

void physx::writeFloat(PxF32 value, bool mismatch, PxOutputStream& stream)
{
	if(mismatch)	flipForWriting(value);
	// NOTE: Do not use stream.storeFloat()! See comment in function flipForWriting()
	stream.write((void*)&value, sizeof(PxF32));
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool physx::readFloatBuffer(PxF32* dest, PxU32 nbFloats, bool mismatch, PxInputStream& stream)
{
#ifdef PX_PSP2
#pragma control %push O=0
#endif

	stream.read(dest, sizeof(PxF32)*nbFloats);
	if(mismatch)
	{
		for(PxU32 i=0;i<nbFloats;i++)
		{
			dest[i] = flip(&dest[i]);
		}
	}
	return true;
#ifdef PX_PSP2
#pragma control %pop O
#endif
}

void physx::writeWordBuffer(const PxU16* src, PxU32 nb, bool mismatch, PxOutputStream& stream)
{
	while(nb--)
	{
		PxU16 w = *src++;
		if(mismatch)	w = flip(&w);
		stream.write(&w, sizeof(PxU16));
	}
}

void physx::writeFloatBuffer(const PxF32* src, PxU32 nb, bool mismatch, PxOutputStream& stream)
{
	while(nb--)
	{
		PxF32 f = *src++;
		if(mismatch)	flipForWriting(f);
		// NOTE: Do not use stream.storeFloat()! See comment in function flipForWriting()
		stream.write((void*)&f, sizeof(PxF32));
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool physx::writeHeader(PxI8 a, PxI8 b, PxI8 c, PxI8 d, PxU32 version, bool mismatch, PxOutputStream& stream)
{
	// Store endianness
	PxI8 streamFlags = littleEndian();
	if(mismatch)	streamFlags^=1;

	// Export header
	saveChunk('N', 'X', 'S', streamFlags, stream);	// "Novodex stream" identifier
	saveChunk(a, b, c, d, stream);					// Chunk identifier
//	stream.storeDword(version);						// Version number
	writeDword(version, mismatch, stream);

	return true;
}

bool physx::readHeader(PxI8 a_, PxI8 b_, PxI8 c_, PxI8 d_, PxU32& version, bool& mismatch, PxInputStream& stream)
{
	// Import header
	PxI8 a, b, c, d;
	readChunk(a, b, c, d, stream);
	if(a!='N' || b!='X' || c!='S')
		return false;

	PxI8 fileLittleEndian = d&1;
	mismatch = fileLittleEndian!=littleEndian();

	readChunk(a, b, c, d, stream);
	if(a!=a_ || b!=b_ || c!=c_ || d!=d_)
		return false;

	version = readDword(mismatch, stream);
	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

PxU32 physx::computeMaxIndex(const PxU32* indices, PxU32 nbIndices)
{
	PxU32 maxIndex=0;
	while(nbIndices--)
	{
		PxU32 currentIndex = *indices++;
		if(currentIndex>maxIndex)	maxIndex = currentIndex;
	}
	return maxIndex;
}
PxU16 physx::computeMaxIndex(const PxU16* indices, PxU32 nbIndices)
{
	PxU16 maxIndex=0;
	while(nbIndices--)
	{
		PxU16 currentIndex = *indices++;
		if(currentIndex>maxIndex)	maxIndex = currentIndex;
	}
	return maxIndex;
}

void physx::storeIndices(PxU32 maxIndex, PxU32 nbIndices, const PxU32* indices, PxOutputStream& stream, bool platformMismatch)
{
	if(maxIndex<=0xff)
	{
		for(PxU32 i=0;i<nbIndices;i++)
		{
			PxU8 data = (PxU8)indices[i];		
			stream.write(&data, sizeof(PxU8));	
		}
	}
	else if(maxIndex<=0xffff)
	{
		for(PxU32 i=0;i<nbIndices;i++)
			writeWord(Ps::to16(indices[i]), platformMismatch, stream);
	}
	else
	{
		writeIntBuffer(indices, nbIndices, platformMismatch, stream);
	}
}

void physx::readIndices(PxU32 maxIndex, PxU32 nbIndices, PxU32* indices, PxInputStream& stream, bool platformMismatch)
{
	if(maxIndex<=0xff)
	{
		PxU8 data;
		for(PxU32 i=0;i<nbIndices;i++)
		{
			stream.read(&data, sizeof(PxU8));
			indices[i] = data;
		}
	}
	else if(maxIndex<=0xffff)
	{
		for(PxU32 i=0;i<nbIndices;i++)
			indices[i] = readWord(platformMismatch, stream);
	}
	else
	{
		readIntBuffer(indices, nbIndices, platformMismatch, stream);
	}
}
