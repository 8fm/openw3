/***********************************************************************
  The content of this file includes source code for the sound engine
  portion of the AUDIOKINETIC Wwise Technology and constitutes "Level
  Two Source Code" as defined in the Source Code Addendum attached
  with this file.  Any use of the Level Two Source Code shall be
  subject to the terms and conditions outlined in the Source Code
  Addendum and the End User License Agreement for Wwise(R).

  Version: v2013.2.9  Build: 4872
  Copyright (c) 2006-2014 Audiokinetic Inc.
 ***********************************************************************/

///////////////////////////////////////////////////////////////////////
//
// AkIndexable.h
//
// Declaration of audionodes items that are indexable
//
///////////////////////////////////////////////////////////////////////
#ifndef _INDEXABLE_H_
#define _INDEXABLE_H_

enum AkObjectCategory
{
	ObjCategory_Undefined	= 0,	// The Object type is undefined
	ObjCategory_ActorMixer	= 1,	// The Object is an actro-mixer
	ObjCategory_Container	= 2,	// The Object is a container
	ObjCategory_Sound		= 4,	// The Object is a sound
	ObjCategory_State		= 5,	// The Object is a state
	ObjCategory_Action		= 6,	// The Object is an action
	ObjCategory_Event		= 7,	// The Object is an event
	ObjCategory_Track		= 8,	// The object is a music track
	ObjCategory_MusicNode	= 9,	// The object is a music node ( but not a track )
	ObjCategory_FeedbackNode= 10,	// The object is a feedback node
	ObjCategory_Bus			= 11	// The object is a bus.

};

class CAkIndexable
{
public:
	// IMPORTANT: These two fields are reserved for use by CAkIndexItem.
	CAkIndexable * pNextItem;
	AkUniqueID key;

protected:
	AkInt32 m_lRef;

public:
	CAkIndexable(AkUniqueID in_ulID = 0);
	virtual ~CAkIndexable();

	virtual AkUInt32 AddRef() = 0; 
    virtual AkUInt32 Release() = 0; 

	// STRICTLY FOR USE BY CAkIndexItem::GetPtrAndAddRef
	// (avoid inner lock for performance)
	AkForceInline AkUInt32 AddRefUnsafe()
	{
		return ++m_lRef; 
	}

	AkForceInline AkUniqueID ID(){ return key; }

	// Commented out since every CAkIndexable will implement it, but there is no need to force virtual interface
	//virtual void AddToIndex() = 0 ;
	//virtual void RemoveFromIndex() = 0 ;

	virtual AkObjectCategory Category();
};

#endif
