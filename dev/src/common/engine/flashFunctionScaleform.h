/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#pragma once

#include "flashFunction.h"

#ifdef USE_SCALEFORM

class CFlashMovieScaleform;

class CFlashFunctionScaleform : public CFlashFunction
{
	friend class CFlashValue;

private:
	GFx::Value					m_gfxFunc;

public:
	CFlashFunctionScaleform( CFlashMovieScaleform* flashMovie, CFlashFunctionHandler* flashFunctionHandler );

protected:
	virtual ~CFlashFunctionScaleform();
};

#endif // USE_SCALEFORM