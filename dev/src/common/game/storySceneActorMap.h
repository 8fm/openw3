/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

/// Mapping of all actors with voice tags in the game world
class CStorySceneActorMap : public CObject
{
	DECLARE_ENGINE_CLASS( CStorySceneActorMap, CObject, 0 );

protected:
	mutable Red::Threads::CMutex					m_lock;
	THashMap< CName, TDynArray< THandle< CActor > > >	m_speakers;		//!< Map of speakers

public:
	CStorySceneActorMap();

	//! Serialization ( GC only )
	virtual void OnSerialize( IFile& file );

public:
	//! Clear map
	void Clear();

	//! Register actor in the actor map with given voice tag
	void RegisterSpeaker( CActor* actor, CName voiceTag );

	//! Unregister actor from the actor map with given voice tag
	void UnregisterSpeaker( CActor* actor, CName voiceTag );

	//! Unregister actor from the actor map ( removes all occurrences )
	void UnregisterSpeaker( CActor* actor );

	//! Find actors with given voice tags
	Bool FindSpeakers( CName voiceTag, TDynArray< THandle< CEntity > >& actors ) const;

	//! Find closest speaker with given voice tags
	CActor* FindClosestSpeaker( CName voiceTag, const Vector& position, Float searchRadius ) const;
};

BEGIN_CLASS_RTTI( CStorySceneActorMap );
	PARENT_CLASS( CObject );
END_CLASS_RTTI();
