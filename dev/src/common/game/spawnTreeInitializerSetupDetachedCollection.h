/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "spawnTreeInitializer.h"

class CSpawnTreeInitializerCollectDetachedSetup : public ISpawnTreeInitializer
{
	DECLARE_ENGINE_CLASS( CSpawnTreeInitializerCollectDetachedSetup, ISpawnTreeInitializer, 0 );
protected:
	Float									m_maximumCollectionDelay;
	Bool									m_greedyCollection;

public:
	CSpawnTreeInitializerCollectDetachedSetup()
		: m_maximumCollectionDelay( 0.f )
		, m_greedyCollection( false )												{}

	String				GetBlockCaption() const override;
	String				GetEditorFriendlyName() const override;	
	void				UpdateEntrySetup( const CBaseCreatureEntry* const  entry, CSpawnTreeInstance& instance, SSpawnTreeEntrySetup& setup ) const override;

	Bool				IsSpawnableOnPartyMembers() const;
};

BEGIN_CLASS_RTTI( CSpawnTreeInitializerCollectDetachedSetup )
	PARENT_CLASS( ISpawnTreeInitializer )
	PROPERTY_EDIT( m_maximumCollectionDelay, TXT("Maximum time needed to collect creature detached by other entry") )
	PROPERTY_EDIT( m_greedyCollection, TXT("Collect as much creatures as you can, don't mind spawn limits") )
END_CLASS_RTTI()
