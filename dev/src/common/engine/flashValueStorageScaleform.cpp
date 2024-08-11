/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#include "build.h"

#ifdef USE_SCALEFORM

#include "flashValueStorageScaleform.h"
#include "flashMovieScaleform.h"

CFlashValueStorageScaleform::CFlashValueStorageScaleform( CFlashMovieScaleform* flashMovie )
	: CFlashValueStorage( flashMovie )
{
}

CFlashValueStorageScaleform::~CFlashValueStorageScaleform()
{
}

#endif // USE_SCALEFORM
