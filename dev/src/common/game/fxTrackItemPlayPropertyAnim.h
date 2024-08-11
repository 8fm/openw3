/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "../../common/engine/fxTrackItem.h"

/// property anim play event
class CFXTrackItemPlayPropertyAnim : public CFXTrackItem
{
	DECLARE_ENGINE_CLASS( CFXTrackItemPlayPropertyAnim, CFXTrackItem, 0 );

private:
	CName						m_propertyAnimationName;
	Uint32						m_loopCount;
	Bool						m_restoreAtEnd;
public:
	CFXTrackItemPlayPropertyAnim();

	//! Change name of track item
	virtual void				SetName( const String& name );

	//! Get name of	track item
	virtual String				GetName() const;

	CName						GetPropertyAnimName() const;

public:
	//! Start track item effect, can spawn play data that will be ticked
	virtual IFXTrackItemPlayData* OnStart( CFXState& fxState ) const;
};

BEGIN_CLASS_RTTI( CFXTrackItemPlayPropertyAnim );
	PARENT_CLASS( CFXTrackItem );
	PROPERTY_EDIT( m_propertyAnimationName, TXT( "Property anim name" ) )
	PROPERTY_EDIT( m_restoreAtEnd, TXT( "Restore at end" ) )
	PROPERTY_EDIT( m_loopCount, TXT( "0 means infinite" ) )
END_CLASS_RTTI();