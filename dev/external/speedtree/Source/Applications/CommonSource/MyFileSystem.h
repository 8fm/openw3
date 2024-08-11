///////////////////////////////////////////////////////////////////////  
//  MyFileSystem.h
//
//	All source files prefixed with "My" indicate an example implementation, 
//	meant to detail what an application might (and, in most cases, should)
//	do when interfacing with the SpeedTree SDK.  These files constitute 
//	the bulk of the SpeedTree Reference Application and are not part
//	of the official SDK, but merely example or "reference" implementations.
//
//	*** INTERACTIVE DATA VISUALIZATION (IDV) CONFIDENTIAL AND PROPRIETARY INFORMATION ***
//
//	This software is supplied under the terms of a license agreement or
//	nondisclosure agreement with Interactive Data Visualization, Inc. and
//  may not be copied, disclosed, or exploited except in accordance with 
//  the terms of that agreement.
//
//      Copyright (c) 2003-2014 IDV, Inc.
//      All rights reserved in all media.
//
//      IDV, Inc.
//      http://www.idvinc.com


///////////////////////////////////////////////////////////////////////  
//  Preprocessor

#pragma once
#include "Core/FileSystem.h"


///////////////////////////////////////////////////////////////////////  
//  All SpeedTree SDK classes and variables are under the namespace "SpeedTree"

namespace SpeedTree
{

	///////////////////////////////////////////////////////////////////////  
	//  class CMyFileSystem

	class ST_DLL_LINK CMyFileSystem : public CFileSystem
	{
	public:
			bool		FileExists(const st_char* pFilename)
			{
				return CFileSystem::FileExists(pFilename);
			}

			size_t		FileSize(const st_char* pFilename)
			{
				// size in bytes
				return CFileSystem::FileSize(pFilename);
			}

			st_byte*	LoadFile(const char* pFilename, ETermHint eTermHint)
			{
				return CFileSystem::LoadFile(pFilename, eTermHint);
			}

			void		Release(st_byte* pBuffer)
			{
				return CFileSystem::Release(pBuffer);
			}
	};

} // end namespace SpeedTree

