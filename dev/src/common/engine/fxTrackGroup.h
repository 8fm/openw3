/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "fxBase.h"

class CFXTrack;
class CFXState;
class CComponent;

/// Group of FX tracks regarding one component
class CFXTrackGroup : public CFXBase
{
	DECLARE_ENGINE_CLASS( CFXTrackGroup, CFXBase, 0 );
	NO_DEFAULT_CONSTRUCTOR( CFXTrackGroup ); 

protected:
	String						m_name;					//!< Name of the track group ( user info only ) 
	TDynArray< CFXTrack* >		m_tracks;				//!< Tracks in the track group
	Bool						m_isEnabled;			//!< Is the group enabled
	Bool						m_isExpanded;			//!< Is the group expanded in the editor
	Color						m_trackGroupColor;		//!< Color of the track group
	CName						m_componentName;		//!< Name of the controlled component

public:
	//! Get the FX we are in
	RED_INLINE CFXDefinition* GetFX() const { return SafeCast< CFXDefinition >( GetParent() ); }

	//! Get FX track name
	RED_INLINE String GetName() const { return m_name; }

	//! Get tracks in this group
	RED_INLINE TDynArray< CFXTrack* >& GetTracks() { return m_tracks; }

	//! Is the track group expanded
	RED_INLINE Bool IsExpanded() const { return m_isExpanded; }

	//! Is the track group enabled
	RED_INLINE Bool IsEnabled() const { return m_isEnabled; }

	//! Get track group color
	RED_INLINE const Color& GetColor() const { return m_trackGroupColor; }

	//! Get the name of the controlled component
	RED_INLINE CName GetComponentName() const { return m_componentName; }

public:
	CFXTrackGroup( CFXDefinition* def, const String& name );

	//! Add track
	CFXTrack* AddTrack( const String& trackName );

	//! Remove track from this track group
	Bool RemoveTrack( CFXTrack *effectTrack );

	//! Get component being affected
	CComponent* GetAffectedComponent( const CFXState& state );

public:
	//! Expand track group in editor
	void Expand();

	//! Collapse track group in editor
	void Collapse();

	//! Change name of the track group
	virtual void SetName( const String &name );

	//! Remove track group from effect
	virtual void Remove();
};

BEGIN_CLASS_RTTI( CFXTrackGroup );
	PARENT_CLASS( CFXBase );
	PROPERTY( m_name );
	PROPERTY( m_tracks );
	PROPERTY( m_isExpanded );
	PROPERTY_EDIT( m_isEnabled, TXT("Is the track group enabled") );
	PROPERTY_EDIT( m_trackGroupColor, TXT("Color") );
	PROPERTY_CUSTOM_EDIT( m_componentName, TXT("Component name"), TXT("EntityComponentListEffectParameters") );
END_CLASS_RTTI();
