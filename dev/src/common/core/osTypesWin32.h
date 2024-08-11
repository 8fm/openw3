/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

struct FindFileObject
{
	HANDLE			m_handle;
	WIN32_FIND_DATA	m_findData;
	Bool			m_hasMore;
};


typedef HANDLE				FileObject;
typedef FILETIME			FileTimeObject;

#if defined( RED_PLATFORM_WIN32 ) || defined( RED_PLATFORM_WIN64 ) || defined( RED_PLATFORM_DURANGO )
#	ifdef UNICODE
#		define TO_PLATFORMCODE(str)	str
#	else
#		define TO_PLATFORMCODE(str)	ANSI_TO_UNICODE( str )
#	endif
#endif
