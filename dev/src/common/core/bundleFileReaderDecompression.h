/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "bundleheader.h"

namespace BundleFileReaderDecompression
{
	// Synchronous decompress
	Bool DecompressFileBufferSynch( Red::Core::Bundle::ECompressionType compType, const void* srcBuffer, Uint32 srcSize, void* destBuffer, Uint32 destSize );
}
