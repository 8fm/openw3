/***********************************************************************
The content of this file includes source code for the sound engine
portion of the AUDIOKINETIC Wwise Technology and constitutes "Level
Two Source Code" as defined in the Source Code Addendum attached
with this file.  Any use of the Level Two Source Code shall be
subject to the terms and conditions outlined in the Source Code
Addendum and the End User License Agreement for Wwise(R).

Version:  Build: 
Copyright (c) 2006-2020 Audiokinetic Inc.
***********************************************************************/

#pragma once

#include <AK/SoundEngine/Common/AkTypes.h>

#include <dlfcn.h>

#define DECLARE_EXTERN_DL_FUNCTION(__name__) extern __typeof(__name__)* _import_##__name__

#define DEFINE_DL_FUNCTION(__name__) __typeof(__name__)* _import_##__name__

#define LOAD_DL_FUNCTION(__hLib__,__name__) { \
	_import_##__name__ = (__typeof(_import_##__name__)) dlsym(__hLib__, #__name__); \
	if (_import_##__name__ == NULL) \
		return AK_Fail; \
}

///
/// Linux sink dependencies need to be dynamically loaded with dlopen
///
class IDynamicAPI
{
public:
    virtual AKRESULT DynamicLoadAPI() = 0;
};
