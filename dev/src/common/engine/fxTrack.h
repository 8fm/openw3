/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#pragma once

class CFXTrackItem;

/// Track in the FX track group
class CFXTrack : public CFXBase
{
	DECLARE_ENGINE_CLASS( CFXTrack, CFXBase, 0 );
	NO_DEFAULT_CONSTRUCTOR( CFXTrack );

protected:
	TDynArray< CFXTrackItem* >		m_trackItems;		//!< Track items
	String							m_name;				//!< Name of the track

public:
	//! Get the track group we are in
	RED_INLINE CFXTrackGroup* GetTrackGroup() const { return (CFXTrackGroup*)( GetParent() ); }

	//! Get FX track name
	RED_INLINE String GetName() const { return m_name; }

	//! Get tracks in this group
	RED_INLINE TDynArray< CFXTrackItem* >& GetTrackItems() { return m_trackItems; }

public:
	CFXTrack( CFXTrackGroup* group, const String& name );

	//! Remove track item from this track
	Bool RemoveTrackItem( CFXTrackItem* trackItem );

	//! Add track item to the track
	CFXTrackItem* AddTrackItem( CClass *trackItemClass, Float position );
	void AddTrackItem( CFXTrackItem* newTrackItem, Float position );
	
public:
	//! Change FX object name
	virtual void SetName( const String &name );

	//! Remove from parent structure
	virtual void Remove();
};

BEGIN_CLASS_RTTI( CFXTrack );
	PARENT_CLASS( CFXBase );
	PROPERTY( m_trackItems );
	PROPERTY( m_name );
END_CLASS_RTTI();
