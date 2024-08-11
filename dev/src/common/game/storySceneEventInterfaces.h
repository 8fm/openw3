
#pragma once

#include "storySceneInterfaces.h"

//////////////////////////////////////////////////////////////////////////

class ISSAnimClipInterface
{
	RED_REGISTER_INTERFACE( ISSAnimClipInterface );

public:
	//...
};

//////////////////////////////////////////////////////////////////////////

class ISSDragAndDropBodyAnimInterface
{
	RED_REGISTER_INTERFACE( ISSDragAndDropBodyAnimInterface );

public:
	virtual void Interface_SetDragAndDropBodyAnimation( const TDynArray< CName >& animData, Float animDuration ) = 0;
};

//////////////////////////////////////////////////////////////////////////

class ISSDragAndDropMimicsAnimInterface
{
	RED_REGISTER_INTERFACE( ISSDragAndDropMimicsAnimInterface );

public:
	virtual void Interface_SetDragAndDropMimicsAnimation( const TDynArray< CName >& animData, Float animDuration ) = 0;
};

//////////////////////////////////////////////////////////////////////////
