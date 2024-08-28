/**
* Copyright � 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#ifdef RED_PLATFORM_ORBIS

typedef SceInt32			 FileObject;
#if 0 // class_CTime is not used anywhere
typedef SceKernelTimespec	 FileTimeObject;
#endif

class COrbisDirWalker;
typedef COrbisDirWalker* FindFileObject;

#ifdef UNICODE
# define TO_PLATFORMCODE(str)	UNICODE_TO_ANSI( str )
#else
# define TO_PLATFORMCODE(str)	str
#endif

#endif // RED_PLATFORM_ORBIS