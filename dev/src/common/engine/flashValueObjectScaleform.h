/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#pragma once

#ifdef USE_SCALEFORM

#include "flashValueObject.h"

class CFlashMovieScaleform;

//////////////////////////////////////////////////////////////////////////
// CFlashObjectScaleform
//////////////////////////////////////////////////////////////////////////
class CFlashObjectScaleform : public CFlashObject
{
public:
	CFlashObjectScaleform( CFlashMovieScaleform* flashMovie, const String& flashClassName );

protected:
	~CFlashObjectScaleform();
};

//////////////////////////////////////////////////////////////////////////
// CFlashArrayScaleform
//////////////////////////////////////////////////////////////////////////
class CFlashArrayScaleform : public CFlashArray
{
public:
	CFlashArrayScaleform( CFlashMovieScaleform* flashMovie );

protected:
	~CFlashArrayScaleform();
};

//////////////////////////////////////////////////////////////////////////
// CFlashStringScaleform
//////////////////////////////////////////////////////////////////////////
class CFlashStringScaleform : public CFlashString
{
public:
	CFlashStringScaleform( CFlashMovieScaleform* flashMovie, const String& value );

protected:
	~CFlashStringScaleform();
};

#endif // USE_SCALEFORM