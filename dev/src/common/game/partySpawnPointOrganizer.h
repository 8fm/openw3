/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "spawnTreeMultiEntry.h"

class CPartySpawnpointOrganizer : public CPartySpawnOrganizer
{
	DECLARE_RTTI_SIMPLE_CLASS( CPartySpawnpointOrganizer )
public:
	void PrePartySpawn( CCreaturePartyEntry* entry, TDynArray< EntitySpawnInfo >& spawnInfo, SEncounterSpawnPoint* sp ) override;
};

BEGIN_CLASS_RTTI( CPartySpawnpointOrganizer )
	PARENT_CLASS( CPartySpawnOrganizer )
END_CLASS_RTTI()


class CPartySpreadOrganizer : public CPartySpawnOrganizer
{
	DECLARE_RTTI_SIMPLE_CLASS( CPartySpreadOrganizer )
protected:
	Float					m_spreadRadiusMin;
	Float					m_spreadRadiusMax;
public:
	void PrePartySpawn( CCreaturePartyEntry* entry, TDynArray< EntitySpawnInfo >& spawnInfo, SEncounterSpawnPoint* sp ) override;
};

BEGIN_CLASS_RTTI( CPartySpreadOrganizer )
	PARENT_CLASS( CPartySpawnOrganizer )
	PROPERTY_EDIT( m_spreadRadiusMin, TXT( "Minimum spread radius" ) )
	PROPERTY_EDIT( m_spreadRadiusMax, TXT( "Maximum spread radius" ) )
END_CLASS_RTTI()