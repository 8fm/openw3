/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../../common/engine/textureCache.h"


class CWccTextureCacheCooker : public CTextureCacheCooker
{
public:
	CWccTextureCacheCooker();
	~CWccTextureCacheCooker();

	virtual CAsyncTextureBaker::CookFunctionPtr GetDefaultCookFunction( const ECookingPlatform platform );
};
