/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "../physics/PhysicsRagdollWrapper.h"
#include "debugPageParam.h"
#include "../physics/physicsEngine.h"
#include "../physics/physicsWorld.h"
#include "debugPage.h"
#ifndef NO_DEBUG_PAGES

#include "../physics/physicsSettings.h"
#include "debugPageManagerBase.h"
#include "renderFrame.h"
#ifndef NO_DEBUG_WINDOWS
#include "debugWindowsManager.h"
#include "game.h"
#include "world.h"
#include "inputBufferedInputEvent.h"
#include "../physics/physXEngine.h"
#endif

#if defined(USE_PHYSX) && !defined(NO_EDITOR)
extern void UpdatePhysxSettings();
#endif

/// Debug page with memory status
class CDebugPagePhysics : public IDebugPage
{
private:
	CDebugOptionsTree*		m_tree;

public:
	CDebugPagePhysics()
		: IDebugPage( TXT("Physics") )
		, m_tree( NULL )
	{};

	//! Page shown
	virtual void OnPageShown()
	{
		IDebugPage::OnPageShown();
	}

	~CDebugPagePhysics()
	{
		delete m_tree;
	}

	virtual void OnViewportGenerateFragments( IViewport *view, CRenderFrame *frame )
	{
#ifndef NO_DEBUG_WINDOWS
		String message = TXT("This debug page is converted to debug window. If you want use it, click key: Enter.");

		frame->AddDebugRect(49, 80, 502, 20, Color(100,100,100,255));
		frame->AddDebugScreenFormatedText( 60, 95, Color(255, 255, 255, 255), TXT("Debug Message Box"));

		frame->AddDebugRect(50, 100, 500, 50, Color(0,0,0,255));
		frame->AddDebugFrame(50, 100, 500, 50, Color(100,100,100,255));

		frame->AddDebugScreenFormatedText( 60, 120, Color(127, 255, 0, 255), message.AsChar());

		frame->AddDebugScreenFormatedText( 275, 140, Color(255, 255, 255), TXT(">> OK <<"));
		return;
#endif

		// Create tree
		if ( !m_tree )
		{
			const Uint32 width = frame->GetFrameOverlayInfo().m_width - 100;
			const Uint32 height = frame->GetFrameOverlayInfo().m_height - 100;
			m_tree = new CDebugOptionsTree( 50, 50, width, height, this, false );

			// Physics
			{
				IDebugCheckBox* workGroup = new IDebugCheckBox( NULL, TXT("Physics"), true, false );
				m_tree->AddRoot( workGroup );
#ifdef USE_PHYSX
				new CDebugCheckBoxParam( workGroup, SPhysicsSettings::m_dontCreateTrees, TXT("DontCreateTrees") );
				new CDebugCheckBoxParam( workGroup, SPhysicsSettings::m_dontCreateRagdolls, TXT("DontCreateRagdolls") );
				new CDebugCheckBoxParam( workGroup, SPhysicsSettings::m_dontCreateDestruction, TXT("DontCreateDestruction") );
				
				new CDebugCheckBoxParam( workGroup, SPhysicsSettings::m_dontCreateCloth, TXT("DontCreateCloth") );
				new CDebugCheckBoxParam( workGroup, SPhysicsSettings::m_dontCreateClothOnGPU, TXT("DontCreateClothOnGPU") );
				new CDebugCheckBoxParam( workGroup, SPhysicsSettings::m_dontCreateClothSecondaryWorld, TXT("DontCreateClothSecondaryWorld") );
				
				new CDebugCheckBoxParam( workGroup, SPhysicsSettings::m_dontCreateParticles, TXT("DontCreateParticles") );
				new CDebugCheckBoxParam( workGroup, SPhysicsSettings::m_dontCreateParticlesOnGPU, TXT("DontCreateParticlesOnGPU") );

				new CDebugCheckBoxParam( workGroup, SPhysicsSettings::m_dontCreateCharacterControllers, TXT("DontCreateCharacterControllers") );
				new CDebugCheckBoxParam( workGroup, SPhysicsSettings::m_dontCreateHeightmap, TXT("dontCreateHeightmap") );
				new CDebugCheckBoxParam( workGroup, SPhysicsSettings::m_dontCreateLayerGeometry, TXT("dontCreateStatics") );
				new CDebugCheckBoxParam( workGroup, SPhysicsSettings::m_dontCreateStaticMeshGeometry, TXT("dontCreateStaticMeshGeometry") );

#ifndef NO_EDITOR
				new CDebugSliderIntParam( workGroup, SPhysicsSettings::m_useCpuDefaultDispacherNbCores, TXT("(NO WORLDS) Default Dispacher CPU Count"), 0, 10, 1, UpdatePhysxSettings );
#endif

				new CDebugSliderParam( workGroup, SPhysicsSettings::m_particleSimulationDistanceLimit, TXT("Particle Simulation Distance Limit"), 0.0f, 100.0f, 1.0f );
				
				new CDebugSliderParam( workGroup, SPhysicsSettings::m_ragdollGlobalWindScaler, TXT("Ragdoll Wind Scaler"), 0.0f, 100, 0.5f );
				new CDebugSliderParam( workGroup, SPhysicsSettings::m_clothWindScaler, TXT("Cloth Wind Scaler"), 0.0f, 100, 0.5f );

				new CDebugSliderParam( workGroup, SPhysicsSettings::m_cameraOccludablesRadiusRatio, TXT("Ratio of sweep sphere radius (for fading occludables)."), 0.0f, 1.0f, 0.05f );
				new CDebugSliderParam( workGroup, SPhysicsSettings::m_cameraOccludablesStartRatio, TXT("Ratio of sweep start offset (for fading occludables)."), 0.0f, 1.0f, 0.05f );

				new CDebugSliderParam( workGroup, SPhysicsSettings::m_simpleBodyLinearDamper, TXT("Simple wrapper linear damping"), 0.0f, 10.0f, 0.05f );
				new CDebugSliderParam( workGroup, SPhysicsSettings::m_simpleBodyAngularDamper, TXT("Simple wrapper angular damping"), 0.0f, 10.0f, 0.05f );

				new CDebugSliderParam( workGroup, SPhysicsSettings::m_ragdollJointedLinearDamper, TXT("Ragdoll jointed linear damping"), 0.0f, 10.0f, 0.05f );
				new CDebugSliderParam( workGroup, SPhysicsSettings::m_ragdollJointedAngularDamper, TXT("Ragdoll jointed angular damping"), 0.0f, 10.0f, 0.05f );
				new CDebugSliderParam( workGroup, SPhysicsSettings::m_ragdollChainedLinearDamper, TXT("Ragdoll chained linear damping"), 0.0f, 100.0f, 0.1f );
				new CDebugSliderParam( workGroup, SPhysicsSettings::m_ragdollChainedAngularDamper, TXT("Ragdoll chained angular damping"), 0.0f, 100.0f, 0.1f );
				new CDebugSliderParam( workGroup, SPhysicsSettings::m_ragdollMaxLinearDamper, TXT("Ragdoll max linear damping"), 0.0f, 100.0f, 0.5f );
				new CDebugSliderParam( workGroup, SPhysicsSettings::m_ragdollMaxAngularDamper, TXT("Ragdoll max angular damping"), 0.0f, 100.0f, 0.5f );
				new CDebugSliderParam( workGroup, SPhysicsSettings::m_ragdollMaxSimulationTime, TXT("Ragdoll max simulation time"), 0.0f, 120.0f, 1.0f );
				new CDebugSliderParam( workGroup, SPhysicsSettings::m_ragdollSpeedForSleep, TXT("Ragdoll speed under which will fall to sleep"), 0.0f, 1.0f, 0.01f );
				new CDebugSliderParam( workGroup, SPhysicsSettings::m_ragdollSleepFallSpeed, TXT("Ragdoll speed of falling to sleep"), 0.0f, 10.0f, 0.25f );

				new CDebugSliderParam( workGroup, SPhysicsSettings::m_destructionLinearDamper, TXT("Destruction wrapper linear damping"), 0.0f, 10.0f, 0.05f );
				new CDebugSliderParam( workGroup, SPhysicsSettings::m_destructionAngularDamper, TXT("Destruction wrapper angular damping"), 0.0f, 10.0f, 0.05f );

				new CDebugSliderParam( workGroup, SPhysicsSettings::m_fluidLinearDamping, TXT("Fluid linear damping offset"), -10.0f, 10.0f, 0.05f );
				new CDebugSliderParam( workGroup, SPhysicsSettings::m_fluidAngularDamping, TXT("Fluid angular damping offset"), -10.0f, 10.0f, 0.05f );
				new CDebugSliderParam( workGroup, SPhysicsSettings::m_fluidAngularForceMultipler, TXT("Fluid angular force multipler"), 0.0f, 100.0f, 1.0f );
				new CDebugSliderParam( workGroup, SPhysicsSettings::m_fluidAngularForceMaxClamp, TXT("Fluid angular force max clamp"), 0.0f, 100.0f, 0.1f );
				new CDebugSliderParam( workGroup, SPhysicsSettings::m_fluidAngularPredefinedRadius, TXT("Fluid angular predefined radius"), 0.0f, 2.0f, 0.05f );

				new CDebugCheckBoxParam( workGroup, SPhysicsSettings::m_ragdollJointsProjection, TXT("Joints Projection") );
				new CDebugSliderParam( workGroup, SPhysicsSettings::m_ragdollProjectionLinearTolerance, TXT("Projection Linear Tolerance"), 0.0f, 100.0f, 0.1f );
				new CDebugSliderParam( workGroup, SPhysicsSettings::m_ragdollProjectionAngularTolerance, TXT("Projection Angular Tolerance"), 0.0f, 3.1415926535897932384626433f, 0.05f );

				new CDebugSliderParam( workGroup, SPhysicsSettings::m_destructionSimulationDistanceLimit, TXT("Destruction simulation distance limit"), 0.0f, 1000.f, 5.0f );
				new CDebugSliderParam( workGroup, SPhysicsSettings::m_simpleBodySimulationDistanceLimit, TXT("Dynamics simulation distance limit"), 0.0f, 1000.f, 5.0f );
				new CDebugSliderParam( workGroup, SPhysicsSettings::m_clothSimulationDistanceLimit, TXT("Cloth simulation distance limit"), 0.0f, 50.f, 1.0f );

#ifndef NO_EDITOR
				new CDebugSliderParam( workGroup, SPhysicsSettings::m_actorSleepThreshold, TXT("Actor sleep threshold"), 0.0f, 10.f, 0.05f, UpdatePhysxSettings );
				new CDebugSliderParam( workGroup, SPhysicsSettings::m_destructibleUnfracturedSleepThreshold, TXT("Destructible unfractured actor sleep threshold"), 0.0f, 10.f, 0.05f, UpdatePhysxSettings );
				new CDebugSliderParam( workGroup, SPhysicsSettings::m_actorAngularVelocityLimit, TXT("Actor angular velocity limit"), 0.0f, 10.f, 0.05f, UpdatePhysxSettings );
#endif			

#endif
				workGroup->Expand( true );
			}

			// character controller
			{
				IDebugCheckBox* workGroup = new IDebugCheckBox( NULL, TXT("Character Controller"), true, false );
				m_tree->AddRoot( workGroup );

				new CDebugSliderParam( workGroup, SPhysicsSettings::m_characterPushingMultipler, TXT("Character Pushing Multipler"), 0.0f, 100, 1.0f );
				new CDebugSliderParam( workGroup, SPhysicsSettings::m_characterPushingMaxClamp, TXT("Character Pushing Max Clamp"), 0.0f, 100.0, 1.0f );
				new CDebugSliderParam( workGroup, SPhysicsSettings::m_characterFootstepWaterLevelLimit, TXT("Character Footstep Water Level Limit"), 0.0f, 2.0, 0.01f );
				
				new CDebugSliderParam( workGroup, GGame->GetGameplayConfig().m_terrainInfluenceLimitMin, TXT("Terrain Influence Min Slope Limit"), 0.0f, 1.0f, 0.05f );
				new CDebugSliderParam( workGroup, GGame->GetGameplayConfig().m_terrainInfluenceLimitMax, TXT("Terrain Influence Max Slope Range"), 0.0f, 1.0f, 0.05f );
				new CDebugSliderParam( workGroup, GGame->GetGameplayConfig().m_terrainInfluenceMul, TXT("Terrain Influence Multiplier"), 0.0f, 10.0f, 0.1f );
				new CDebugSliderParam( workGroup, GGame->GetGameplayConfig().m_slidingLimitMin, TXT("Sliding Limit Min"), 0.0f, 1.0f, 0.05f );
				new CDebugSliderParam( workGroup, GGame->GetGameplayConfig().m_slidingLimitMax, TXT("Sliding Limit Max"), 0.0f, 1.0f, 0.05f );
				new CDebugSliderParam( workGroup, GGame->GetGameplayConfig().m_slidingDamping, TXT("Sliding Damping"), 0.025f, 1.0f, 0.025f );
				new CDebugSliderParam( workGroup, GGame->GetGameplayConfig().m_maxPlatformDisplacement, TXT("Max platform displacement per step"), 0.0f, 5.0f, 0.2f );
				new CDebugSliderParam( workGroup, GGame->GetGameplayConfig().m_virtualRadiusTime, TXT("Virtual Radius Time"), 0.01f, 10.0, 0.01f );
			}

			// character controller jump/falling
			{
				IDebugCheckBox* workGroup = new IDebugCheckBox( NULL, TXT("Character Controller Jump/Falling"), true, false );
				m_tree->AddRoot( workGroup );

				new CDebugSliderParam( workGroup, GGame->GetGameplayConfig().m_jumpV0, TXT("Jump Velocity"), 0.0f, 30.0f, 0.2f );
				new CDebugSliderParam( workGroup, GGame->GetGameplayConfig().m_jumpTc, TXT("Jump Time Scale"), 0.0f, 5.0f, 0.05f );
				new CDebugSliderParam( workGroup, GGame->GetGameplayConfig().m_jumpDelay, TXT("Jump Delay"), 0.0f, 1.0f, 0.02f );
				new CDebugSliderParam( workGroup, GGame->GetGameplayConfig().m_jumpMinTime, TXT("Min Jump time"), 0.0f, 10.0f, 0.1f );
				new CDebugSliderParam( workGroup, GGame->GetGameplayConfig().m_jumpGravityUp, TXT("Jump Gravity Up"), -20.0f, 20.0f, 0.1f );
				new CDebugSliderParam( workGroup, GGame->GetGameplayConfig().m_jumpGravityDown, TXT("Jump Gravity Down"), -20.0f, 20.0f, 0.1f );
				new CDebugSliderParam( workGroup, GGame->GetGameplayConfig().m_jumpMaxVertSpeed, TXT("Max vertical speed while jumping"), -20.0f, 20.0f, 0.1f );
				new CDebugSliderParam( workGroup, GGame->GetGameplayConfig().m_jumpLenMul, TXT("Jump Len multiplier"), 0.0f, 10.0f, 0.2f );
				new CDebugSliderParam( workGroup, GGame->GetGameplayConfig().m_jumpHeightMul, TXT("Jump Height multiplier"), 0.0f, 10.0f, 0.2f );
				new CDebugSliderParam( workGroup, GGame->GetGameplayConfig().m_probeTerrainOffset, TXT("Probe terrain off"), 0.0f, 0.25f, 0.01f );
				new CDebugSliderParam( workGroup, GGame->GetGameplayConfig().m_fallingTime, TXT("Falling Delay time"), 0.0f, 1.0f, 0.05f );
				new CDebugSliderParam( workGroup, GGame->GetGameplayConfig().m_fallingMul, TXT("Falling multiplier"), 0.0f, 10.0f, 0.2f );
			}

			// character controller swimming/diving
			{
				IDebugCheckBox* workGroup = new IDebugCheckBox( NULL, TXT("Character Controller Swimming/Diving"), true, false );
				m_tree->AddRoot( workGroup );

				new CDebugSliderParam( workGroup, GGame->GetGameplayConfig().m_movingSwimmingOffset, TXT("Offset between moving and swimming"), -2.0f, 2.0f, 0.05f );
				new CDebugSliderParam( workGroup, GGame->GetGameplayConfig().m_emergeSpeed, TXT("Emerging Speed"), 0.1f, 100.0f, 0.1f );
				new CDebugSliderParam( workGroup, GGame->GetGameplayConfig().m_submergeSpeed, TXT("Submerging Speed"), 0.1f, 100.0f, 0.1f );
			}
		}

		Int32 x = 700;
		Int32 y = 100;
#ifndef RED_FINAL_BUILD
		Float time = 0;
		Float counter = 0;

		PHYSICS_STATISTICS_GET_AND_CLEAR(WindCount,counter)
		frame->AddDebugScreenText( x, y, String::Printf( TXT( " wind %i" ), ( Uint32 ) counter ), Color( 255, 255, 0 ) );
		y += 15;

		PHYSICS_STATISTICS_GET_AND_CLEAR(WaterCount,counter)
		frame->AddDebugScreenText( x, y, String::Printf( TXT( " water %i" ), ( Uint32 ) counter ), Color( 255, 255, 0 ) );
		y += 15;

		PHYSICS_STATISTICS_GET_AND_CLEAR(CameraCount,counter)
		frame->AddDebugScreenText( x, y, String::Printf( TXT( " camera %i" ), ( Uint32 ) counter ), Color( 255, 255, 0 ) );
		y += 15;

		PHYSICS_STATISTICS_GET_AND_CLEAR(CharacterCount,counter)
		frame->AddDebugScreenText( x, y, String::Printf( TXT( " character %i" ), ( Uint32 ) counter ), Color( 255, 255, 0 ) );
		y += 15;

		PHYSICS_STATISTICS_GET_AND_CLEAR(PhysicalSoundsCount,counter)
		frame->AddDebugScreenText( x, y, String::Printf( TXT( " physical sounds %i" ), ( Uint32 ) counter ), Color( 255, 255, 0 ) );
		y += 50;
		
#ifdef USE_PHYSX

#endif
#endif
		// Render tree		
		m_tree->OnRender( frame );		
	}

	virtual Bool OnViewportInput( IViewport* view, enum EInputKey key, enum EInputAction action, Float data )
	{
#ifndef NO_RED_GUI
#ifndef NO_DEBUG_WINDOWS
		if( action == IACT_Press && key == IK_Enter )
		{
			GDebugWin::GetInstance().SetVisible( true );
			GDebugWin::GetInstance().ShowDebugWindow( DebugWindows::DW_Physics );
		}
		return true;
#endif	// NO_DEBUG_WINDOWS
#endif	// NO_RED_GUI

		// Send the event
		if ( m_tree )
		{
			if ( m_tree->OnInput( key, action, data ) )
			{
				return true;
			}
		}

		// Not processed
		return false;
	}

	virtual void OnTick( Float timeDelta )
	{
		// Update crap
		if ( m_tree )
		{
			m_tree->OnTick( timeDelta );
		}
	}
};

void CreateDebugPagePhysics()
{
	IDebugPage* page = new CDebugPagePhysics();
	IDebugPageManagerBase::GetInstance()->RegisterDebugPage( page );
}

#endif
