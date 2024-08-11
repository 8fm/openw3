/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */
#pragma once

//////////////////////////////////////////////////////////////////////////
// Forward Declarations
//////////////////////////////////////////////////////////////////////////
class CFlashMovieScaleform;
class CScaleformSystem;

//////////////////////////////////////////////////////////////////////////
// IScaleformPlayer
//////////////////////////////////////////////////////////////////////////
class IScaleformPlayer
{
public:
	virtual void OnScaleformInit()=0;
	virtual void OnScaleformShutdown()=0;

public:
	virtual Bool RegisterScaleformMovie( CFlashMovieScaleform* movie ) = 0;
	virtual Bool UnregisterScaleformMovie( CFlashMovieScaleform* movie ) = 0;

protected:
	IScaleformPlayer() {}
	virtual ~IScaleformPlayer() {}
};
