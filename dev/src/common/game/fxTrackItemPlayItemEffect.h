/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "../../common/engine/fxTrackItem.h"

/// Particle emitter spawner
class CFXTrackItemPlayItemEffect : public CFXTrackItem, public INamesListOwner
{
	DECLARE_ENGINE_CLASS( CFXTrackItemPlayItemEffect, CFXTrackItem, 0 );

private:
	CName	m_category;
	CName	m_itemName_optional;
	CName	m_effectName;

public:
	CFXTrackItemPlayItemEffect();

	//! Change name of track item
	virtual void SetName( const String& name );

	//! Get name of track item
	virtual String GetName() const;

	virtual void GetNamesList( TDynArray< CName >& names ) const;

public:
	//! Start track item effect, can spawn play data that will be ticked
	virtual IFXTrackItemPlayData* OnStart( CFXState& fxState ) const;
};

BEGIN_CLASS_RTTI( CFXTrackItemPlayItemEffect );
	PARENT_CLASS( CFXTrackItem );
	PROPERTY_CUSTOM_EDIT( m_category, TXT( "Category of the item" ), TXT("ItemCategorySelection") );
	PROPERTY_CUSTOM_EDIT( m_itemName_optional, TXT( "Item name (optional)" ), TXT("SelfSuggestedListSelection") );
	PROPERTY_EDIT( m_effectName, TXT( "Name of the effect to play" ) );
END_CLASS_RTTI();
