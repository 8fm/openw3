/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

namespace PathLib
{

	class CSpecialZonesMap;
	class CDetailedSurfaceCollection;
	class CGlobalConnectorsBin;

	class CCookerData
	{
	protected:
		CSpecialZonesMap*					m_specialZones;
		CDetailedSurfaceCollection*			m_detailedSurfaceCollection;
		CGlobalConnectorsBin*				m_globalConnectorsBin;

	public:
		CCookerData();
		~CCookerData();

		void						Initialize( CWorld* world );
		void						Shutdown();

		void						DumpSurfaceData();

		CSpecialZonesMap*			GetSpecialZones() const									{ return m_specialZones; }
		CDetailedSurfaceCollection*	GetSurfaceCollection() const							{ return m_detailedSurfaceCollection; }
		CGlobalConnectorsBin*		GetGlobalConnections() const							{ return m_globalConnectorsBin; }
	};

};			// namespace PathLib

class INavigationCookingSystem
{
	friend class CNavigationCookingContext;
protected:
	CNavigationCookingContext*							m_context;
public:
	static const Uint32 COOKING_SYSTEM_ID = 0xbaaaaaad;

	virtual ~INavigationCookingSystem()														{}

	virtual Bool CommitOutput();
};

class CNavigationCookingContext
{
protected:
	PathLib::CCookerData*								m_cookerData;
	THashMap< Uint32, INavigationCookingSystem* >		m_cookingSystems;
	CWorld*												m_world;
	CDirectory*											m_cookDirectory;

	template< class TSystem >
	void RegisterSystem( TSystem* system )													{ ASSERT( m_cookingSystems.Find( TSystem::COOKING_SYSTEM_ID ) == m_cookingSystems.End() ); m_cookingSystems[ TSystem::COOKING_SYSTEM_ID ] = system; system->m_context = this; }
public:
	CNavigationCookingContext();
	~CNavigationCookingContext();

	void Initialize( CWorld* world, Bool pathlibCook = true );
	virtual void InitializeSystems( CWorld* world, Bool pathlibCook );

	PathLib::CCookerData* GetPathlibCookerData() const										{ return m_cookerData; }
	Bool ShouldIgnorePathlib() const														{ return m_cookerData == nullptr; }
	CWorld* GetWorld() const																{ return m_world; }
	CDirectory* GetOutputDir() const														{ return m_cookDirectory; }
	INavigationCookingSystem* GetSystem( Uint32 systemId ) const;

	Bool CommitOutput();

	template< class TSystem >
	TSystem* Get() const																	{ INavigationCookingSystem* system = GetSystem( TSystem::COOKING_SYSTEM_ID ); if ( !system ) { return nullptr; } return static_cast< TSystem* >( system ); }
};