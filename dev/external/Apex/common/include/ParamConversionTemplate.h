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

#ifndef PARAM_CONVERSION_TEMPLATE_H
#define PARAM_CONVERSION_TEMPLATE_H

#include <NxTraitsInternal.h>

namespace physx
{
namespace apex
{
/**
\brief Class to handle all the redundant part of version upgrades.

It verifies class names and versions, and it runs the default converter. The user may overload
convert(), getPreferredVersions() and release() methods.
*/
template<typename Told, typename Tnew, PxU32 oldVersion, PxU32 newVersion>
class ParamConversionTemplate : public NxParameterized::Conversion
{
public:

	bool operator()(NxParameterized::Interface& legacyObj, NxParameterized::Interface& obj)
	{
		if (!mDefaultConversion)
		{
			mDefaultConversion = NxParameterized::internalCreateDefaultConversion(mTraits, getPreferredVersions());
		}

		// verify class names
		if (strcmp(legacyObj.className(), Told::staticClassName()) != 0)
		{
			return false;
		}
		if (strcmp(obj.className(), Tnew::staticClassName()) != 0)
		{
			return false;
		}

		// verify version
		if (legacyObj.version() != oldVersion)
		{
			return false;
		}
		if (obj.version() != newVersion)
		{
			return false;
		}

		//Copy unchanged fields
		if (!(*mDefaultConversion)(legacyObj, obj))
		{
			return false;
		}

		mLegacyData = static_cast<Told*>(&legacyObj);
		mNewData = static_cast<Tnew*>(&obj);

		if (!convert())
		{
			return false;
		}

		NxParameterized::Handle invalidHandle(mNewData);
		if (!mNewData->areParamsOK(&invalidHandle, 1))
		{
			if (invalidHandle.isValid())
			{
				char buf[256];
				physx::string::strcpy_s(buf, 256, "First invalid item: ");
				invalidHandle.getLongName(buf + 20, 256 - 20);

				mTraits->traitsWarn(buf);
			}
			return false;
		}

		return true;
	}

	/// User code, frees itself with the traits, and also calls destroy() on the ParamConversionTemplate object
	virtual void release()
	{
		destroy();
		mTraits->free(this);
	}

protected:
	ParamConversionTemplate(NxParameterized::Traits* traits)
		: mTraits(traits), mDefaultConversion(0), mLegacyData(0), mNewData(0)
	{
		// Virtual method getPreferredVersions() can not be called in constructors
		// so we defer construction of mDefaultConversion
	}

	/// User code, return list of preferred versions.
	virtual const NxParameterized::PrefVer* getPreferredVersions() const
	{
		return 0;
	}

	/// User code, return true if conversion is successful.
	virtual bool convert()
	{
		return true;
	}

	void destroy()
	{
		if (mDefaultConversion)
		{
			mDefaultConversion->release();
		}
	}


	NxParameterized::Traits*			mTraits;
	NxParameterized::Conversion*		mDefaultConversion;

	Told*							mLegacyData;
	Tnew*							mNewData;
};
}
}

// Force inclusion of files with initParamRef before we redefine it below
#include "NxParameterized.h"
#include "NxParameters.h"
#include "NxParamUtils.h"

// Do not call initParamRef in converter - prefer Traits::createNxParameterized with explicit version
// (see wiki for more details)
#define initParamRef DO_NOT_USE_ME

#endif // PARAM_CONVERSION_TEMPLATE_H