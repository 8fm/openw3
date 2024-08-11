/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "pathlibCookerData.h"

#include "pathlibConnectorsBin.h"
#include "pathlibDetailedSurfaceData.h"
#include "pathlibSpecialZoneMap.h"
#include "pathlibWorld.h"

namespace PathLib
{

CCookerData::CCookerData()
	: m_specialZones( nullptr )
	, m_detailedSurfaceCollection( nullptr )
	, m_globalConnectorsBin( nullptr )
{

}
CCookerData::~CCookerData()
{
	Shutdown();
	ASSERT( !m_specialZones );
}

void CCookerData::Initialize( CWorld* world )
{
	CPathLibWorld* pathlib = world->GetPathLibWorld();

	m_specialZones = new CSpecialZonesMap();

	m_detailedSurfaceCollection = new CDetailedSurfaceCollection();
	m_detailedSurfaceCollection->Initialize( *pathlib );

	m_globalConnectorsBin = new CGlobalConnectorsBin();
}
void CCookerData::Shutdown()
{
	if ( m_specialZones )
	{
		delete m_specialZones;
		m_specialZones = nullptr;
	}
	if ( m_detailedSurfaceCollection )
	{
		delete m_detailedSurfaceCollection;
		m_detailedSurfaceCollection = nullptr;
	}

	if ( m_globalConnectorsBin )
	{
		delete m_globalConnectorsBin;
		m_globalConnectorsBin = nullptr;
	}
}

void CCookerData::DumpSurfaceData()
{
	if ( m_detailedSurfaceCollection )
	{
		delete m_detailedSurfaceCollection;
		m_detailedSurfaceCollection = nullptr;
	}
}

};		// namespace PathLib


Bool INavigationCookingSystem::CommitOutput()
{
	return true;
}

CNavigationCookingContext::CNavigationCookingContext()
	: m_cookerData( nullptr )
	, m_world( nullptr )
	, m_cookDirectory( nullptr )
{

}
CNavigationCookingContext::~CNavigationCookingContext()
{
	if ( m_cookerData )
	{
		m_cookerData->Shutdown();
		delete m_cookerData;
	}

	for ( auto& pair : m_cookingSystems )
	{
		delete pair.m_second;
	}
	
}

void CNavigationCookingContext::Initialize( CWorld* world, Bool pathlibCook )
{
	m_world = world;
	m_cookDirectory = world->GetPathLibWorld()->GetCookedDataDirectory();

	if ( pathlibCook )
	{
		m_cookerData = new PathLib::CCookerData();
		m_cookerData->Initialize( world );
	}

	InitializeSystems( world, pathlibCook );
}
 void CNavigationCookingContext::InitializeSystems( CWorld* world, Bool pathlibCook )
 {
 }

INavigationCookingSystem* CNavigationCookingContext::GetSystem( Uint32 systemId ) const
{
	auto itFind = m_cookingSystems.Find( systemId );
	if ( itFind == m_cookingSystems.End() )
	{
		return nullptr;
	}
	return itFind->m_second;
}

Bool CNavigationCookingContext::CommitOutput()
{
	for ( const auto& pair : m_cookingSystems )
	{
		if ( !pair.m_second->CommitOutput() )
		{
			return false;
		}
	}
	return true;
}