#pragma once

// GLOBALS for the whole systems, tuned using worst case scenario (W3 Novigrad)
namespace SectorData
{
	/// Maximum number of objects registered in the system
	/// NOTE: This number grew from 600 to 700k for EP2
	static const unsigned int MAX_OBJECTS = 700 * 1000;

	/// Maximum number of objects that can be currently streamed
	static const unsigned int MAX_STREAMED_OBJECTS = 70 * 1000;

	/// Maximum number of sectors registered in the system
	static const unsigned int MAX_SECTORS = 30 * 1000;
}
