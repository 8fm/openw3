/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "crypto.h"

#ifdef RED_USE_CRYPTO

struct Ssha256
{
	Uint8 m_value[32];
};

void CalculateSha256( IFile& file, Ssha256& outHash );
void CalculateSha256( IFile& file, void* extraData, size_t extraDataSize, Ssha256& outHash );
void CalculateSha256( void* buffer, size_t size, Ssha256& outHash );

#endif // RED_USE_CRYPTO