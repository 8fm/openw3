#include "build.h"

#ifndef NO_TEST_FRAMEWORK

// #define DEBUG_PLAYER_POSITION

#include "../../common/engine/gameResource.h"
#include "testFramework.h"
#include "inputRecorder.h"
#include "../core/depot.h"
#include "gameSession.h"
#include "game.h"
#include "renderer.h"
#include "renderFrame.h"
#include "../core/2darray.h"
#include "baseEngine.h"
#include "entity.h"
#include "../core/objectGC.h"
#include "../core/tokenizer.h"

#ifdef DEBUG_PLAYER_POSITION
	FILE* testFile = NULL;
#endif

/////////////////////////////////////////////////
// ITestCase

// Arbitrary value to avoid lots of automatic buffer resizes. 
#define INITIAL_FRAMEDELTA_BUFFERSIZE ( 1024 * 8 )

ITestCase::ITestCase( const STestConfiguration& configuration )
	: m_configuration( configuration )
	, m_successful( true )
{
}

ITestCase* ITestCase::Create( const STestConfiguration& configuration )
{
	switch ( configuration.m_testMode )
	{
	case ETM_Record:
		return new CRecordTestCase( configuration );
	case ETM_Replay:
		return new CReplayTestCase( configuration );

	default:
		return NULL;
	}
}

Bool ITestCase::OnTick( Uint64 tickNumber, Float& gameDelta )
{
	m_currentRelativeTick = tickNumber - m_startingTick;

	return true;
}

void ITestCase::OnStart()
{
	m_startingTick = GEngine->GetCurrentEngineTick();
}

void ITestCase::GenerateDebugFragments( Uint32 x, Uint32 y, CRenderFrame* frame )
{
	if ( !frame )
	{
		return;
	}

	const Uint32 dX = 5;
	const Uint32 dY = 12;
	
	Uint32 row = 2;

	frame->AddDebugScreenFormatedText( x + dX,	y + ++row * dY, TXT( "Starting tick: %llu" ), m_startingTick );
	frame->AddDebugScreenFormatedText( x + dX,	y + ++row * dY, TXT( "Current tick: %llu" ), m_currentRelativeTick );

	frame->AddDebugScreenFormatedText( x + dX,	y + ++row * dY, TXT( "Configuration" ) );
	frame->AddDebugScreenFormatedText( x + dX,	y + ++row * dY, TXT( "Mode: %ls" ), ( m_configuration.m_testMode == ETM_Record ) ? TXT( "Record" ) : TXT( "Replay" ) );
	frame->AddDebugScreenFormatedText( x + dX,	y + ++row * dY, TXT( "Test case name: '%ls'" ), m_configuration.m_name.AsChar() );
	frame->AddDebugScreenFormatedText( x + dX,	y + ++row * dY, TXT( "Loaded game definition file: '%ls'" ), m_configuration.m_gameDefinition.AsChar() );
	frame->AddDebugScreenFormatedText( x + dX,	y + ++row * dY, TXT( "Root test cases directory: '%ls'" ), m_configuration.m_dirPath.AsChar() );
	frame->AddDebugScreenFormatedText( x + dX,	y + ++row * dY, TXT( "Continuous: '%ls'" ), m_configuration.m_continuousTest ? TXT( "Yes" ) : TXT( "No" ) );
}

/////////////////////////////////////////////////
// CRecordTestCase

CRecordTestCase::CRecordTestCase( const STestConfiguration& configuration )
	: ITestCase( configuration )
{
	m_inputRecorder = new CInputRecorder( configuration.m_name );

	m_frameDeltas.Reserve( INITIAL_FRAMEDELTA_BUFFERSIZE );
	m_framePositions.Reserve( INITIAL_FRAMEDELTA_BUFFERSIZE );
}

CRecordTestCase::~CRecordTestCase()
{
	if ( m_inputRecorder )
	{
		delete m_inputRecorder;
		m_inputRecorder = NULL;
	}
}

void CRecordTestCase::OnStart()
{
	ITestCase::OnStart();

#ifdef DEBUG_PLAYER_POSITION
	_wfopen_s( &testFile, TXT( "player_positions_record.txt" ), TXT( "w" ) );
#endif
}

void CRecordTestCase::OnFinish()
{
#ifndef NO_RESOURCE_IMPORT
	CResource::FactoryInfo< C2dArray > info;
	CDirectory* testCaseDir = GDepot->FindPath( m_configuration.m_dirPath.AsChar() );

	if( !testCaseDir )
	{
		WARN_ENGINE( TXT( "Could not find test case directory: %s" ), m_configuration.m_dirPath.AsChar() );
		return;
	}

	// Save Frame information
	C2dArray* frameInfo = info.CreateResource();

	frameInfo->AddColumn( TXT( "FrameNumber" ),	TXT( "NoInfo" ) );
	frameInfo->AddColumn( TXT( "FrameDelta" ),	TXT( "NoInfo" ) );
	frameInfo->AddColumn( TXT( "PosX" ),	TXT( "NoInfo" ) );
	frameInfo->AddColumn( TXT( "PosY" ),	TXT( "NoInfo" ) );
	frameInfo->AddColumn( TXT( "PosZ" ),	TXT( "NoInfo" ) );

	for( Uint32 i = 0; i < m_frameDeltas.Size(); ++i )
	{
		frameInfo->AddRow();

		frameInfo->SetValue( ToString( (Uint32)i ), 0, i );
		frameInfo->SetValue( ToString( m_frameDeltas[ i ] ), 1, i );
		frameInfo->SetValue( ToString( m_framePositions[ i ].X ), 2, i );
		frameInfo->SetValue( ToString( m_framePositions[ i ].Y ), 3, i );
		frameInfo->SetValue( ToString( m_framePositions[ i ].Z ), 4, i );
	}

	if( !frameInfo->SaveAs( testCaseDir, String::Printf( TXT( "%s-frames.csv" ), m_configuration.m_name.AsChar() ) ) )
	{
		WARN_ENGINE( TXT( "FAILED saving frame data for test case '%ls'" ), m_configuration.m_name.AsChar() );
		return;
	}

	// create general info file and save it
	C2dArray* generalInfo = info.CreateResource();

	generalInfo->AddColumn( TXT("GameDefinition"),	TXT( "NoInfo" ) );
	generalInfo->AddColumn( TXT("StartingFrame"),	TXT( "NoInfo" ) );

	generalInfo->AddRow();
	generalInfo->SetValue( GGame->GetGameResource()->GetFile()->GetDepotPath().AsChar(), 0, 0 );
	generalInfo->SetValue( ToString( static_cast< Uint32 >( m_startingTick ) ), 1, 0 );

	if ( !generalInfo->SaveAs( testCaseDir, String::Printf( TXT( "%s.csv" ), m_configuration.m_name.AsChar() ) ) )
	{
		WARN_ENGINE( TXT( "FAILED saving general info for test case '%ls'" ), m_configuration.m_name.AsChar() );
		return;
	}

	// save the input, we already have the inputRecorder created with proper name
	m_inputRecorder->OnSave();
#endif
}

Bool CRecordTestCase::OnTick( Uint64 tickNumber, Float& frameDelta )
{
	ITestCase::OnTick( tickNumber, frameDelta );
	CEntity* ent = GGame->GetPlayerEntity();
	Vector3 playerPosition = Vector3( 0.0f, 0.0f, 0.0f );
	if( ent )
	{
		playerPosition = ent->GetWorldPositionRef();
	}

	if( (Uint32)m_currentRelativeTick < m_frameDeltas.Size() )
	{
		m_frameDeltas[ (Uint32)m_currentRelativeTick ] = frameDelta;
		m_framePositions[ (Uint32)m_currentRelativeTick ] = playerPosition;
	}
	else
	{
		m_frameDeltas.PushBack( frameDelta );
		m_framePositions.PushBack( playerPosition );
	}

	return true;
}

void CRecordTestCase::ProcessInput( const BufferedInput& input )
{
	if ( m_inputRecorder )
	{
		m_inputRecorder->ProcessInput( m_currentRelativeTick, input );
	}
}

/////////////////////////////////////////////////
// CStatsLogger

CStatsLogger::CStatsLogger( Uint64 framesPerLog)
	: m_framesPerLog( framesPerLog )
{
	ResetValues();
}

void CStatsLogger::ResetValues()
{
	m_sumFrameTime = 0.f;
	m_sumRenderTime = 0.f;
	m_sumRenderFenceTime = 0.f;
	m_sumGPUTime = 0.f;
	m_maxGameplayTime = 0.f;
	m_maxRenderTime = 0.f;
	m_maxRenderFenceTime = 0.f;
	m_maxGPUTime = 0.f;

	m_counter = 0;
}

void CStatsLogger::Log( Uint64 tick ) 
{
#ifndef NO_DEBUG_PAGES
	extern IRender* GRender;
	extern volatile Float GLastRenderFrameTime;
	extern volatile Float GLastRenderFenceTime;

	// Accumulate
	Float lastRenderFenceTime = GLastRenderFenceTime;
	m_maxGameplayTime = ::Max<Float>( GEngine->GetLastTimeDeltaUnclamped() - lastRenderFenceTime, m_maxGameplayTime );
	m_sumFrameTime += GEngine->GetLastTimeDeltaUnclamped();

	m_maxRenderFenceTime = ::Max<Float>( lastRenderFenceTime, m_maxRenderFenceTime );
	m_sumRenderFenceTime += lastRenderFenceTime;

	Float lastRenderFrameTime = GLastRenderFrameTime;
	m_maxRenderTime = ::Max<Float>( lastRenderFrameTime, m_maxRenderTime );
	m_sumRenderTime += lastRenderFrameTime;

	if (GRender)
	{
		m_maxGPUTime = ::Max<Float>( GRender->GetLastGPUFrameDuration(), m_maxGPUTime );
		m_sumGPUTime += GRender->GetLastGPUFrameDuration();
	}

	if ( ++m_counter == m_framesPerLog )
	{
		// Count stats
		Float statRenderFenceTime = m_sumRenderFenceTime / (Float)m_framesPerLog;
		Float statGameplayTime = m_sumFrameTime / (Float)m_framesPerLog - statRenderFenceTime;
		Float statRenderTime = m_sumRenderTime / (Float)m_framesPerLog;
		Float statGPUTime = m_sumGPUTime / (Float)m_framesPerLog;

		LOG_CORE( TXT("Tick: %llu | FPS: %1.2f | Times: max/avg | RenderFrame: %1.3f/%1.3f ms | GameplayFrame: %1.3f/%1.3f ms | RenderFence: %1.3f/%1.3f ms | RenderGPU: %1.3f/%1.3f ms"),  \
			tick, GEngine->GetLastTickRate(), m_maxRenderTime * 1000.0f, statRenderTime * 1000.0f, \
			m_maxGameplayTime * 1000.0f, statGameplayTime * 1000.0f, m_maxRenderFenceTime * 1000.0f, statRenderFenceTime * 1000.0f, m_maxGPUTime, statGPUTime );

		ResetValues();
	}
#endif // NO_DEBUG_PAGES
}

/////////////////////////////////////////////////
// CReplayTestCase

CReplayTestCase::CReplayTestCase( const STestConfiguration& configuration )
	: ITestCase( configuration )/*, IDebugChannelReciever(true)*/
	, statsLogger( 10 )
{
#ifdef DEBUG_REPLAY
	m_frameReplayedPositions.Reserve( INITIAL_FRAMEDELTA_BUFFERSIZE );
#endif
}

Bool CReplayTestCase::Initialize()
{
	return LoadConfig() && LoadFrames() && LoadRawInput();
}

void CReplayTestCase::OnFinish()
{
	RunFinishingScripts();

	if(m_configuration.m_continuousTest == true)
	{
		STestFramework::GetInstance().OnStart( nullptr );
		return;
	}

#if !defined(NO_RESOURCE_IMPORT) && defined(DEBUG_REPLAY)

	CResource::FactoryInfo< C2dArray > info;
	CDirectory* testCaseDir = GDepot->FindDirectory( m_configuration.m_dirPath.AsChar() );

	if( !testCaseDir )
	{
		WARN_ENGINE( TXT( "Could not find test case directory: %s" ), m_configuration.m_dirPath.AsChar() );
		return;
	}

	// Save Frame information
	C2dArray* frameInfo = info.CreateResource();
	frameInfo->AddColumn( TXT( "FrameNumber" ),	TXT( "NoInfo" ) );
	frameInfo->AddColumn( TXT( "PosX" ),	TXT( "NoInfo" ) );
	frameInfo->AddColumn( TXT( "PosY" ),	TXT( "NoInfo" ) );
	frameInfo->AddColumn( TXT( "PosZ" ),	TXT( "NoInfo" ) );

	for( Uint32 i = 0; i < m_frameReplayedPositions.Size(); ++i )
	{
		frameInfo->AddRow();

		frameInfo->SetValue( ToString( (Uint32)i ), 0, i );
		frameInfo->SetValue( ToString( m_frameReplayedPositions[ i ].X ), 1, i );
		frameInfo->SetValue( ToString( m_frameReplayedPositions[ i ].Y ), 2, i );
		frameInfo->SetValue( ToString( m_frameReplayedPositions[ i ].Z ), 3, i );
	}

	if( !frameInfo->SaveAs( testCaseDir, String::Printf( TXT( "%s-frames(Replay).csv" ), m_configuration.m_name ) ) )
	{
		WARN_ENGINE( TXT( "FAILED saving frame data for test case '%ls'" ), m_configuration.m_name );
		return;
	}
#endif
}

Bool CReplayTestCase::RunFinishingScripts()
{
	TDynArray< Uint8 > stackBuffer;

	for ( Uint32 i = 0; i < m_finishingScripts.Size(); ++i )
	{
		CName functionName( m_finishingScripts[ i ] );
		CFunction* function = SRTTI::GetInstance().FindGlobalFunction( functionName );

		if( function )
		{
			if( function->IsExec() )
			{
				stackBuffer.Resize( function->GetStackSize() );

				CPropertyDataBuffer retData( function->GetReturnValue() );
				function->Call( NULL, stackBuffer.Data(), retData.Data() );
				CProperty* prop = function->GetReturnValue();

				if( prop && prop->GetType()->GetName() == CNAME( Bool ) )
				{
					if( ! *( static_cast< Bool* >( retData.Data() ) ) )
					{
						m_successful = false;
						return false;
					}
				}
				else
				{
					WARN_ENGINE( TXT( "Function '%ls' does not return a boolean value" ), m_finishingScripts[ i ].AsChar() );
				}
			}
			else
			{
				WARN_ENGINE( TXT( "Function '%ls' is not an EXEC function" ), m_finishingScripts[ i ].AsChar() );
			}
		}
		else
		{
			WARN_ENGINE( TXT( "Function '%ls' Does not exist" ), m_finishingScripts[ i ].AsChar() );
		}
	}

	return true;
}

Bool CReplayTestCase::LoadRawInput()
{
	String rawInputPath = GFileManager->GetDataDirectory() + TXT("test_cases/") + m_configuration.m_name + TXT(".rawinput");
	IFile* file = GFileManager->CreateFileReader( rawInputPath, FOF_AbsolutePath );
	if( file == NULL )
	{
		ERR_ENGINE( TXT( "Can't load the '%ls' raw input for the test framework." ), rawInputPath.AsChar() );
		return false;
	}

	Uint32 mapSize;

	m_maxSavedTickForCurrentTestCase = 0;

	*file << mapSize;

	for( Uint32 i = 0; i < mapSize; ++i )
	{
		Uint64 tickNumber;
		TDynArray< SRecordedInput > inputs;

		*file << tickNumber;
		inputs.Serialize( *file );

		for( Uint32 iInput = 0; iInput < inputs.Size(); ++iInput )
		{
			AddToMap( m_rawInput, tickNumber, inputs[ iInput ] );
		}

		if ( m_maxSavedTickForCurrentTestCase < tickNumber )
		{
			m_maxSavedTickForCurrentTestCase = tickNumber;
		}
	}

	delete file;

	return true;
}

Bool CReplayTestCase::LoadConfig()
{
	String configFileName = String::Printf( TXT( "%ls%ls.csv" ), GDepot->FindPath( m_configuration.m_dirPath.AsChar() )->GetAbsolutePath().AsChar(), m_configuration.m_name.AsChar() );
	C2dArray* configArray = C2dArray::CreateFromString( configFileName );
	if( configArray == NULL )
	{
		ERR_ENGINE( TXT( "Can't load the '%ls' config for the test framework." ), configFileName.AsChar() );
		return false;
	}

	// get game definition name
	if ( m_configuration.m_gameDefinition == String::EMPTY ) 
	{
		m_configuration.m_gameDefinition = configArray->GetValue( 0, 0 );
		ASSERT( m_configuration.m_gameDefinition != String::EMPTY );
		GGame->SetupGameResourceFromFile( m_configuration.m_gameDefinition );
	}

	for ( Uint32 i = 1; i < configArray->GetNumberOfRows(); ++i )
	{
		String scriptToExecute = configArray->GetValue( 0, i );
		if ( scriptToExecute.Size() /* && GScriptingSystem->IsScriptCallValid( scriptToExecute ) */ )
		{
			m_finishingScripts.PushBack( scriptToExecute );
		}
	}
	return true;
}

Bool CReplayTestCase::LoadFrames()
{
	String configFileName = String::Printf( TXT( "%ls%ls-frames.csv" ), GDepot->FindPath( m_configuration.m_dirPath.AsChar() )->GetAbsolutePath().AsChar(), m_configuration.m_name.AsChar() );
	C2dArray* configArray = C2dArray::CreateFromString( configFileName );
	if( configArray == NULL )
	{
		ERR_ENGINE( TXT( "Can't load the '%ls' recorded frames for the test framework." ), configFileName.AsChar() );
		return false;
	}

	m_frameRecordedDeltas.Resize( configArray->GetNumberOfRows() );
	m_frameRecordedPositions.Resize( configArray->GetNumberOfRows() );

	for( Uint32 i = 0; i < m_frameRecordedDeltas.Size(); ++i )
	{
		Float delta = 0.1f;
		FromString< Float >( configArray->GetValue( 1, i ), delta );
		m_frameRecordedDeltas[ i ] = delta;

		FromString< Float >( configArray->GetValue( 2, i ), m_frameRecordedPositions[i].X );
		FromString< Float >( configArray->GetValue( 3, i ), m_frameRecordedPositions[i].Y );
		FromString< Float >( configArray->GetValue( 4, i ), m_frameRecordedPositions[i].Z );
	}
	return true;
}

Bool CReplayTestCase::OnTick( Uint64 tickNumber, Float& frameDelta )
{
	ITestCase::OnTick( tickNumber, frameDelta );
	statsLogger.Log( tickNumber );

	Uint32 frameIdx = static_cast< Uint32 >( m_currentRelativeTick );
	Bool isNotFinished = IsNotFinished();
	if ( isNotFinished ) 
	{
		frameDelta = m_frameRecordedDeltas[ frameIdx ];

		if ( m_configuration.m_holdPosition ) 
		{
			HoldPosition( frameIdx );
		}
	}

#ifdef DEBUG_REPLAY
	Vector3 playerPosition = Vector3( 0.0f, 0.0f, 0.0f );
	CEntity* ent = GGame->GetPlayerEntity();
	if( ent )
	{
		playerPosition = ent->GetWorldPositionRef();
	}
	if( (Uint32)m_currentRelativeTick < m_frameReplayedPositions.Size() )
	{
		m_frameReplayedPositions[ (Uint32)m_currentRelativeTick ] = playerPosition;
	}
	else
	{
		m_frameReplayedPositions.PushBack( playerPosition );
	}
#endif

	return isNotFinished;
}

Bool CReplayTestCase::OnTickInput()
{
	const TDynArray< SRecordedInput >* bufferedInputForFrame = ProvideRawInput( m_currentRelativeTick );
	TDynArray< IRawInputListener* >& listeners = SRawInputManager::GetInstance().GetListeners();

	if( bufferedInputForFrame )
	{
		for( Uint32 i = 0; i < bufferedInputForFrame->Size(); ++i )
		{
			TDynArray< IRawInputListener* >::iterator end = listeners.End();
			for( TDynArray< IRawInputListener* >::iterator it = listeners.Begin(); it != end; ++it )
			{
				IRawInputListener* listener = *it;
				if ( listener )
				{
					BufferedInput oneEntry(1);
					oneEntry[0] = ( *bufferedInputForFrame )[ i ];

					// If one of the listeners returns true, don't pass to others. Assume input is consumed
					if( listener->ProcessInput( oneEntry ) )
					{
						break;
					}
				}
			}
		}
	}

	return IsNotFinished();
}

void CReplayTestCase::HoldPosition( Uint32 frameIdx ) 
{
	if( CEntity* ent = GGame->GetPlayerEntity() )
	{
		Vector3 playerPosition = ent->GetWorldPositionRef();
		if ( ( playerPosition - m_frameRecordedPositions[ frameIdx ] ).SquareMag() > 9 ) 
		{
			EulerAngles angle( 0, 0, ( m_frameRecordedPositions[ frameIdx ] - m_frameRecordedPositions[ frameIdx - 1 ] ).Yaw() );
			ent->Teleport( m_frameRecordedPositions[ frameIdx ], angle );
		}
	}
}

/////////////////////////////////////////////////
// CTestFrameworkGCScheduler

CTestFrameworkGCScheduler::CTestFrameworkGCScheduler()
	: m_performGC( false ) 
	, m_GCCount( 0 )
{
}

Uint32 CTestFrameworkGCScheduler::RequestGC() 
{
	m_performGC = true;
	m_GCCount = GObjectGC->GetGCCount();
	return m_GCCount;
}

Bool CTestFrameworkGCScheduler::WasGCPerformed( Uint32 requestId ) 
{
	return requestId < GObjectGC->GetGCCount();
}

void CTestFrameworkGCScheduler::PerformRequests()
{
	if ( m_performGC ) 
	{
		if ( !WasGCPerformed( m_GCCount ) ) 
		{
			PC_SCOPE_LV0( TestFrameworkGarbageCollection, PBC_CPU );
			GObjectGC->CollectNow();
		}
		m_performGC = false;
	}
}

/////////////////////////////////////////////////
// CTestFramework

CTestFramework::CTestFramework()
	: m_activeTestCase( NULL )
	, m_continuousTestMode( false )
	, m_shutdownOnFinish( false )
	, m_viewport( nullptr )
	, m_state( TXT("<Begin>" ) )
{
}

CTestFramework::~CTestFramework()
{
	if ( m_activeTestCase )
	{
		m_activeTestCase->OnFinish();
		delete m_activeTestCase;
		m_activeTestCase = NULL;
	}
}

void CTestFramework::Tick( Uint64 tickNumber, Float& frameDelta )
{
	PC_SCOPE_LV0( TestFrameworkTick, PBC_CPU );
	ASSERT( IsActive(), TXT( "TestFramework is not active, check IsActive() prior to calling Tick()" ) );
	
	m_GCScheduler.PerformRequests();

	frameDelta = floor( frameDelta * 1000 ) / 1000;

	if( tickNumber == 0 && frameDelta == 0.0f )
	{
		frameDelta = 1.0f / 30.0f;
	}

	if ( !m_activeTestCase->OnTick( tickNumber, frameDelta ) )
	{
		Int32 programReturnValue = ( OnFinish() )? 0 : 1;

		if( m_shutdownOnFinish )
		{
			GGame->RequestGameEnd();

			GEngine->RequestExit( programReturnValue );
		}
	}
#ifdef DEBUG_PLAYER_POSITION
	else
	{
		CEntity* ent = GGame->GetPlayerEntity();
		if( ent )
		{
			Vector pos = ent->GetWorldPositionRef();
			fwprintf_s( testFile, TXT( "Tick %4u, Delta: % .5f, X: % .5f, Y: % .5f, Z: % .5f, W: % .5f\n" ), (Uint32)tickNumber, frameDelta, pos.X, pos.Y, pos.Z, pos.W );
		}
		else
		{
			fwprintf_s( testFile, TXT( "! No player entity\n" ) );
		}
	}
#endif
}

Bool CTestFramework::TickInput()
{
	PC_SCOPE_LV0( TestFrameworkTickInput, PBC_CPU );
	if( m_activeTestCase )
	{
		if ( m_viewport ) 
		{
			m_viewport->Activate();
		}
		return m_activeTestCase->OnTickInput();
	}

	return false;
}

Bool CTestFramework::ProcessInput( const BufferedInput& input )
{
	PC_SCOPE_LV0( TestFrameworkProcessInput, PBC_CPU );
	if ( m_activeTestCase )
	{
		m_activeTestCase->ProcessInput( input );
	}

	return false;
}
/*
void CTestFramework::PushGameEvent( const SGameEvent& gameEvent )
{
	if ( m_activeTestCase )
	{
		fwprintf_s
		(
			piefile,
			TXT( " Pushed Game Event: TimeToActivation: % .5f, DurationIdleTime: % .5f, Value: % .5f, Name: %s\n" ),
			gameEvent.timeToActivation,
			gameEvent.durationIdleTime,
			gameEvent.value,
			gameEvent.name.AsChar()
		);
	}
}

void CTestFramework::RemoveGameEvent( const SGameEvent& gameEvent )
{
	if ( m_activeTestCase )
	{
		fwprintf_s
		(
			piefile,
			TXT( "Removed Game Event: TimeToActivation: % .5f, DurationIdleTime: % .5f, Value: % .5f, Name: %s\n" ),
			gameEvent.timeToActivation,
			gameEvent.durationIdleTime,
			gameEvent.value,
			gameEvent.name.AsChar()
		);
	}
}*/

void CTestFramework::OnStart( IViewport* viewport )
{
	PC_SCOPE_LV0( TestFrameworkOnStart, PBC_CPU );
	if ( m_activeTestCase )
	{
		if ( viewport != nullptr )
		{
			m_viewport = viewport;
		}
		GEngine->GetRandomNumberGenerator().Seed( 0 );

		m_activeTestCase->OnStart();
	}
}

Bool CTestFramework::OnFinish()
{
	PC_SCOPE_LV0( TestFrameworkOnFinish, PBC_CPU );
	Bool successfulTest = true;

	if ( m_activeTestCase )
	{
		m_activeTestCase->OnFinish();

		successfulTest = m_activeTestCase->WasSuccessful();

		if(m_continuousTestMode == false)
		{
			delete m_activeTestCase;
			m_activeTestCase = NULL;
		}
		
#ifdef DEBUG_PLAYER_POSITION
		fclose( testFile );
		testFile = NULL;
#endif
	}

	return successfulTest;
}

Bool CTestFramework::CommandlineStartsTestFramework( const String& commandLine )
{
	CTokenizer tok( commandLine, TXT(" ") );

	for ( Uint32 i = 0; i < tok.GetNumTokens(); ++i )
	{
		String token = tok.GetToken( i );

		if ( token == TXT( "-test" ) )
			return true;
	}
	return false;
}

Bool CTestFramework::ParseCommandline( const String& commandLine )
{
	PC_SCOPE_LV0( TestFrameworkParseCommandline, PBC_CPU );
	CTokenizer tok( commandLine, TXT(" ") );

	// at this point it is known, that new test case will be recorded or replayed,
	// prepare the configuration
	STestConfiguration configuration;
	configuration.m_valid = false;
	configuration.m_monkey = false;
	configuration.m_continuousTest = false;
	configuration.m_holdPosition = false;
	m_allowVideos = false;
	m_continuousTestMode = false;
	
	/*
	tokens:
	test -record TestCaseName -additionalparams
	test -replay TestCaseName -additionalparams
	test -finish
	*/

	// Track if any of the tokens are related to tests. If they are not, then the params are still valid (we just won't run tests)
	Bool testTokensParsed = false;
	for ( Uint32 i = 0; i < tok.GetNumTokens(); ++i )
	{
		String token = tok.GetToken( i );

		if ( token == TXT( "-test" ) )
		{
			testTokensParsed = true;
			configuration.m_valid = true;

			auto testDir = GDepot->FindPath( TXT("test_cases\\") );
			if( testDir == nullptr )
			{
				ERR_ENGINE( TXT( "Can't find the 'test_cases' folder" ) );
				return false;
			}
			configuration.m_dirPath = GDepot->FindPath( TXT("test_cases\\") )->GetDepotPath();

			String commandToken = tok.GetToken( ++i );

			if ( commandToken == TXT( "-record" ) )
			{
				String testNameToken = tok.GetToken( ++i );
				ASSERT( !m_activeTestCase );

				//configuration.m_gameDefinition = GGame->GetGameResource()->GetFile()->GetFileName().AsChar();
				configuration.m_gameDefinition = tok.GetToken( ++i );
				//if(GIsEditorGame)
				//	configuration.m_world = GGame->GetActiveWorld()->GetDepotPath();
				//else
				//	configuration.m_world = SGameSessionManager::GetInstance().GetGameInfo().m_worldFileToLoad;
				configuration.m_name = testNameToken;
				configuration.m_testMode = ETM_Record;
			}

			else if ( commandToken == TXT( "-replay" ) )
			{
				String testNameToken = tok.GetToken( ++i );
				ASSERT( !m_activeTestCase );

				configuration.m_name = testNameToken;
				configuration.m_testMode = ETM_Replay;
			}

			else if ( commandToken == TXT("-finish") )
			{
				configuration.m_valid = false;
				ASSERT( m_activeTestCase );

				OnFinish();
			}
		}

		else if ( token == TXT( "-monkey" ) )
		{
			testTokensParsed = true;
			configuration.m_monkey = true;
		}

		else if ( token == TXT( "-continuousTest" ) )
		{
			testTokensParsed = true;
			configuration.m_continuousTest = true;
			m_continuousTestMode = true;
		}

		else if ( token == TXT( "-shutdownonfinish" ) )
		{
			testTokensParsed = true;
			m_shutdownOnFinish = true;
		}

		else if ( token == TXT("-allowVideos") )
		{
			m_allowVideos = true;
		}

		else if ( token.EndsWith( TXT(".redgame") ) )
		{
			configuration.m_gameDefinition = token;
		}

		else if ( token == TXT( "-holdPosition" ) )
		{
			configuration.m_holdPosition = true;
		}
	}

	if ( testTokensParsed && configuration.m_valid )
	{
		OnFinish();
		m_activeTestCase = ITestCase::Create( configuration );
		return m_activeTestCase ? m_activeTestCase->Initialize() : false;
	}
	else if( !testTokensParsed )
	{
		return true;	// No testing params passed
	}

	return false;
}

void CTestFramework::ReportState( const String& newState ) 
{
	PC_SCOPE_LV0( TestFrameworkReportState, PBC_CPU );
	m_state = newState;
	LOG_CORE( TXT( "New state has been reached: %ls" ), m_state.AsChar() );
}

void CTestFramework::GenerateDebugFragments( CRenderFrame* frame )
{
	PC_SCOPE_LV0( TestFrameworkGenerateDebugFragments, PBC_CPU );
	Int32 x = frame->GetFrameOverlayInfo().m_width - 300;
	Int32 y = 33;

	Int32 width = 250;
	Int32 height = 50;
	if ( m_activeTestCase )
	{
		height = 200;
	}

	if ( frame )
	{
		frame->AddDebugFrame( x, y, width, height, Color::WHITE );
		frame->AddDebugScreenFormatedText( x + 5, y + 12, TXT(" TEST FRAMEWORK [%ls]"), m_activeTestCase ? TXT( "ON" ) : TXT( "OFF" ) );
		frame->AddDebugScreenFormatedText( x, y + 24, TXT(" State: %ls"), m_state.AsChar() );

		if ( m_activeTestCase )
		{
			m_activeTestCase->GenerateDebugFragments( x, y, frame );
		}
	}
}

#endif // NO_TEST_FRAMEWORK
