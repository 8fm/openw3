/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "debugPageParam.h"
#include "debugCommandBox.h"
#include "renderCommands.h"
#include "../core/depot.h"
#include "../core/garbageCollector.h"

#ifndef NO_DEBUG_WINDOWS
#include "debugWindowsManager.h"
#endif
#include "../core/gatheredResource.h"
#include "debugPage.h"
#include "debugPageManagerBase.h"
#include "cutscene.h"
#include "cutsceneInstance.h"
#include "gameTime.h"
#include "game.h"
#include "gameTimeManager.h"
#include "soundSystem.h"
#include "renderFrame.h"
#include "../core/2darray.h"
#include "layer.h"
#include "world.h"
#include "dynamicLayer.h"

#ifndef NO_DEBUG_PAGES

//////////////////////////////////////////////////////////////////////////

extern Bool GImmediateDeploymentEntitiesLoadingJobs;

class CDebugCheckBoxSaveGameplayConfig : public IDebugCommandBox
{
public:
	CDebugCheckBoxSaveGameplayConfig( IDebugCheckBox* parent )
		: IDebugCommandBox( parent, TXT("Save config") )
	{};

	virtual void Process()
	{
		GGame->GetGameplayConfig().Validate();
		GGame->GetGameplayConfig().Save();
	}
};

class CDebugTimeIncCommandBox : public IDebugCommandBox
{
public:
	CDebugTimeIncCommandBox( IDebugCheckBox* parent )
		: IDebugCommandBox( parent, TXT("Inc time mul") )
	{};

	virtual void Process()
	{
		GGame->SetOrRemoveTimeScale( ::Min( GGame->GetTimeScale() * 2.f, 8.0f ), CNAME( DebugTimeScale ), 0x7FFFFFFF );
	}
};

class CDebugTimeDecCommandBox : public IDebugCommandBox
{
public:
	CDebugTimeDecCommandBox( IDebugCheckBox* parent )
		: IDebugCommandBox( parent, TXT("Dec time mul") )
	{};

	virtual void Process()
	{
		GGame->SetOrRemoveTimeScale( ::Max( GGame->GetTimeScale() * 0.5f, 0.03125f ), CNAME( DebugTimeScale ), 0x7FFFFFFF );
	}
};

class CDebugTogglePauseCommandBox : public IDebugCommandBox
{
public:
	CDebugTogglePauseCommandBox( IDebugCheckBox* parent )
		: IDebugCommandBox( parent, TXT("Toggle pause on / off") )
	{};

	virtual void Process()
	{
		if ( GGame->IsPaused() )
		{
			GGame->Unpause( TXT( "CDebugTogglePauseCommandBox" ) );
		}
		else
		{
			GGame->Pause( TXT( "CDebugTogglePauseCommandBox" ) );
		}
	}
};

class CDebugGodModeCommandBox : public IDebugCommandBox
{
public:
	CDebugGodModeCommandBox( IDebugCheckBox* parent )
		: IDebugCommandBox( parent, TXT("God Mode") )
	{};

	virtual void Process()
	{
		CallFunction( NULL, CNAME( GodMode ) );
	}
};

class CDebugKillEnemiesCommandBox : public IDebugCommandBox
{
public:
	CDebugKillEnemiesCommandBox( IDebugCheckBox* parent )
		: IDebugCommandBox( parent, TXT("Kill enemies") )
	{};

	virtual void Process()
	{
		CallFunction( NULL, CNAME( DebugKillEnemies ) );
	}
};

class CDebugIncTimeCommandBox : public IDebugCommandBox
{
public:
	CDebugIncTimeCommandBox( IDebugCheckBox* parent )
		: IDebugCommandBox( parent, TXT("Inc time") )
	{};

	virtual void Process()
	{
		GameTime time = GGame->GetTimeManager()->GetTime();
		time += 60 * 60;
		GGame->GetTimeManager()->SetTime( time, false );
	}

	virtual String GetComment() const 
	{ 
		return String::Printf( TXT("Time: %s"), GGame->GetTimeManager()->GetTime().ToString().AsChar() ); 
	}
};

class CDebugDecTimeCommandBox : public IDebugCommandBox
{
public:
	CDebugDecTimeCommandBox( IDebugCheckBox* parent )
		: IDebugCommandBox( parent, TXT("Dec time") )
	{};

	virtual void Process()
	{
		GameTime time = GGame->GetTimeManager()->GetTime();
		time -= 60 * 60;
		GGame->GetTimeManager()->SetTime( time, false );
	}

	virtual String GetComment() const 
	{ 
		return String::Printf( TXT("Time: %s"), GGame->GetTimeManager()->GetTime().ToString().AsChar() ); 
	}
};

class CDebugGCCommandBox : public IDebugCommandBox
{
public:
	CDebugGCCommandBox( IDebugCheckBox* parent )
		: IDebugCommandBox( parent, TXT("GarbageCollector") )
	{};

	virtual void Process()
	{
		SGarbageCollector::GetInstance().CollectNow();
	}
};

#ifndef NO_LOG
class CDebugDumpClassMemCommandBox : public IDebugCommandBox
{
public:
	CDebugDumpClassMemCommandBox( IDebugCheckBox* parent )
		: IDebugCommandBox( parent, TXT("DumpClassMem") )
	{};

	virtual void Process()
	{
		RED_MEMORY_DUMP_CLASS_MEMORY_REPORT( "DebugDumpClassMem" );
	}
};

class CDebugDumpPoolMemCommandBox : public IDebugCommandBox
{
public:
	CDebugDumpPoolMemCommandBox( IDebugCheckBox* parent )
		: IDebugCommandBox( parent, TXT("DumpPoolMem") )
	{};

	virtual void Process()
	{
		RED_MEMORY_DUMP_POOL_MEMORY_REPORT( "DebugDumpClassMem" );
	}
};

class CDebugDumpObjectMemClassCommandBox : public IDebugCommandBox
{
public:
	CDebugDumpObjectMemClassCommandBox( IDebugCheckBox* parent )
		: IDebugCommandBox( parent, TXT("DumpObjectClassMem") )
	{};

	virtual void Process()
	{
		CObject::DebugDumpClassList();
	}
};

class CDebugDumpObjectMemCommandBox : public IDebugCommandBox
{
public:
	CDebugDumpObjectMemCommandBox( IDebugCheckBox* parent )
		: IDebugCommandBox( parent, TXT("DumpObjectMem") )
	{};

	virtual void Process()
	{
		CObject::DebugDumpList();
	}
};
#endif


class CDebugDumpCNamesCommandBox : public IDebugCommandBox
{
public:
	CDebugDumpCNamesCommandBox( IDebugCheckBox* parent )
		: IDebugCommandBox( parent, TXT("DumpCNames") )
	{};

	virtual void Process()
	{
		//FIXME>>>>>>>
		//CName::DebugDumpPool();
	}
};

class CDebugPhysicsMovementCommandBox : public IDebugCommandBox
{
public:
	CDebugPhysicsMovementCommandBox( IDebugCheckBox* parent )
		: IDebugCommandBox( parent, TXT("TogglePhysicalMovement") )
	{};

	virtual void Process()
	{
		extern void GDebugPageTogglePhysicalMovement();
		GDebugPageTogglePhysicalMovement();
	}
};

CGatheredResource resCutsceneArrayDef2( TXT("gameplay\\globals\\cutscene.csv"), RGF_Startup );

class CDebugCsCommandBox : public IDebugCommandBox
{
public:
	CDebugCsCommandBox( IDebugCheckBox* parent )
		: IDebugCommandBox( parent, TXT("Cs") )
	{};

	virtual void Process()
	{
		SGarbageCollector::GetInstance().CollectNow();

		static Int32 counter = 0;

		// Get cutscene resource from cs array
		C2dArray* csArray = resCutsceneArrayDef2.LoadAndGet< C2dArray >();
		ASSERT( csArray );

		if ( !csArray )
		{
			WARN_ENGINE( TXT("PlayCutscene: Couldn't find 2d array with cs definitions. Fail.") );
			return;
		}

		String csName = csArray->GetValue( TXT("Name"), counter++ );
		while ( !csName.BeginsWith( TXT("fin") ) )
		{
			csName = csArray->GetValue( TXT("Name"), counter++ );
		}

		const String fileParh = csArray->GetValue( TXT("Name"), csName, TXT("Resource") );
		if ( fileParh.Empty() )
		{
			WARN_ENGINE( TXT("PlayCutscene: Couldn't find file path for '%ls' cutscene. Fail."), csName.AsChar() );
			return;
		}

		// Get cutscene resource template
		CCutsceneTemplate* csTempl = Cast< CCutsceneTemplate >( GDepot->LoadResource( fileParh ) );
		if ( !csTempl )
		{
			WARN_ENGINE( TXT("PlayCutscene: Couldn't find resource '%ls' for '%ls' cutscene. Fail."), fileParh.AsChar(), csName.AsChar() );
			return;
		}

		// Destination point
		Matrix dest = Matrix::IDENTITY;

		// Get dynamic layer
		CLayer* layer = GGame->GetActiveWorld()->GetDynamicLayer();
		ASSERT( layer );

		// Create actors map
		THashMap< String, CEntity* > actors;

		String errors;

		// Create cutscene instance
		CCutsceneInstance* csInstance = csTempl->CreateInstance( layer, dest, errors, actors );
		if ( !csInstance )
		{
			errors = String::Printf( TXT("PlayCutscene: Couldn't find resource '%ls' for '%ls' cutscene. Fail.\n"), fileParh.AsChar(), csName.AsChar() ) + errors;
			return;
		}

		// Start cutscene playing. Auto-update is true. Auto-destroy is true.
		csInstance->Play( true, true, false, 0 );
	}
};

//////////////////////////////////////////////////////////////////////////

/// Debug page for gameplayConfig
class CDebugPageGameplayConfig : public IDebugPage
{
public:
	CDebugOptionsTree*			m_tree;

public:
	CDebugPageGameplayConfig()
		: IDebugPage( TXT("Gameplay Config") )
		, m_tree( NULL )
	{
		
	};

	~CDebugPageGameplayConfig()
	{
		delete m_tree;
	}

	virtual void OnViewportGenerateFragments( IViewport *view, CRenderFrame *frame )
	{	
#ifndef NO_DEBUG_WINDOWS
		String message = TXT("This debug page is destined to remove. Click key 'Enter' to open new debug framework.");

		frame->AddDebugRect(49, 80, 502, 20, Color(100,100,100,255));
		frame->AddDebugScreenFormatedText( 60, 95, Color(255, 255, 255, 255), TXT("Debug Message Box"));

		frame->AddDebugRect(50, 100, 500, 50, Color(0,0,0,255));
		frame->AddDebugFrame(50, 100, 500, 50, Color(100,100,100,255));

		frame->AddDebugScreenFormatedText( 70, 120, Color(255, 0, 0, 255), message.AsChar());

		frame->AddDebugScreenFormatedText( 275, 140, Color(255, 255, 255), TXT(">> OK <<"));
		return;
#endif

		// Create tree
		if ( !m_tree )
		{
			const Uint32 width = frame->GetFrameOverlayInfo().m_width - 100;
			const Uint32 height = frame->GetFrameOverlayInfo().m_height - 80;

			m_tree = new CDebugOptionsTree( 50, 50, width, height, this );

			// Animated Properties
			{
				IDebugCheckBox* workGroup = new IDebugCheckBox( NULL, TXT("Animated Properties"), true, false );
				m_tree->AddRoot( workGroup );

				new CDebugCheckBoxParam( workGroup, GGame->GetGameplayConfig().m_showSegments, TXT("Show Curves Segments") );
				new CDebugCheckBoxParam( workGroup, GGame->GetGameplayConfig().m_showRotations, TXT("Show Rotations") );
				new CDebugCheckBoxParam( workGroup, GGame->GetGameplayConfig().m_showNodes, TXT("Show Nodes") );
				new CDebugSliderIntParam( workGroup, GGame->GetGameplayConfig().m_curvePrecision, TXT("Curves Precisions"), 1, 100, 2 );
				new CDebugSliderParam( workGroup, GGame->GetGameplayConfig().m_timeScale, TXT("Time Scale"), 0.0f, 5.0f, 0.1f );
			}

			// Animations
			{
				IDebugCheckBox* workGroup = new IDebugCheckBox( NULL, TXT("Animations"), true, false );
				m_tree->AddRoot( workGroup );

				new CDebugCheckBoxParam( workGroup, GGame->GetGameplayConfig().m_animationMultiUpdate, TXT("Animation multi update") );
				new CDebugCheckBoxParam( workGroup, GGame->GetGameplayConfig().m_animationAsyncUpdate, TXT("Animation async update") );
				new CDebugCheckBoxParam( workGroup, GGame->GetGameplayConfig().m_animationAsyncJobs, TXT("Animation async jobs") );
				new CDebugCheckBoxParam( workGroup, GGame->GetGameplayConfig().m_logMissingAnimations, TXT("Log missing anims") );
				new CDebugCheckBoxParam( workGroup, GGame->GetGameplayConfig().m_logRequestedAnimations, TXT("Log all requested anims") );
				new CDebugCheckBoxParam( workGroup, GGame->GetGameplayConfig().m_logSampledAnimations, TXT("Log all sampled anims") );
			}

			// Work
			{
				IDebugCheckBox* workGroup = new IDebugCheckBox( NULL, TXT("Work"), true, false );
				m_tree->AddRoot( workGroup );

				new CDebugCheckBoxParam( workGroup, GGame->GetGameplayConfig().m_useWorkFreezing, TXT("Use freezing") );
				new CDebugCheckBoxParam( workGroup, GGame->GetGameplayConfig().m_workResetInFreezing, TXT("Reset pose in freezing mode") );

				new CDebugSliderParam( workGroup, GGame->GetGameplayConfig().m_workFreezingDelay, TXT("Max time for actor invisibility"), 0.1f, 10.f );
				new CDebugSliderParam( workGroup, GGame->GetGameplayConfig().m_workMaxFreezingTime, TXT("Max freezing time"), 0.f, 10.f );
				new CDebugSliderParam( workGroup, GGame->GetGameplayConfig().m_workFreezingRadiusForInvisibleActors, TXT("Radius for invisible actors"), 0.f, 25.f, 1.f );

				new CDebugCheckBoxParam( workGroup, GGame->GetGameplayConfig().m_streamOnlyVisibleLayers, TXT("streamOnlyVisibleLayers") );

			}

			// Actor lod
			{
				IDebugCheckBox* workGroup = new IDebugCheckBox( NULL, TXT("Actor lod"), true, false );
				m_tree->AddRoot( workGroup );

				new CDebugCheckBoxParam( workGroup, GGame->GetGameplayConfig().m_actorOptUse, TXT("Opt Use") );
				new CDebugSliderIntParam( workGroup, GGame->GetGameplayConfig().m_actorOptDiv, TXT("Opt Div"), 1, 5 );
			}

			// Movement
			{
				IDebugCheckBox* workGroup = new IDebugCheckBox( NULL, TXT("Movement"), true, false );
				m_tree->AddRoot( workGroup );

				new CDebugCheckBoxParam( workGroup, GGame->GetGameplayConfig().m_movementTraceOpt, TXT("Trace opt") );
				new CDebugCheckBoxParam( workGroup, GGame->GetGameplayConfig().m_movementDeltaTestOpt, TXT("Delta test") );
				new CDebugSliderParam( workGroup, GGame->GetGameplayConfig().m_movementSmoothing, TXT("Smoothing for characters"), 0.f, 30.f, 0.5f );
				new CDebugSliderParam( workGroup, GGame->GetGameplayConfig().m_movementSmoothingOnHorse, TXT("Smoothing for horse"), 0.f, 30.f, 0.5f );
			}

			// Behavior
			{
				IDebugCheckBox* workGroup = new IDebugCheckBox( NULL, TXT("Behavior"), true, false );
				m_tree->AddRoot( workGroup );

				new CDebugCheckBoxParam( workGroup, GGame->GetGameplayConfig().m_useBehaviorLod, TXT("Use LOD") );
				new CDebugCheckBoxParam( workGroup, GGame->GetGameplayConfig().m_forceBehaviorLod, TXT("Force LOD") );
				new CDebugSliderIntParam( workGroup, GGame->GetGameplayConfig().m_forceBehaviorLodLevel, TXT("Force Level"), 0, BL_NoLod - 1 );
			}

			// GC
			{
				IDebugCheckBox* workGroup = new IDebugCheckBox( NULL, TXT("GC options"), true, false );
				m_tree->AddRoot( workGroup );

				new CDebugCheckBoxParam( workGroup, GGame->GetGameplayConfig().m_gcAfterCutscenesWithCamera, TXT("Force after cutscenes with camera") );
				new CDebugCheckBoxParam( workGroup, GGame->GetGameplayConfig().m_gcAfterNotGameplayScenes, TXT("Force after not gameplay scenes") );
			}

			// Prog
			{
				IDebugCheckBox* workGroup = new IDebugCheckBox( NULL, TXT("Prog"), true, false );
				m_tree->AddRoot( workGroup );

				new CDebugCheckBoxParam( workGroup, GGame->GetGameplayConfig().m_useMultiTick, TXT("Use multi tick") );
				
			}

			// Items
			{
				IDebugCheckBox* workGroup = new IDebugCheckBox( NULL, TXT("Items"), true, false );
				m_tree->AddRoot( workGroup );

				new CDebugCheckBoxParam( workGroup, GImmediateDeploymentEntitiesLoadingJobs, TXT("Immediate Deployment Items") );
			}

			// Scenes
			{
				IDebugCheckBox* workGroup = new IDebugCheckBox( NULL, TXT("Scenes"), true, false );
				m_tree->AddRoot( workGroup );
				
				new CDebugCheckBoxParam( workGroup, GGame->GetGameplayConfig().m_enableMeshFlushInScenes, TXT( "Flush mesh loading when starting scene" ) );
				new CDebugCheckBoxParam( workGroup, GGame->GetGameplayConfig().m_enableTextureFlushInScenes, TXT( "Flush mesh loading when starting scene" ) );
				new CDebugCheckBoxParam( workGroup, GGame->GetGameplayConfig().m_enableAnimationFlushInScenes, TXT( "Flush animation loading when starting scene" ) );
				new CDebugCheckBoxParam( workGroup, GGame->GetGameplayConfig().m_enableSimplePriorityLoadingInScenes, TXT( "Disable completing io passes by priority during non gameplay scenes" ) );
			}

			// Commands
			{
				m_tree->AddRoot( new CDebugGodModeCommandBox( NULL ) );
				m_tree->AddRoot( new CDebugKillEnemiesCommandBox( NULL ) );
				m_tree->AddRoot( new CDebugIncTimeCommandBox( NULL ) );
				m_tree->AddRoot( new CDebugDecTimeCommandBox( NULL ) );
				m_tree->AddRoot( new CDebugTogglePauseCommandBox( NULL ) );
				m_tree->AddRoot( new CDebugTimeIncCommandBox( NULL ) );
				m_tree->AddRoot( new CDebugTimeDecCommandBox( NULL ) );

				m_tree->AddRoot( new CDebugPhysicsMovementCommandBox( NULL ) );
				
				// For programmers
				{
					IDebugCheckBox* group = new IDebugCheckBox( NULL, TXT("Prog"), true, false );
					m_tree->AddRoot( group );

					new CDebugGCCommandBox( group );
					new CDebugCsCommandBox( group );
#ifndef NO_LOG
					new CDebugDumpClassMemCommandBox( group );
					new CDebugDumpPoolMemCommandBox( group );
					new CDebugDumpObjectMemClassCommandBox( group );
					new CDebugDumpObjectMemCommandBox( group );
#endif
					new CDebugDumpCNamesCommandBox( group );
				}

				m_tree->AddRoot( new CDebugCheckBoxSaveGameplayConfig( NULL ) );
			}

		}

		m_tree->OnRender( frame );
	}

	virtual Bool OnViewportInput( IViewport* view, enum EInputKey key, enum EInputAction action, Float data )
	{
#ifndef NO_RED_GUI
#ifndef NO_DEBUG_WINDOWS
		if( action == IACT_Press && key == IK_Enter)
		{
			GDebugWin::GetInstance().SetVisible(true);
		}
		return true;
#endif	// NO_DEBUG_WINDOWS
#endif	// NO_RED_GUI

		if ( m_tree && m_tree->OnInput( key, action, data ) )
		{
			return true;
		}

		// Not processed
		return false;
	}

	virtual void OnTick( Float timeDelta )
	{
		if ( m_tree )
		{
			m_tree->OnTick( timeDelta );
		}
	}
};

void CreateDebugPageGameplayConfig()
{
	IDebugPage* page = new CDebugPageGameplayConfig();
	IDebugPageManagerBase::GetInstance()->RegisterDebugPage( page );
}

#endif
