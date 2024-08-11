/**
* Copyright © 2011 CD Projekt Red. All Rights Reserved.
*/

#pragma once

namespace HardwareInstrumentation
{
	class CDiskInstrumentationPC
	{
	public:
		// Gathers information about disk for specified directory (example: "C:")
		Bool GatherInformation( const String& directoryName );

		// Returns all space information about disk in bytes
		void GetSpaceInformation( Uint64& totalFreeSpace, Uint64& availableFreeSpace, Uint64& totalSpace );

		// Returns free space in bytes
		RED_INLINE Uint64 GetTotalFreeSpace() { return m_totalFreeSpace; }

		// Returns used space in bytes
		RED_INLINE Uint64 GetAvailableFreeSpace() { return m_availableFreeSpace; }

		// Returns total amount of space in bytes
		RED_INLINE Uint64 GetTotalSpace() { return m_totalSpace; }

	private:
		Uint64 m_totalFreeSpace;
		Uint64 m_availableFreeSpace;
		Uint64 m_totalSpace;

	};
}
