/**
 * Copyright © 2007 CD Projekt Red. All Rights Reserved.
 */
#pragma once

struct EngineTime;
class CTimeManager;

#include "../../common/game/gameSystem.h"
#include "../../common/game/gameSystemOrder.h"
#include "../../common/engine/globalEventsManager.h"

class CFactsDB : public IGameSystem, public IGameSaveSection
{
	DECLARE_ENGINE_CLASS( CFactsDB, IGameSystem, 0 );

public:
	/// Use this expiration flag to indicate that a fact should never expire.
	static Int32 EXP_NEVER_SEC;
	static EngineTime EXP_NEVER;
	/// Use this expiration flag to indicate that a fact should expire when new world is loaded
	static Int32 EXP_ACT_END_SEC;
	static EngineTime EXP_ACT_END;

	struct Fact
	{
		Int32		m_value;
		EngineTime	m_time;
		EngineTime	m_expiryTime;

		RED_INLINE Fact()
			: m_value( 0 ), m_time( EngineTime::ZERO ), m_expiryTime( EXP_NEVER ) 
		{};

		RED_INLINE Fact( Int32 value, const EngineTime& time, EngineTime& expiryTime )
			: m_value( value ),  m_time( time ),  m_expiryTime( expiryTime ) 
		{};

		RED_INLINE Bool operator<( const Fact& rhs ) const
		{
			return m_time < rhs.m_time;
		}
	};

	struct FactsBucket : public TSortedArray< Fact >
	{
		Uint32 m_expiringCount;
		RED_INLINE FactsBucket()
			: m_expiringCount( 0 )
		{}
	};

private:
	typedef THashMap<String, FactsBucket> TFactsMap;
	TFactsMap	m_facts;

	TGlobalEventsReporterImpl< String >*	m_globalEventsReporter;

public:
	CFactsDB();
	~CFactsDB();

	virtual void Initialize() override;
	virtual void Shutdown() override;
	
	void AddFact( const String& id, Int32 value, const EngineTime& time, const EngineTime& validFor = EXP_NEVER );
	void AddFact( const String& id, Int32 value, const EngineTime& time, Int32 validFor );
	void RemoveFact( const String& id );
	void GetFacts( const String& id, TDynArray<const Fact*>& outFacts ) const;

	void AddID( const String& id );
	void GetIDs( TDynArray<String>& outIDs ) const;
	
	Int32 QuerySum( const String& factId );
	Int32 QuerySumSince( const String& factId, const EngineTime& sinceTime );
	Int32 QueryLatestValue( const String& factId );

	Bool DoesExist( const String& factId ) const;

	friend IFile& operator<<( IFile& file, CFactsDB* db );

	void RemoveFactsAreaChanged();
	void RemoveFactsExpired();

public:
	virtual void OnGameStart( const CGameInfo& info ) override;
	virtual void OnGameEnd( const CGameInfo& gameInfo ) override;
	virtual void Tick( Float timeDelta ) override;
	void	OnDLCGameStarted(){ AddInitFacts(); }


	virtual bool OnSaveGame( IGameSaver* saver );
	virtual void OnLoadGame( IGameLoader* loader );
	virtual void ExportToXML( CXMLWriter& writer );

	
private:
	void AddInitFacts();
	const FactsBucket* QueryFactsBucket( const String& id );
	void RemoveExpired( const String& id, FactsBucket& FactsBucket );

	// Patch 1.21 hacky optimization:
	void StreamLoad( ISaveFile* loader, Uint32 version );
	void StreamSave( ISaveFile* saver );
	void ApplyPatch_RemoveUnneededFacts();
	void RemoveFactsWithPrefix( const String& prefix );

	ASSIGN_GAME_SYSTEM_ID( GS_FactsDB );
};

BEGIN_CLASS_RTTI( CFactsDB );
	PARENT_CLASS( IGameSystem );
END_CLASS_RTTI();