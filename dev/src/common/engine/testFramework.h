/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#ifndef NO_TEST_FRAMEWORK

#include "inputRecorder.h"
#include "rawInputManager.h"
#include "viewport.h"

//#define DEBUG_REPLAY

class CRenderFrame;
class CDirectory;

enum ETestMode
{
	ETM_Record = 0,
	ETM_Replay,
};

struct STestConfiguration
{
	String		m_dirPath;
	String		m_name;				//! test case name
	String		m_gameDefinition;	//! options to load = world, start waypoint, etc.
	ETestMode	m_testMode;			//! 
	Bool		m_monkey;			//! random input is provided (valid only when recording)
	Bool		m_valid;
	Bool		m_continuousTest;	//! looped test (valid only when playback)
	Bool		m_holdPosition;		//! should replay try to correct position during execution
};

class ITestCase
{
protected:
	STestConfiguration				m_configuration;		//! config for the test case
	Uint64							m_startingTick;
	Uint64							m_currentRelativeTick;
	Bool							m_successful;

public:
	ITestCase( const STestConfiguration& configuration );
	virtual ~ITestCase()														{ /* intentionally empty */ }

	virtual Bool OnTick( Uint64 tickNumber, Float& gameDelta );
	virtual Bool OnTickInput()													{ return false; }
	virtual void OnStart();
	virtual void OnFinish()														{ /* intentionally empty */ }
	virtual Bool Initialize()													{ return true; }

	virtual String GetActivity() const											{ return String::EMPTY; }

	RED_INLINE Bool WasSuccessful() const										{ return m_successful; }
	RED_INLINE const STestConfiguration& GetTestConfiguration() const			{ return m_configuration; }

	virtual void ProcessInput( const BufferedInput& input )						{ /* intentionally empty */ }
	virtual void GenerateDebugFragments( Uint32 x, Uint32 y, CRenderFrame* frame );

	RED_INLINE String GetFullReportPath() const
	{
		return String::Printf( TXT("%s%s_report.txt"), m_configuration.m_dirPath.AsChar(), m_configuration.m_name.AsChar() );
	}

	RED_INLINE Uint64 GetCurrentTick() const { return m_currentRelativeTick; }

	static ITestCase* Create( const STestConfiguration& configuration );
};

class CRecordTestCase : public ITestCase
{
public:
	CRecordTestCase( const STestConfiguration& configuration );
	virtual ~CRecordTestCase();
	
	virtual void ProcessInput( const BufferedInput& input );

	virtual void OnStart();
	virtual void OnFinish();
	virtual Bool OnTick( Uint64 tickNumber, Float& gameDelta );

	virtual String GetActivity() const { return String::Printf( TXT( "Recording TestCase '%ls' " ), m_configuration.m_name.AsChar() ); }

private:
	CInputRecorder*					m_inputRecorder;
	TDynArray< Float >				m_frameDeltas;
	TDynArray< Vector3 >		    m_framePositions;
};

class CStatsLogger
{
private:
	Float m_sumFrameTime;
	Float m_sumRenderTime;
	Float m_sumRenderFenceTime;
	Float m_sumGPUTime;

	Float m_maxGameplayTime;
	Float m_maxRenderTime;
	Float m_maxRenderFenceTime;
	Float m_maxGPUTime;

	Uint64 m_framesPerLog;
	Uint64 m_counter;

	void ResetValues();
public:
	CStatsLogger( Uint64 framesPerLog );
	void Log( Uint64 tick );
};

class CReplayTestCase : public ITestCase//, public IDebugChannelReciever
{
public:
	CReplayTestCase( const STestConfiguration& configuration );
	virtual ~CReplayTestCase()																				{ /* intentionally empty */ }

	RED_INLINE const TDynArray< SRecordedInput >*	ProvideRawInput( Uint64 tickNumber ) const				{ return m_rawInput.FindPtr( tickNumber ); }

	virtual Bool Initialize();
	virtual Bool OnTick( Uint64 tickNumber, Float& gameDelta );
	virtual Bool OnTickInput();
	virtual void OnFinish();

	virtual String GetActivity() const { return String::Printf( TXT( "Replaying TestCase '%ls' " ), m_configuration.m_name.AsChar() ); }

	//virtual Bool ProcessDebugRequest( CName requestType, const CNetworkPacket& packet, CNetworkPacket& response );

private:
	typedef THashMap< Uint64, TDynArray< SRecordedInput > > TRawInputMap;

	TRawInputMap		 m_rawInput;
	TDynArray< Float >	 m_frameRecordedDeltas;
	TDynArray< Vector3 > m_frameRecordedPositions;
#ifdef DEBUG_REPLAY
	TDynArray< Vector3 > m_frameReplayedPositions;
#endif
	CStatsLogger statsLogger;

	TDynArray< String >  m_finishingScripts;

	Uint64 m_maxSavedTickForCurrentTestCase;

	Bool LoadRawInput();
	Bool LoadConfig();
	Bool LoadFrames();

	Bool RunFinishingScripts();
	void CleanPendingGameEvents();

	void CreateSocketForExitRequest();
	void HoldPosition( Uint32 frameIdx );

	RED_INLINE Bool IsNotFinished() const { return m_maxSavedTickForCurrentTestCase >= m_currentRelativeTick; }

	template< typename T >
	void AddToMap( THashMap< Uint64, TDynArray< T > >& itemMap, Uint64 key, const T& item ) const
	{
		TDynArray< T >* itemArray = itemMap.FindPtr( key );

		if ( itemArray )
		{
			itemArray->PushBack( item );
		}
		else
		{
			TDynArray< T > newItemArray;
			newItemArray.PushBack( item );
			itemMap.Insert( key, newItemArray );
		}
	}
};

class CTestFrameworkGCScheduler
{
private:
	Bool	m_performGC;
	Uint32	m_GCCount;
public:
	CTestFrameworkGCScheduler();

	// Returns request id
	Uint32 RequestGC();
	// Checks whether the request with given id was performed
	Bool WasGCPerformed( Uint32 requestId );

	void PerformRequests();
};

class CTestFramework : public IRawInputListener
{
public:
	CTestFramework();
	~CTestFramework();
	
	void OnStart( IViewport* viewport );
	Bool OnFinish();

	void Tick( Uint64 tickNumber, Float& frameDelta );
	Bool TickInput();

	RED_INLINE Bool IsActive() { return m_activeTestCase != NULL; } const
	RED_INLINE Bool AllowVideos() { return m_allowVideos; } const
	RED_INLINE void ContinuousTestOff() { m_continuousTestMode = false; }

	virtual Bool ProcessInput( const BufferedInput& input );

	Bool ParseCommandline( const String& commandLine );

	void ReportState( const String& newState );
	void GenerateDebugFragments( CRenderFrame* frame );

	RED_INLINE Bool IsFullDeterminism() const { static Bool g_fullDeterminism = true; return g_fullDeterminism; }

	RED_INLINE CTestFrameworkGCScheduler& GetGCScheduler() { return m_GCScheduler; }

	RED_INLINE Uint64 GetCurrentTick() const { return m_activeTestCase->GetCurrentTick(); }

	Bool CommandlineStartsTestFramework( const String& commandLine );

private:
	IViewport*					m_viewport;
	ITestCase*					m_activeTestCase;

	Bool						m_continuousTestMode;
	Bool						m_shutdownOnFinish;		//! Once the test(s) have finished, close the game
	Bool						m_allowVideos;

	String	m_state;

	CTestFrameworkGCScheduler	m_GCScheduler;
};

/// Test Framework singleton
typedef TSingleton< CTestFramework > STestFramework;

#endif // NO_TEST_FRAMEWORK
