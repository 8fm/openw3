/**
* Copyright © 2011 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "diskInstrumentationPC.h"
#include <fileapi.h>

namespace HardwareInstrumentation
{
	Bool CDiskInstrumentationPC::GatherInformation(const String& directoryName)
	{
		ULARGE_INTEGER availableFreeSpace;
		ULARGE_INTEGER totalSpace;
		ULARGE_INTEGER totalFreeSpace;
		if( GetDiskFreeSpaceEx( directoryName.AsChar(), &availableFreeSpace, &totalSpace, &totalFreeSpace ) == false )
		{
			return false;
		}

		m_availableFreeSpace = availableFreeSpace.QuadPart;
		m_totalSpace = totalSpace.QuadPart;
		m_totalFreeSpace = totalFreeSpace.QuadPart;

		return true;
	}

	void CDiskInstrumentationPC::GetSpaceInformation( Uint64& totalFreeSpace, Uint64& availableFreeSpace, Uint64& totalSpace )
	{
		totalFreeSpace = GetTotalFreeSpace();
		availableFreeSpace = GetAvailableFreeSpace();
		totalSpace = GetTotalSpace();
	}
}
