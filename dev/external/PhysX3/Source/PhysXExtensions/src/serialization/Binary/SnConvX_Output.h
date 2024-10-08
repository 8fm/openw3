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

#ifndef PX_CONVX_OUTPUT_H
#define PX_CONVX_OUTPUT_H

namespace physx { namespace Sn {

	struct PxMetaDataEntry;
	class ConvX;
	
	typedef void	(Sn::ConvX::*ConvertCallback)	(const char* src, const PxMetaDataEntry& entry, const PxMetaDataEntry& dstEntry);

	inline_ void flip(short& v)
	{
		char* b = (char*)&v;
		char temp = b[0];
		b[0] = b[1];
		b[1] = temp;
	}

	inline_ void flip(unsigned short& v)
	{
		flip((short&)v);
	}

	inline_ void flip(int& v)
	{
		char* b = (char*)&v;

		char temp = b[0];
		b[0] = b[3];
		b[3] = temp;
		temp = b[1];
		b[1] = b[2];
		b[2] = temp;
	}

	inline_ void flip(unsigned int& v)
	{
		flip((int&)v);
	}

	inline_ void flip(long long& v)
	{
		char* b = (char*)&v;

		char temp = b[0];
		b[0] = b[7];
		b[7] = temp;
		temp = b[1];
		b[1] = b[6];
		b[6] = temp;
		temp = b[2];
		b[2] = b[5];
		b[5] = temp;
		temp = b[3];
		b[3] = b[4];
		b[4] = temp;
	}

	inline_ void flip(float& v)
	{
		flip((int&)v);
	}

	inline_ void flip(void*& v)
	{
		flip((int&)v);
	}

	inline_ void flip(const char*& v)
	{
		flip((int&)v);
	}
} }

#endif
