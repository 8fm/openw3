#include "build.h"
#include "factsDB.h"
#include "../core/xmlWriter.h"
#include "../core/gameSave.h"
#include "../core/engineTime.h"
#include "../core/gatheredResource.h"
#include "../engine/gameDataStorage.h"

Int32 CFactsDB::EXP_NEVER_SEC = -1;
Int32 CFactsDB::EXP_ACT_END_SEC = -2;

EngineTime CFactsDB::EXP_NEVER = EngineTime( (Double) EXP_NEVER_SEC );
EngineTime CFactsDB::EXP_ACT_END = EngineTime( (Double) EXP_ACT_END_SEC );

#define FACTS_DB_MAGIC_START 'FDBS'
#define FACTS_DB_MAGIC_END 'FDBE'

IMPLEMENT_ENGINE_CLASS( CFactsDB );

CFactsDB::CFactsDB()
{}

CFactsDB::~CFactsDB()
{}

void CFactsDB::AddID( const String& id )
{
	if ( id == String::EMPTY )
	{
		RED_HALT( "CFactsDB::AddID - adding empty string facts is not allowed" );
		return;
	}

	TFactsMap::iterator it = m_facts.Find( id );
	if ( it == m_facts.End() )
	{
		m_facts.Insert( id, FactsBucket() );
		m_globalEventsReporter->AddEvent( GET_FactAdded, id );
	}
}

void CFactsDB::AddFact( const String& id, Int32 value, const EngineTime& time, Int32 validFor )
{
	AddFact( id, value, time, EngineTime( (double) validFor ) );
}

void CFactsDB::AddFact( const String& id, Int32 value, const EngineTime& time, const EngineTime& validFor )
{
	if ( id == String::EMPTY )
	{
		RED_HALT( "CFactsDB::AddFact - adding empty string facts is not allowed" );
		return;
	}

	TFactsMap::iterator it = m_facts.Find( id );
	if ( it == m_facts.End() )
	{
		m_facts.Insert( id, FactsBucket() );
		it = m_facts.Find( id );
	}
	FactsBucket& bucket = it->m_second;
	if ( bucket.Size() ) 
	{
		RemoveExpired( id, bucket );
	}

	EngineTime expiryTime;
	if ( validFor >= EngineTime::ZERO )
	{
		expiryTime = time + validFor;
		++bucket.m_expiringCount;
	}
	else
	{
		expiryTime = validFor;
	}
	Fact newFact( value, time, expiryTime );
	bucket.Insert( newFact );
	m_globalEventsReporter->AddEvent( GET_FactAdded, id );
}

void CFactsDB::RemoveFact( const String& id )
{
	m_facts.Erase( id );
	m_globalEventsReporter->AddEvent( GET_FactRemoved, id );
}

void CFactsDB::GetIDs( TDynArray<String>& outIDs ) const
{
	for ( TFactsMap::const_iterator it = m_facts.Begin(); it != m_facts.End(); ++it )
	{
		outIDs.PushBack( it->m_first );
	}
}

void CFactsDB::GetFacts( const String& id, TDynArray< const Fact* >& outFacts ) const
{
	TFactsMap::const_iterator it = m_facts.Find( id );
	if ( it != m_facts.End() )
	{
		const FactsBucket& bucket = it->m_second;
		for ( FactsBucket::const_iterator it = bucket.Begin(); it != bucket.End(); ++it )
		{
			outFacts.PushBack( &( *it ) );
		}
	}
}

const CFactsDB::FactsBucket* CFactsDB::QueryFactsBucket( const String& id )
{
	TFactsMap::iterator it = m_facts.Find( id );
	if ( it != m_facts.End() )
	{
		FactsBucket& bucket = it->m_second;
		if ( bucket.Size()  ) 
		{
			RemoveExpired( id, bucket );
			return &bucket;
		}
	}
	return NULL;
}

Int32 CFactsDB::QuerySum( const String& id )
{
	const FactsBucket* bucket = QueryFactsBucket( id );
	if ( (bucket == NULL) || ( bucket->Size() == 0 ) )
	{
		return 0;
	}
	Int32 sum = 0;
	for ( FactsBucket::const_iterator bucketIt = bucket->Begin(); bucketIt != bucket->End(); ++bucketIt)
	{
		sum += bucketIt->m_value;
	}
	return sum; 
}

Int32 CFactsDB::QuerySumSince( const String& id, const EngineTime& sinceTime )
{
	const FactsBucket* bucket = QueryFactsBucket( id );
	if ( (bucket == NULL) || ( bucket->Size() == 0 ) )
	{
		return 0;
	}
	Int32 sinceTimeSum = 0;
	for( Int32 i = bucket->Size() - 1; i >= 0 && (*bucket)[i].m_time < sinceTime ; --i )
	{
		sinceTimeSum += (*bucket)[i-1].m_value;
	}
	return sinceTimeSum;
}

Int32 CFactsDB::QueryLatestValue( const String& id )
{
	const FactsBucket* bucket = QueryFactsBucket( id );
	if ( (bucket == NULL) || ( bucket->Size() == 0 ) )
	{
		return 0;
	}
	return bucket->Back().m_value; 
}

Bool CFactsDB::DoesExist( const String& id ) const
{
	TFactsMap::const_iterator it = m_facts.Find( id );
	return ( it != m_facts.End() );
}

void CFactsDB::RemoveFactsAreaChanged()
{
	for ( TFactsMap::iterator idIt = m_facts.Begin(); idIt != m_facts.End(); ++idIt )
	{
		FactsBucket& bucket = idIt->m_second;
		for ( FactsBucket::iterator bucketIt = bucket.Begin();bucketIt != bucket.End(); )
		{
			if ( bucketIt->m_expiryTime == EXP_ACT_END )
			{
				bucket.Erase( bucketIt );
				m_globalEventsReporter->AddEvent( GET_FactRemoved, idIt->m_first );
			}
			else
			{
				++bucketIt;
			}
		}
	}
}

void CFactsDB::RemoveFactsExpired()
{
	for ( TFactsMap::iterator idIt = m_facts.Begin(); idIt != m_facts.End(); ++idIt )
	{
		const String& id = idIt->m_first;
		FactsBucket& bucket = idIt->m_second;
		RemoveExpired( id, bucket );
	}
}

void CFactsDB::RemoveExpired( const String& id, FactsBucket& bucket )
{
	if ( bucket.m_expiringCount > 0 )
	{
		const EngineTime& currTime = GGame->GetEngineTime();
		
		for ( Uint32 i = 0; i < bucket.Size(); )
		{
			const auto & theBucket = bucket[ i ];
			if ( ( theBucket.m_expiryTime >= EngineTime::ZERO ) && ( currTime >= theBucket.m_expiryTime ) )
			{
				bucket.RemoveAt(i);
				--bucket.m_expiringCount;
				m_globalEventsReporter->AddEvent( GET_FactRemoved, id );
			}
			else
			{
				++i;
			}
		}
	}
}

void CFactsDB::StreamLoad( ISaveFile* loader, Uint32 version )
{
	Uint32 magic = 0;
	*loader << magic;
	RED_FATAL_ASSERT( magic == FACTS_DB_MAGIC_START, "format problem" );
	if ( magic != FACTS_DB_MAGIC_START )
	{
		return;
	}

	Uint32 factsCount = m_facts.Size();
	*loader << factsCount;

	for ( Uint32 i = 0; i < factsCount; i++ )
	{
		String factName; 
		*loader << factName;

		Uint16 expCount = 0;
		*loader << expCount;

		Uint16 entryCount = 0;
		*loader << entryCount;

		FactsBucket bucket;
		bucket.m_expiringCount = expCount;

		for ( Uint16 i = 0; i < entryCount; ++i )
		{
			Fact* fact = new ( bucket ) Fact;
			Float time = -1.f;
			Int16 val = 0; 

			*loader << val;
			*loader << time;

			fact->m_value = val;
			fact->m_time = time;

			*loader << time;

			fact->m_expiryTime = time;
		}

		m_facts.Insert( factName, bucket );
		m_globalEventsReporter->AddEvent( GET_FactAdded, factName, true );
	}

	magic = 0;
	*loader << magic;
	RED_FATAL_ASSERT( magic == FACTS_DB_MAGIC_END, "format problem" );
}

void CFactsDB::StreamSave( ISaveFile* saver )
{
	// SAVE_VERSION_STREAM_FACTS_DB

	Uint32 magic = FACTS_DB_MAGIC_START;
	*saver << magic;

	Uint32 factsCount = m_facts.Size();
	*saver << factsCount;

	for ( TFactsMap::const_iterator it = m_facts.Begin(); it != m_facts.End(); ++it )
	{
		String id = it->m_first; 
		*saver << id;

		Uint16 expCount = it->m_second.m_expiringCount;
		*saver << expCount;

		Uint16 count = it->m_second.Size();
		*saver << count;

		for ( Uint16 i = 0; i < count; ++i )
		{
			const Fact& fact = it->m_second[ i ];

			RED_FATAL_ASSERT( fact.m_value > Int32( SHRT_MIN ) && fact.m_value < Int32( SHRT_MAX ), "OOPS! use Int32..." ); 
			Int16 val = fact.m_value; 
			*saver << val;

			Float time = fact.m_time;
			*saver << time;

			time = fact.m_expiryTime;
			*saver << time;
		}
	}

	magic = FACTS_DB_MAGIC_END;
	*saver << magic;
}

Bool CFactsDB::OnSaveGame( IGameSaver* saver )
{
	TIMER_BLOCK( time )

	CGameSaverBlock block( saver, CNAME( facts ) );

	IGameDataStorage* data = CGameDataStorage< MC_Gameplay, MemoryPool_GameSave >::Create( 1024 * 1024, 0 );

	if ( data )
	{
		ISaveFile* directStream = nullptr;
		IGameSaver* mysaver = SGameSaveManager::GetInstance().CreateSaver( data, &directStream );
		
		if ( mysaver )
		{
			StreamSave( directStream );
			delete mysaver;
		}

		saver->AddStorageStream( data );
		delete data;
	}

	END_TIMER_BLOCK( time )

	return true;
}

void CFactsDB::OnLoadGame( IGameLoader* loader )
{
	CGameSaverBlock block( loader, CNAME( facts ) );
	m_facts.Clear();

	if ( loader->GetSaveVersion() < SAVE_VERSION_STREAM_FACTS_DB )
	{
		// legacy format 

		Uint32 count = 0;
		loader->ReadValue( CNAME(count), count );

		#ifndef NO_SAVE_VERBOSITY
			RED_LOG( Save, TXT("Facts count: %ld"), count ); 
		#endif

		for ( Uint32 i=0; i<count; i++ )
		{
			CGameSaverBlock factBlock( loader, CNAME(fact) );
			String factName;
			loader->ReadValue( CNAME(id), factName );
			FactsBucket bucket;
			loader->ReadValue( CNAME(expiringCount), bucket.m_expiringCount );
			Uint32 entryCount = 0;
			loader->ReadValue( CNAME(entryCount), entryCount );

			#ifndef NO_SAVE_VERBOSITY
				RED_LOG( Save, TXT("Fact: %ls, entryCount: %ld"), factName.AsChar(), entryCount ); 
			#endif

			for ( Uint32 i=0; i<entryCount; ++i )
			{
				CGameSaverBlock entryBlock( loader, CNAME(entry) );
				Fact* fact = new ( bucket ) Fact;
				Double time = -1.0;
				loader->ReadValue( CNAME(value), fact->m_value );
				loader->ReadValue( CNAME(time), time );
				fact->m_time = time;
				loader->ReadValue( CNAME(expiryTime), time );
				fact->m_expiryTime = time;
			}

			m_facts.Insert( factName, bucket );
			m_globalEventsReporter->AddEvent( GET_FactAdded, factName, true );
		}

		// ka-boom
		ApplyPatch_RemoveUnneededFacts();
	}
	else
	{
		// "optimized" :( format

		IGameDataStorage* data = loader->ExtractDataStorage();
		if( data )
		{
			Uint32 version = 0;
			ISaveFile* directStream = nullptr;
			IGameLoader* loader = SGameSaveManager::GetInstance().CreateLoader( data, &directStream, &version );
			if ( loader )
			{
				StreamLoad( directStream, version );
				delete loader;
			}

			delete data;
		}
	}
}

void CFactsDB::ExportToXML( CXMLWriter& writer )
{
	writer.BeginNode( TXT( "FactsDB" ) );
	writer.AttributeT( TXT("count"), m_facts.Size() );
	for ( TFactsMap::const_iterator it = m_facts.Begin(); it != m_facts.End(); ++it )
	{
		writer.BeginNode( TXT("Fact") );
		writer.SetAttribute( TXT("id"), it->m_first );
		writer.AttributeT( TXT("expiringCount"), it->m_second.m_expiringCount );
		{
			writer.BeginNode( TXT("Entries") );
			const Uint32 count = it->m_second.Size();
			writer.AttributeT( TXT("count"), count );
			for ( Uint32 i=0; i<count; ++i )
			{
				writer.BeginNode( TXT("Entry") );
				const Fact& fact = it->m_second[i];								
				writer.AttributeT( TXT("value"), fact.m_value );
				writer.AttributeT( TXT("time"), fact.m_time );
				writer.AttributeT( TXT("expiryTime"), fact.m_expiryTime );
				writer.EndNode();				
			}
			writer.EndNode();
		}		
		writer.EndNode();		
	}
	writer.EndNode();
}

CGatheredResource resFactsAtStart( TXT("gameplay\\globals\\initialfacts.csv"), RGF_Startup );	
CGatheredResource resFactsFixup( TXT("gameplay\\globals\\facts_disabling.csv"), RGF_Startup );	

void CFactsDB::AddInitFacts()
{
	const EngineTime& time = GGame->GetEngineTime();
	const TDynArray< CGame::SInitialFact > &initialFacts = GGame->GetInitialFacts();

	C2dArray* factsArray = resFactsAtStart.LoadAndGet< C2dArray >();
	if ( factsArray && factsArray->GetNumberOfColumns() > 0 )
	{
		const Uint32 size = factsArray->GetNumberOfRows();
		for ( Uint32 i = 0; i < size; i++ )
		{
			const String& val = factsArray->GetValue( 0, i );
			if( !val.Empty() )
			{
				AddFact( val, 1, time, CFactsDB::EXP_NEVER );
			}				
		}
	}


	for ( auto ci = initialFacts.Begin(); ci != initialFacts.End(); ++ci )
	{
		AddFact( ci->m_name, ci->m_value, time, CFactsDB::EXP_NEVER );
	}
}

void CFactsDB::ApplyPatch_RemoveUnneededFacts()
{
	C2dArray* factsArray = resFactsFixup.LoadAndGet< C2dArray >();
	if ( factsArray && factsArray->GetNumberOfColumns() > 1 )
	{
		const Uint32 size = factsArray->GetNumberOfRows();
		for ( Uint32 i = 0; i < size; i++ )
		{
			Bool conditionMet = true;
			const String& cond = factsArray->GetValue( 0, i );
			if( !cond.Empty() )
			{
				conditionMet = ( QuerySum( cond ) != 0 );
			}	

			const String& val = factsArray->GetValue( 1, i );
			if ( conditionMet && !val.Empty() )
			{
				RemoveFactsWithPrefix( val );
			}
		}
	}

	#ifndef NO_SAVE_VERBOSITY
		RED_LOG( Save, TXT("*** FDB NEW CONTENTS:") );
		
		for ( TFactsMap::iterator it = m_facts.Begin(); it != m_facts.End(); ++it )
		{
			RED_LOG( Save, TXT("*** %ls, cnt: %ld"), it->m_first.AsChar(), it->m_second.Size() );
		}
	#endif
}


void CFactsDB::RemoveFactsWithPrefix( const String& prefix )
{
	// i'm not proud of it, but it's necessary
	// this method is basically a HACK, which I use to optimize content of the facts db in existing game sessions
	// (applied for saved older than SAVE_VERSION_STREAM_FACTS_DB to remove some unrelevant data from saves)

	for ( TFactsMap::iterator it = m_facts.Begin(); it != m_facts.End(); ++it )
	{
		while ( it->m_first.BeginsWith( prefix ) && it != m_facts.End() )
		{
			String nameCopy = it->m_first;
			m_facts.Erase( it );	
			m_globalEventsReporter->AddEvent( GET_FactRemoved, nameCopy );
		}
	}
}

void CFactsDB::OnGameStart( const CGameInfo& info )
{
	if ( !info.m_isChangingWorldsInGame )
	{
		m_facts.Clear();
		
		AddInitFacts();

		if ( info.m_gameLoadStream )
		{
			OnLoadGame( info.m_gameLoadStream );
		}
	}
}

void CFactsDB::OnGameEnd( const CGameInfo& gameInfo )
{
	if ( !gameInfo.m_isChangingWorldsInGame )
	{
		m_facts.Clear();
	}
}

void CFactsDB::Tick( Float timeDelta )
{
	m_globalEventsReporter->ReportEvents();
}

void CFactsDB::Initialize()
{
	CFactsDB::EXP_NEVER = EngineTime( (Double) EXP_NEVER_SEC );
	CFactsDB::EXP_ACT_END = EngineTime( (Double) EXP_ACT_END_SEC );

	m_globalEventsReporter = new TGlobalEventsReporterImpl< String >( GEC_Fact );

	SGameSessionManager::GetInstance().RegisterGameSaveSection( this );
}

void CFactsDB::Shutdown()
{
	SGameSessionManager::GetInstance().UnregisterGameSaveSection( this );

	delete m_globalEventsReporter;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
void funcFactsAdd( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String, id, String::EMPTY );
	GET_PARAMETER_OPT( Int32, value, 1 );
	GET_PARAMETER_OPT( Int32, validFor, CFactsDB::EXP_NEVER_SEC );
	FINISH_PARAMETERS;

	if ( GCommonGame && GCommonGame->GetSystem< CFactsDB >() && id != String::EMPTY )
	{
		GCommonGame->GetSystem< CFactsDB >()->AddFact( id, value, GGame->GetEngineTime(), validFor );
	}
}

void funcFactsQuerySum( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String, id, String::EMPTY );
	FINISH_PARAMETERS;

	Int32 sum = 0;
	if ( GCommonGame && GCommonGame->GetSystem< CFactsDB >() && id != String::EMPTY )
	{
		sum = GCommonGame->GetSystem< CFactsDB >()->QuerySum( id );
	}
	RETURN_INT ( sum );
}

void funcFactsQuerySumSince( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String, id, String::EMPTY );
	GET_PARAMETER( EngineTime, sinceTime, EngineTime::ZERO );
	FINISH_PARAMETERS;

	Int32 sum = 0;
	if ( GCommonGame && GCommonGame->GetSystem< CFactsDB >() && id != String::EMPTY )
	{	
		sum = GCommonGame->GetSystem< CFactsDB >()->QuerySumSince( id, sinceTime );
	}

	RETURN_INT ( sum );
}

void funcFactsQueryLatestValue( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String, id, String::EMPTY );
	FINISH_PARAMETERS;

	Int32 value = 0;
	if ( GCommonGame && GCommonGame->GetSystem< CFactsDB >() && id != String::EMPTY )
	{	
		value = GCommonGame->GetSystem< CFactsDB >()->QueryLatestValue( id );
	}
	RETURN_INT ( value );
}

void funcFactsDoesExist( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String, id, String::EMPTY );
	FINISH_PARAMETERS;

	Bool doesExist = false;
	if ( GCommonGame && GCommonGame->GetSystem< CFactsDB >() && id != String::EMPTY )
	{	
		doesExist = GCommonGame->GetSystem< CFactsDB >()->DoesExist( id );
	}
	RETURN_BOOL ( doesExist );
}

void funcFactsRemove( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String, id, String::EMPTY );
	FINISH_PARAMETERS;

	if ( GCommonGame && GCommonGame->GetSystem< CFactsDB >() && id != String::EMPTY )
	{	
		GCommonGame->GetSystem< CFactsDB >()->RemoveFact( id );
	}
}

#define FACTSDB_SCRIPT( x )	\
	NATIVE_GLOBAL_FUNCTION( #x, func##x );

void RegisterFactsDBScriptFunctions()
{
	FACTSDB_SCRIPT( FactsAdd );
	FACTSDB_SCRIPT( FactsQuerySum );
	FACTSDB_SCRIPT( FactsQuerySumSince );
	FACTSDB_SCRIPT( FactsQueryLatestValue );
	FACTSDB_SCRIPT( FactsDoesExist );
	FACTSDB_SCRIPT( FactsRemove );
}
