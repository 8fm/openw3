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

#ifndef DIR_ENTRY_H
#define DIR_ENTRY_H

#include "foundation/PxPreprocessor.h"
#include "PsString.h"

#if defined PX_WINDOWS
#	ifndef NOMINMAX
#		define NOMINMAX
#	endif
#	include <windows.h>
#else
#	error Unsupported platform
#endif

namespace physx
{
	class DirEntry
	{
	public:

		DirEntry()
		{
			mIsDone = false;
			mFindHandle = NULL;
		}

		~DirEntry()
		{
			if (!isDone())
			{
				while (next());
			}
		}

		// Get successive element of directory.
		// Returns true on success, error otherwise.
		bool next()
		{
			if (mIsDone)
			{
				return false;
			} 
			else if (!FindNextFile(mFindHandle, &mFindData))
			{
				mIsDone = true;

				// No && !
				return (ERROR_NO_MORE_FILES == GetLastError())
					& (TRUE != FindClose(mFindHandle));
			}
			return true;
		}

		// No more entries in directory?
		bool isDone() const
		{
			return mIsDone;
		}

		// Is this entry a directory?
		bool isDirectory() const
		{
			return 0 != (mFindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
		}

		// Get name of this entry.
		const char* getName() const
		{
			return mFindData.cFileName;
		}

		// Get first entry in directory.
		static bool GetFirstEntry(const char* path, DirEntry& dentry)
		{
			char tmp[MAX_PATH+1];

			if (2 + (PxI32)::strlen(path) > physx::string::sprintf_s(tmp, sizeof(tmp), "%s\\*", path))
			{
				return false;
			}

			dentry.mFindHandle = FindFirstFile(tmp, &dentry.mFindData);
			dentry.mIsDone = INVALID_HANDLE_VALUE == dentry.mFindHandle;

			return true;
		}

	private:

		bool mIsDone;
		HANDLE mFindHandle;
		WIN32_FIND_DATA mFindData;
	};
}

#endif
