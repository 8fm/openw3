/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#pragma once

#include "crowdAgent.h"
#include "crowdManager.h"
#include "crowdObstacle.h"

class CCrowdEntryPoint;

class CCrowdArea : public CEntity
{
	DECLARE_ENGINE_CLASS( CCrowdArea, CEntity, 0 );
																	 			 
private:
	// properties
	Int32										m_desiredAmountOfAgents; // must be Int32. if it is TAgentIndex I can not set in in editor.

	// runtime data
	TDynArray< THandle< CCrowdEntryPoint > >	m_entries;

	// runtime flags
	Bool										m_spawned	: 1;

private:
	void										FindObstacleEdgesInArea( CWorld* world, CCrowdManager* crowd );

public:
	RED_INLINE TAgentIndex					GetDesiredAmountOfAgents() const		{ return static_cast< TAgentIndex >( m_desiredAmountOfAgents ); }
	RED_INLINE void							MarkSpawned()							{ m_spawned = true; }
	RED_INLINE Bool							AreAllAgentsSpawned()					{ return m_spawned; }

	Box											GetBoundingBox() const;
	Box2										GetBoundingBox2() const;
	Bool										ContainsPosition2( const Vector2& pos ) const;
	void										AddIfInside( CCrowdEntryPoint* entry );
	void										ClearEntries( )							{ m_entries.Clear( ); }
	void										OnAttached( CWorld* world ) override;	
	void										OnDetached( CWorld* world ) override;	
	CCrowdEntryPoint*							RandomEntry() const;
	CCrowdEntryPoint*							RandomEntryDifferent( CCrowdEntryPoint* differentThanThis ) const;
	Vector2										RandomPositionInside2() const;
};

BEGIN_CLASS_RTTI( CCrowdArea )
	PARENT_CLASS( CEntity );	
	PROPERTY_EDIT_RANGE( m_desiredAmountOfAgents, TXT("Desired amount of agents"), 0, MAX_CROWD_AGENTS );
END_CLASS_RTTI();
