/**
* Copyright (c) 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#ifdef RED_PLATFORM_LINUX

#include <dirent.h>

struct FindFileObject
{
	DIR* m_folder;
	struct dirent* m_currentEntry;
	AnsiChar m_path[ PATH_MAX ];
	AnsiChar m_pattern[ PATH_MAX ];
	Bool m_usePattern;
};

typedef int				FileObject;

#endif // RED_PLATFORM_LINUX
