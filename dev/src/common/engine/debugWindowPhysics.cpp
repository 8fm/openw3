/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#ifndef NO_RED_GUI
#ifndef NO_DEBUG_WINDOWS

#include "../physics/PhysicsRagdollWrapper.h"
#include "../physics/physicsEngine.h"
#include "../physics/physicsWorld.h"

#include "redGuiGroupBox.h"
#include "redGuiGridLayout.h"
#include "redGuiLabel.h"
#include "redGuiScrollPanel.h"
#include "redGuiPanel.h"
#include "redGuiAdvancedSlider.h"
#include "redGuiTab.h"
#include "redGuiCheckBox.h"
#include "redGuiButton.h"
#include "redGuiManager.h"
#include "debugWindowPhysics.h"
#include "../physics/physicsSettings.h"
#include "game.h"
#include "world.h"
#include "collisionCache.h"

#if defined(USE_PHYSX) && !defined(NO_EDITOR)
extern void UpdatePhysxSettings();
#endif

namespace DebugWindows
{
	CDebugWindowPhysics::CDebugWindowPhysics()
		: RedGui::CRedGuiWindow(200,200,800,600)
	{
		GRedGui::GetInstance().EventTick.Bind( this, &CDebugWindowPhysics::NotifyOnTick );
		SetCaption( TXT("Physics") );

		CreateControls();
	}

	CDebugWindowPhysics::~CDebugWindowPhysics()
	{
		GRedGui::GetInstance().EventTick.Unbind( this, &CDebugWindowPhysics::NotifyOnTick );
	}

	void CDebugWindowPhysics::CreateControls()
	{
		m_dumpComponentsWithPhysics = new RedGui::CRedGuiButton( 0, 0, 15, 15 );
		m_dumpComponentsWithPhysics->EventButtonClicked.Bind(this, &CDebugWindowPhysics::NotifyButtonClicked);
		m_dumpComponentsWithPhysics->SetText(TXT("dump physical components names"));
		m_dumpComponentsWithPhysics->SetMargin(Box2(1,0,1,0));
		m_dumpComponentsWithPhysics->SetPadding(Box2(2,2,2,2));
		m_dumpComponentsWithPhysics->SetBackgroundColor(Color(175, 175, 175));
		AddChild(m_dumpComponentsWithPhysics);
		m_dumpComponentsWithPhysics->SetDock(RedGui::DOCK_Top);

		m_dumpCollisionCache = new RedGui::CRedGuiButton( 0, 0, 15, 15 );
		m_dumpCollisionCache->EventButtonClicked.Bind(this, &CDebugWindowPhysics::NotifyButtonClickedStats);
		m_dumpCollisionCache->SetText(TXT("dump collision cache"));
		m_dumpCollisionCache->SetMargin(Box2(1,0,1,0));
		m_dumpCollisionCache->SetPadding(Box2(2,2,2,2));
		m_dumpCollisionCache->SetBackgroundColor(Color(175, 175, 175));
		AddChild(m_dumpCollisionCache);
		m_dumpCollisionCache->SetDock(RedGui::DOCK_Top);

		RedGui::CRedGuiGroupBox* group = new RedGui::CRedGuiGroupBox( 0, 0, 100, 200 );
		group->SetDock( RedGui::DOCK_Top );
		group->SetBackgroundColor( Color( 20, 20, 20, 255 ) );
		group->SetMargin( Box2( 3, 3, 3, 3 ) );
		group->SetText( TXT("Fast statistics") );
		AddChild( group );
		{
			RedGui::CRedGuiGridLayout* layout = new RedGui::CRedGuiGridLayout( 0, 0, 100, 100 );
			layout->SetDock( RedGui::DOCK_Fill );
			layout->SetDimensions( 3, 1 );
			group->AddChild( layout );
			{
				// create controls for left info panel
				RedGui::CRedGuiGridLayout* leftLayout = new RedGui::CRedGuiGridLayout( 0, 0, 100, 100 );
				leftLayout->SetDock( RedGui::DOCK_Fill );
				leftLayout->SetDimensions( 1, 5 );
				layout->AddChild( leftLayout );
				{
					m_wind = new RedGui::CRedGuiLabel( 0, 0, 100, 20 );
					leftLayout->AddChild( m_wind );
					m_water = new RedGui::CRedGuiLabel( 0, 0, 100, 20 );
					leftLayout->AddChild( m_water );
					m_camera = new RedGui::CRedGuiLabel( 0, 0, 100, 20 );
					leftLayout->AddChild( m_camera );
					m_character = new RedGui::CRedGuiLabel( 0, 0, 100, 20 );
					leftLayout->AddChild( m_character );
					m_physicalSounds = new RedGui::CRedGuiLabel( 0, 0, 100, 20 );
					leftLayout->AddChild( m_physicalSounds );
				}

				// create controls for middle info panel
				RedGui::CRedGuiGridLayout* middleLayout = new RedGui::CRedGuiGridLayout( 0, 0, 100, 100 );
				middleLayout->SetDock( RedGui::DOCK_Fill );
				middleLayout->SetDimensions( 1, 6 );
				layout->AddChild( middleLayout );
				{
					m_terrainTiles = new RedGui::CRedGuiLabel( 0, 0, 100, 20 );
					middleLayout->AddChild( m_terrainTiles );
					m_staticBodies = new RedGui::CRedGuiLabel( 0, 0, 100, 20 );
					middleLayout->AddChild( m_staticBodies );
					m_simpleBodies = new RedGui::CRedGuiLabel( 0, 0, 100, 20 );
					middleLayout->AddChild( m_simpleBodies );
					m_ragdolls = new RedGui::CRedGuiLabel( 0, 0, 100, 20 );
					middleLayout->AddChild( m_ragdolls );
					m_destructions = new RedGui::CRedGuiLabel( 0, 0, 100, 20 );
					middleLayout->AddChild( m_destructions );
					m_cloth = new RedGui::CRedGuiLabel( 0, 0, 100, 20 );
					middleLayout->AddChild( m_cloth );
				}

				// create controls for right info panel
				RedGui::CRedGuiGridLayout* rightLayout = new RedGui::CRedGuiGridLayout( 0, 0, 100, 100 );
				rightLayout->SetDock( RedGui::DOCK_Fill );
				rightLayout->SetDimensions( 1, 8 );
				layout->AddChild( rightLayout );
				{
					m_activeConstraints = new RedGui::CRedGuiLabel( 0, 0, 100, 20 );
					rightLayout->AddChild( m_activeConstraints );
					m_dynamicActiveBodies = new RedGui::CRedGuiLabel( 0, 0, 100, 20 );
					rightLayout->AddChild( m_dynamicActiveBodies );
					m_activeKinematicBodies = new RedGui::CRedGuiLabel( 0, 0, 100, 20 );
					rightLayout->AddChild( m_activeKinematicBodies );
					m_solverConstraints = new RedGui::CRedGuiLabel( 0, 0, 100, 20 );
					rightLayout->AddChild( m_solverConstraints );
					m_contactSize = new RedGui::CRedGuiLabel( 0, 0, 100, 20 );
					rightLayout->AddChild( m_contactSize );
					m_pairs = new RedGui::CRedGuiLabel( 0, 0, 100, 20 );
					rightLayout->AddChild( m_pairs );
					m_addBroadphase = new RedGui::CRedGuiLabel( 0, 0, 100, 20 );
					rightLayout->AddChild( m_addBroadphase );
					m_removeBroadphase = new RedGui::CRedGuiLabel( 0, 0, 100, 20 );
					rightLayout->AddChild( m_removeBroadphase );
				}
			}
		}

		// create tabs
		m_tabs = new RedGui::CRedGuiTab( 0, 0, 100, 100 );
		m_tabs->SetDock( RedGui::DOCK_Fill );
		m_tabs->SetMargin( Box2( 3, 3, 3, 3 ) );
		m_tabs->EventTabChanged.Bind( this, &CDebugWindowPhysics::NotifyOnTabChanged );
		AddChild( m_tabs );

		AddPhysicsTab();
		AddPhysicsSlidersTab();
		AddCharacterControllerTab();
		AddCharacterControllerJumpFallingTab();
		AddCharacterControllerSwimmingDivingTab();

		m_tabs->SetActiveTab( 0 );
	}

	void CDebugWindowPhysics::NotifyOnTick( RedGui::CRedGuiEventPackage& eventPackage, Float deltaTime )
	{
		if( GetVisible() == false )
		{
			return;
		}

		UpdateLeftInfoPanel();
		UpdateMiddleInfoPanel();
		UpdateRightInfoPanel();
	}

	void CDebugWindowPhysics::UpdateLeftInfoPanel()
	{
		Float time = 0;
		Float counter = 0;

		PHYSICS_STATISTICS_GET_AND_CLEAR( WindCount, counter );
		m_wind->SetText( String::Printf( TXT("Wind: %i pcs"), (Uint32)counter ) );

		PHYSICS_STATISTICS_GET_AND_CLEAR( WaterCount, counter );
		m_water->SetText( String::Printf( TXT("Water: %i pcs"), (Uint32)counter ) );

		PHYSICS_STATISTICS_GET_AND_CLEAR( CameraCount, counter );
		m_camera->SetText( String::Printf( TXT("Camera: %i pcs"), (Uint32)counter ) );

		PHYSICS_STATISTICS_GET_AND_CLEAR( CharacterCount, counter );
		m_character->SetText( String::Printf( TXT("Character: %i pcs"), (Uint32)counter ) );

		PHYSICS_STATISTICS_GET_AND_CLEAR( PhysicalSoundsCount, counter );
		m_physicalSounds->SetText( String::Printf( TXT("Physical sounds: %i pcs"), (Uint32)counter ) );
	}

	void CDebugWindowPhysics::UpdateMiddleInfoPanel()
	{
		Float simulated = 0;

		m_terrainTiles->SetText( String::Printf( TXT("Terrain tiles: %i pcs"), (Int32)SPhysicsStatistics::TerrainTilesInstanced ) );

		m_staticBodies->SetText( String::Printf( TXT("Static bodies: %i / %i pcs"), (Int32)SPhysicsStatistics::StaticBodiesSimulated, (Int32)SPhysicsStatistics::StaticBodiesInstanced ) );

		PHYSICS_STATISTICS_GET_AND_CLEAR( SimpleBodiesSimulated, simulated );
		m_simpleBodies->SetText( String::Printf( TXT("Simple bodies: %i / %i"), (Int32)simulated, (Int32)SPhysicsStatistics::SimpleBodiesInstanced ) );

		PHYSICS_STATISTICS_GET_AND_CLEAR( RagdollsSimulated, simulated );
		m_ragdolls->SetText( String::Printf( TXT("Ragdolls: %i / %i"), (Int32)simulated, (Int32)SPhysicsStatistics::RagdollsInstanced ) );

		PHYSICS_STATISTICS_GET_AND_CLEAR( DestructionsSimulated, simulated );
		m_destructions->SetText( String::Printf( TXT("Destruction: %i / %i"), (Int32)simulated, (Int32)SPhysicsStatistics::DestructionsInstanced ) );
	}

	void CDebugWindowPhysics::UpdateRightInfoPanel()
	{
	}

	void CDebugWindowPhysics::AddPhysicsTab()
	{
		String name = TXT("Physics");
		m_tabs->AddTab( name);

		RedGui::CRedGuiControl* tab = m_tabs->GetTabByName( name );
		if( tab != nullptr )
		{
			CreateCheckBox( tab, TXT("Pvd Transmit Contacts"), &SPhysicsSettings::m_pvdTransmitContacts );
			CreateCheckBox( tab, TXT("Pvd Transmit Scenequeries"), &SPhysicsSettings::m_pvdTransimtScenequeries );
			CreateCheckBox( tab, TXT("Pvd Transmit Constraints"), &SPhysicsSettings::m_pvdTransimtConstraints );
			CreateCheckBox( tab, TXT("Don't create trees"), &SPhysicsSettings::m_dontCreateTrees );
			CreateCheckBox( tab, TXT("Don't create ragdolls"), &SPhysicsSettings::m_dontCreateRagdolls );
			CreateCheckBox( tab, TXT("Don't create destruction"), &SPhysicsSettings::m_dontCreateDestruction );
			CreateCheckBox( tab, TXT("Don't create cloth"), &SPhysicsSettings::m_dontCreateCloth );
			CreateCheckBox( tab, TXT("Don't create cloth on GPU"), &SPhysicsSettings::m_dontCreateClothOnGPU );
			CreateCheckBox( tab, TXT("Don't create cloth secondary world"), &SPhysicsSettings::m_dontCreateClothSecondaryWorld );
			CreateCheckBox( tab, TXT("Don't create particles"), &SPhysicsSettings::m_dontCreateParticles );
			CreateCheckBox( tab, TXT("Don't create particles on GPU"), &SPhysicsSettings::m_dontCreateParticlesOnGPU );
			CreateCheckBox( tab, TXT("Don't create character controllers"), &SPhysicsSettings::m_dontCreateCharacterControllers );
			CreateCheckBox( tab, TXT("Don't create heightmap"), &SPhysicsSettings::m_dontCreateHeightmap );
			CreateCheckBox( tab, TXT("Don't create layer geometry"), &SPhysicsSettings::m_dontCreateLayerGeometry );
			CreateCheckBox( tab, TXT("Don't create static mesh geometry"), &SPhysicsSettings::m_dontCreateStaticMeshGeometry );
			CreateCheckBox( tab, TXT("Dynamics structure is dynamic"), &SPhysicsSettings::m_dynamicStructureIsDynamic );
			CreateCheckBox( tab, TXT("Statics structure is dynamic"), &SPhysicsSettings::m_staticsStructureIsDynamic );
			CreateSlider( tab, 4.0f, 10000.f, 10.0f, TXT("Dynamic rebuild hint"), TXT("Int"), &SPhysicsSettings::m_dynamicRebuildHint );
			CreateCheckBox( tab, TXT("Kinematics Contacts Enabled"), &SPhysicsSettings::m_kinematicsContactsEnabled );
			CreateCheckBox( tab, TXT("Use async terrain collision gen when in editor (faster terrain sculpting)"), &SPhysicsSettings::m_doAsyncTerrainCollisionInEditor );
		}
	}


	void CDebugWindowPhysics::AddPhysicsSlidersTab()
	{
		String name = TXT("Physics 2");
		m_tabs->AddTab( name);

		RedGui::CRedGuiControl* tab = m_tabs->GetTabByName( name );
		if( tab != nullptr )
		{
#ifdef USE_PHYSX
			
			CreateSlider( tab, 0.0f, 100.f, 1.0f, TXT("Simulation Delta Clamp"), TXT("Float"), &SPhysicsSettings::m_simulationDeltaClamp);
			CreateSlider( tab, 0.0f, 100.0f, 1.0f, TXT("Ragdoll Contact Multipler"), TXT("Float"), &SPhysicsSettings::m_ragdollContactMultipler);
			CreateSlider( tab, 0.0f, 100.0f, 1.0f, TXT("Ragdoll Contact Clamp"), TXT("Float"), &SPhysicsSettings::m_ragdollContactClamp);
			CreateSlider( tab, 0.0f, 100.0f, 1.0f, TXT("Rigidbody Contact Multipler"), TXT("Float"), &SPhysicsSettings::m_rigidbodyContactMultipler);
			CreateSlider( tab, 0.0f, 100.0f, 1.0f, TXT("Rigidbody Contact Clamp"), TXT("Float"), &SPhysicsSettings::m_rigidbodyContactClamp);

			CreateSlider( tab, 0.0f, 10.f, 0.05f, TXT("Rigidbody Linear Velocity Clamp"), TXT("Float"), &SPhysicsSettings::m_rigidbodyLinearVelocityClamp);
			CreateSlider( tab, 0.0f, 10.f, 0.05f, TXT("Actor angular velocity limit"), TXT("Float"), &SPhysicsSettings::m_actorAngularVelocityLimit);
			
			CreateSlider( tab, 0.0f, 100.0f, 1.0f, TXT("Particle Simulation Distance Limit"), TXT("Float"), &SPhysicsSettings::m_particleSimulationDistanceLimit );
			CreateSlider( tab, 0.0f, 10.0f, 0.1f, TXT("Particle Cell Size"), TXT("Float"), &SPhysicsSettings::m_particleCellSize );

			CreateSlider( tab, 0.0f, 1000.f, 5.0f, TXT("Static simulation distance limit"), TXT("Float"), &SPhysicsSettings::m_staticBodiesDistanceLimit );
			CreateSlider( tab, 0.0f, 10000.f, 10.0f, TXT("Tiles simulation distance limit"), TXT("Float"), &SPhysicsSettings::m_tilesDistanceLimit );
			CreateSlider( tab, 0.0f, 1000.f, 5.0f, TXT("Destruction simulation distance limit"), TXT("Float"), &SPhysicsSettings::m_destructionSimulationDistanceLimit );
			CreateSlider( tab, 0.0f, 1000.f, 5.0f, TXT("Ragdoll simulation dynamic distance limit"), TXT("Float"), &SPhysicsSettings::m_ragdollSimulationDynamicDistanceLimit );
			CreateSlider( tab, 0.0f, 1000.f, 5.0f, TXT("Ragdoll simulation kinematic distance limit"), TXT("Float"), &SPhysicsSettings::m_ragdollSimulationKinematicDistanceLimit );
			CreateSlider( tab, 0.0f, 1000.f, 5.0f, TXT("Dynamics simulation distance limit"), TXT("Float"), &SPhysicsSettings::m_simpleBodySimulationDistanceLimit );
			CreateSlider( tab, 0.0f, 50.f,	 1.0f, TXT("Cloth simulation distance limit"), TXT("Float"), &SPhysicsSettings::m_clothSimulationDistanceLimit );

			CreateCheckBox( tab, TXT("Joints projection"), &SPhysicsSettings::m_ragdollJointsProjection );
			CreateSlider( tab, 0.0f, 100.0f, 0.01f, TXT("Projection Linear Tolerance"), TXT("Float"), &SPhysicsSettings::m_ragdollProjectionLinearTolerance );
			CreateSlider( tab, 0.0f, 3.1415926535897932384626433f, 0.05f, TXT("Projection Angular Tolerance"), TXT("Float"), &SPhysicsSettings::m_ragdollProjectionAngularTolerance );
			
			CreateSlider( tab, 0.0f, 255.0f, 1.0f, TXT("Ragdoll Min Position Iters"), TXT("Int"), &SPhysicsSettings::m_rigidbodyPositionIters);
			CreateSlider( tab, 0.0f, 255.0f, 1.0f, TXT("Ragdoll Min Velocity Iters"), TXT("Int"), &SPhysicsSettings::m_rigidbodyVelocityIters);

			CreateSlider( tab, 0.0f, 255.0f, 1.0f, TXT("Ragdoll Min Position Iters"), TXT("Int"), &SPhysicsSettings::m_ragdollMinPositionIters);
			CreateSlider( tab, 0.0f, 255.0f, 1.0f, TXT("Ragdoll Min Velocity Iters"), TXT("Int"), &SPhysicsSettings::m_ragdollMinVelocityIters);

			CreateSlider( tab, 0.0f, 10.f, 0.05f, TXT("Ragdoll In Vieport Sleep Threshold"), TXT("Float"), &SPhysicsSettings::m_ragdollInVieportSleepThreshold);

			CreateSlider( tab, 0.0f, 255.0f, 1.0f, TXT("Ragdoll In Viewport Min Position Iters"), TXT("Int"), &SPhysicsSettings::m_ragdollInVieportMinPositionIters);
			CreateSlider( tab, 0.0f, 255.0f, 1.0f, TXT("Ragdoll In Viewport Min Velocity Iters"), TXT("Int"), &SPhysicsSettings::m_ragdollInVieportMinVelocityIters);

			CreateSlider( tab, 0.0f, 1.0f, 0.01f, TXT("Actor contact offset"), TXT("Float"), &SPhysicsSettings::m_contactOffset );
			CreateSlider( tab, 0.0f, 1.0f, 0.01f, TXT("Actor rest offset"), TXT("Float"), &SPhysicsSettings::m_restOffset );

			CreateSlider( tab, 0.0f, 100.0f, 0.5f, TXT("Ragdoll Wind Scaler"), TXT("Float"), &SPhysicsSettings::m_ragdollGlobalWindScaler );
			CreateSlider( tab, 0.0f, 100.0f, 0.5f, TXT("Cloth Wind Scaler"), TXT("Float"), &SPhysicsSettings::m_clothWindScaler );

			CreateSlider( tab, 0.0f, 1.0f, 0.01f, TXT("Cloth collider sphere minimal radius"), TXT("Float"), &SPhysicsSettings::m_clothColiderSphereMinimalRadius );
			CreateSlider( tab, 0.0f, 1.0f, 0.01f, TXT("Cloth collider sphere minimal radius"), TXT("Float"), &SPhysicsSettings::m_clothColiderCapsuleMinimalRadius );

			CreateSlider( tab, 0.0f, 1.0f, 0.05f, TXT("Ratio of sweep sphere radius (for fading occludables)."), TXT("Float"), &SPhysicsSettings::m_cameraOccludablesRadiusRatio );
			CreateSlider( tab, 0.0f, 1.0f, 0.05f, TXT("Ratio of sweep start offset (for fading occludables)."), TXT("Float"), &SPhysicsSettings::m_cameraOccludablesStartRatio);

			CreateSlider( tab, 0.0f, 10.0f, 0.05f, TXT("Simple wrapper linear damping"), TXT("Float"), &SPhysicsSettings::m_simpleBodyLinearDamper );
			CreateSlider( tab, 0.0f, 10.0f, 0.05f, TXT("Simple wrapper angular damping"), TXT("Float"), &SPhysicsSettings::m_simpleBodyAngularDamper );

			CreateSlider( tab, 0.0f, 100.0f, 0.1f, TXT("Ragdoll jointed linear damping"), TXT("Float"), &SPhysicsSettings::m_ragdollJointedLinearDamper );
			CreateSlider( tab, 0.0f, 100.0f, 0.1f, TXT("Ragdoll jointed angular damping"), TXT("Float"), &SPhysicsSettings::m_ragdollJointedAngularDamper );

			CreateSlider( tab, 0.0f, 100.0f, 0.1f, TXT("Ragdoll chained linear damping"), TXT("Float"), &SPhysicsSettings::m_ragdollChainedLinearDamper );
			CreateSlider( tab, 0.0f, 100.0f, 0.1f, TXT("Ragdoll chained angular damping"), TXT("Float"), &SPhysicsSettings::m_ragdollChainedAngularDamper );

			CreateSlider( tab, 0.0f, 100.0f, 0.1f, TXT("Ragdoll jointed sleep threshold"), TXT("Float"), &SPhysicsSettings::m_ragdollJointedSleepThreshold );
			CreateSlider( tab, 0.0f, 100.0f, 0.1f, TXT("Ragdoll chained sleep threshold"), TXT("Float"), &SPhysicsSettings::m_ragdollChainedSleepThreshold );

			CreateSlider( tab, 0.0f, 100.0f, 0.5f, TXT("Ragdoll max linear damping"), TXT("Float"), &SPhysicsSettings::m_ragdollMaxLinearDamper );
			CreateSlider( tab, 0.0f, 100.0f, 0.5f, TXT("Ragdoll max angular damping"), TXT("Float"), &SPhysicsSettings::m_ragdollMaxAngularDamper );
			CreateSlider( tab, 0.0f, 120.0f, 1.0f, TXT("Ragdoll max simulation time"), TXT("Float"), &SPhysicsSettings::m_ragdollMaxSimulationTime );
			CreateSlider( tab, 0.0f, 1.0f, 0.01f, TXT("Ragdoll speed under which will fall to sleep"), TXT("Float"), &SPhysicsSettings::m_ragdollSpeedForSleep );
			CreateSlider( tab, 0.0f, 10.0f, 0.25f, TXT("Ragdoll speed of falling to sleep"), TXT("Float"), &SPhysicsSettings::m_ragdollSleepFallSpeed );

			CreateSlider( tab, 0.0f, 10.0f, 0.05f, TXT("Destruction wrapper linear damping"), TXT("Float"), &SPhysicsSettings::m_destructionLinearDamper );
			CreateSlider( tab, 0.0f, 10.0f, 0.25f, TXT("Destruction wrapper angular damping"), TXT("Float"), &SPhysicsSettings::m_destructionAngularDamper );

			CreateSlider( tab, 0.0f, 1.0f, 0.01f, TXT("Fluid buoyancy minimal depth"), TXT("Float"), &SPhysicsSettings::m_fluidBuoyancyMinimalDepth );
			
			CreateSlider( tab, -10.0f, 10.0f, 0.05f, TXT("Fluid linear damping offset"), TXT("Float"), &SPhysicsSettings::m_fluidLinearDamping );
			CreateSlider( tab, -10.0f, 10.0f, 0.05f, TXT("Fluid linear force multipler"), TXT("Float"), &SPhysicsSettings::m_fluidLinearForceMultipler );

			CreateSlider( tab, -10.0f, 10.0f, 0.05f, TXT("Fluid angular damping offset"), TXT("Float"), &SPhysicsSettings::m_fluidAngularDamping );
			CreateSlider( tab, 0.0f, 100.0f, 1.0f, TXT("Fluid angular force multipler"), TXT("Float"), &SPhysicsSettings::m_fluidAngularForceMultipler );
			CreateSlider( tab, 0.0f, 100.0f, 0.1f, TXT("Fluid angular force max clamp"), TXT("Float"), &SPhysicsSettings::m_fluidAngularForceMaxClamp );
			CreateSlider( tab, 0.0f, 2.0f, 0.05f, TXT("Fluid angular predefined radius"), TXT("Float"), &SPhysicsSettings::m_fluidAngularPredefinedRadius );

			CreateSlider( tab, 0.0f, 10.f, 0.05f, TXT("Actor sleep threshold"), TXT("Float"), &SPhysicsSettings::m_actorSleepThreshold);
			CreateSlider( tab, 0.0f, 10.f, 0.05f, TXT("Destructible unfractured sleep threshold"), TXT("Float"), &SPhysicsSettings::m_destructibleUnfracturedSleepThreshold);
			CreateSlider( tab, 0.0f, 10.f, 1.0f, TXT("(NO WORLDS) Default Dispacher CPU Count"), TXT("Int"), &SPhysicsSettings::m_useCpuDefaultDispacherNbCores );
#endif	// USE_PHYSX
		}
	}


	void CDebugWindowPhysics::AddCharacterControllerTab()
	{
		String name = TXT("Character controller");
		m_tabs->AddTab( name);

		RedGui::CRedGuiControl* tab = m_tabs->GetTabByName( name );
		if( tab != nullptr )
		{
			CreateCheckBox( tab, TXT("Only Player Can Push"), &SPhysicsSettings::m_onlyPlayerCanPush );
			CreateCheckBox( tab, TXT("Nonplayers CanPush Non Sleepers"), &SPhysicsSettings::m_nonplayersCanPushOnlyNonsleepers );
			CreateCheckBox( tab, TXT("Character Force Position Cache Invalidation"), &SPhysicsSettings::m_characterForcePositionCacheInvalidation );
			CreateCheckBox( tab, TXT("Character Manual Cache Invalidation"), &SPhysicsSettings::m_characterManualCacheInvalidation );

			CreateSlider( tab, 0.0f, 100, 1.0f, TXT("Character Pushing Multipler"), TXT("Float"), &SPhysicsSettings::m_characterPushingMultipler);

			CreateSlider( tab, 0.0f, 100, 0.01f, TXT("Character Step Big Kinematics Clamp"), TXT("Float"), &SPhysicsSettings::m_characterStepBigKinematicClamp);
			CreateSlider( tab, 0.0f, 100, 0.01f, TXT("Character Step Big Dynamics Clamp"), TXT("Float"), &SPhysicsSettings::m_characterStepBigDynamicsClamp);
			CreateSlider( tab, 0.0f, 100.0, 1.0f, TXT("Character Pushing Max Clamp"), TXT("Float"), &SPhysicsSettings::m_characterPushingMaxClamp );
			CreateSlider( tab, 0.0f, 2.0, 0.01f, TXT("Character Footstep Water Level Limit"), TXT("Float"), &SPhysicsSettings::m_characterFootstepWaterLevelLimit );
			CreateSlider( tab, 0.0f, 1.0f, 0.05f, TXT("Terrain Influence Min Slope Limit"), TXT("Float"), &GGame->GetGameplayConfig().m_terrainInfluenceLimitMin );
			CreateSlider( tab, 0.0f, 1.0f, 0.05f, TXT("Terrain Influence Max Slope Limit"), TXT("Float"), &GGame->GetGameplayConfig().m_terrainInfluenceLimitMax );
			CreateSlider( tab, 0.0f, 10.0f, 0.1f, TXT("Terrain Influence Multiplier"), TXT("Float"), &GGame->GetGameplayConfig().m_terrainInfluenceMul );
			CreateSlider( tab, 0.0f, 1.0f, 0.05f, TXT("Sliding Limit Min"), TXT("Float"), &GGame->GetGameplayConfig().m_slidingLimitMin );
			CreateSlider( tab, 0.0f, 1.0f, 0.05f, TXT("Sliding Limit Max"), TXT("Float"), &GGame->GetGameplayConfig().m_slidingLimitMax );
			CreateSlider( tab, 0.025f, 1.0f, 0.025f, TXT("Sliding Damping"), TXT("Float"), &GGame->GetGameplayConfig().m_slidingDamping );
			CreateSlider( tab, 0.0f, 5.0f, 0.2f, TXT("Max platform displacement per step"), TXT("Float"), &GGame->GetGameplayConfig().m_maxPlatformDisplacement );
			CreateSlider( tab, 0.01f, 10.0, 0.01f, TXT("Virtual Radius Time"), TXT("Float"), &GGame->GetGameplayConfig().m_virtualRadiusTime );
		}
	}

	void CDebugWindowPhysics::AddCharacterControllerJumpFallingTab()
	{
		String name = TXT("Jump / Falling");
		m_tabs->AddTab( name);

		RedGui::CRedGuiControl* tab = m_tabs->GetTabByName( name );
		if( tab != nullptr )
		{
			CreateSlider( tab, 0.0f, 30.0f, 0.2f, TXT("Jump Velocity"), TXT("Float"), &GGame->GetGameplayConfig().m_jumpV0 );
			CreateSlider( tab, 0.0f, 5.0f, 0.05f, TXT("Jump Time Scale"), TXT("Float"), &GGame->GetGameplayConfig().m_jumpTc );
			CreateSlider( tab, 0.0f, 1.0f, 0.02f, TXT("Jump Delay"), TXT("Float"), &GGame->GetGameplayConfig().m_jumpDelay );
			CreateSlider( tab, 0.0f, 10.0f, 0.1f, TXT("Min Jump time"), TXT("Float"), &GGame->GetGameplayConfig().m_jumpMinTime );
			CreateSlider( tab, -20.0f, 20.0f, 0.1f, TXT("Jump Gravity Up"), TXT("Float"), &GGame->GetGameplayConfig().m_jumpGravityUp );
			CreateSlider( tab, -20.0f, 20.0f, 0.1f, TXT("Jump Gravity Down"), TXT("Float"), &GGame->GetGameplayConfig().m_jumpGravityDown );
			CreateSlider( tab, -20.0f, 20.0f, 0.1f, TXT("Max vertical speed while jumping"), TXT("Float"), &GGame->GetGameplayConfig().m_jumpMaxVertSpeed );
			CreateSlider( tab, 0.0f, 10.0f, 0.2f, TXT("Jump Len multiplier"), TXT("Float"), &GGame->GetGameplayConfig().m_jumpLenMul );
			CreateSlider( tab, 0.0f, 10.0f, 0.2f, TXT("Jump Height multiplier"), TXT("Float"), &GGame->GetGameplayConfig().m_jumpHeightMul );
			CreateSlider( tab, 0.0f, 0.25f, 0.01f, TXT("Probe terrain off"), TXT("Float"), &GGame->GetGameplayConfig().m_probeTerrainOffset );
			CreateSlider( tab, 0.0f, 1.0f, 0.05f, TXT("Falling Delay time"), TXT("Float"), &GGame->GetGameplayConfig().m_fallingTime );
			CreateSlider( tab, 0.0f, 10.0f, 0.2f, TXT("Falling multiplier"), TXT("Float"), &GGame->GetGameplayConfig().m_fallingMul );
		}
	}

	void CDebugWindowPhysics::AddCharacterControllerSwimmingDivingTab()
	{
		String name = TXT("Swimming / Diving");
		m_tabs->AddTab( name);

		RedGui::CRedGuiControl* tab = m_tabs->GetTabByName( name );
		if( tab != nullptr )
		{
			CreateSlider( tab, -2.0f, 2.0f, 0.05f, TXT("Offset between moving and swimming"), TXT("Float"), &GGame->GetGameplayConfig().m_movingSwimmingOffset );
			CreateSlider( tab, 0.1f, 100.0f, 0.1f, TXT("Emerging Speed"), TXT("Float"), &GGame->GetGameplayConfig().m_emergeSpeed );
			CreateSlider( tab, 0.1f, 100.0f, 0.1f, TXT("Submerging Speed"), TXT("Float"), &GGame->GetGameplayConfig().m_submergeSpeed );
		}
	}

	void CDebugWindowPhysics::CreateSlider( RedGui::CRedGuiControl* parent, Float min, Float max, Float step, const String& name, const String& userDataType, RedGui::RedGuiAny userData )
	{
		RedGui::CRedGuiPanel* panel = new RedGui::CRedGuiPanel( 0, 0, 100, 40 );
		panel->SetDock( RedGui::DOCK_Top );
		panel->SetMargin( Box2( 5, 5, 5, 5 ) );
		panel->SetBorderVisible( false );
		parent->AddChild( panel );
		{
			RedGui::CRedGuiAdvancedSlider* slider = new RedGui::CRedGuiAdvancedSlider( 0, 0, 300, 20 );
			slider->SetDock( RedGui::DOCK_Left );
			slider->SetMinValue( min );
			slider->SetMaxValue( max );
			slider->SetStepValue( step );
			slider->SetUserString( TXT("Type"), userDataType );
			slider->SetUserData( userData );
			slider->EventScroll.Bind( this, &CDebugWindowPhysics::NotifyOnSliderChanged );
			panel->AddChild( slider );
			m_sliders.PushBack( slider );

			RedGui::CRedGuiLabel* label = new RedGui::CRedGuiLabel( 0, 0, 200, 20 );
			label->SetText( name );
			label->SetDock( RedGui::DOCK_Left );
			label->SetMargin( Box2( 30, 5, 0, 0 ) );
			panel->AddChild( label );
		}
	}

	void CDebugWindowPhysics::CreateCheckBox( RedGui::CRedGuiControl* parent, const String& name, RedGui::RedGuiAny userData )
	{
		RedGui::CRedGuiCheckBox* checkBox = new RedGui::CRedGuiCheckBox( 0, 0, 100, 25 );
		checkBox->SetMargin( Box2( 5, 5, 5, 5 ) );
		checkBox->SetDock( RedGui::DOCK_Top );
		checkBox->SetText( name );
		checkBox->SetUserString( TXT("Type"), TXT("Bool") );
		checkBox->SetUserData( userData );
		checkBox->EventCheckedChanged.Bind( this, &CDebugWindowPhysics::NotifyOnCheckedChanged );
		parent->AddChild( checkBox );
		m_checkboxes.PushBack( checkBox );
	}

	void CDebugWindowPhysics::NotifyOnTabChanged( RedGui::CRedGuiEventPackage& eventPackage, RedGui::CRedGuiControl* selectedTab )
	{
		RED_UNUSED( eventPackage );

		const Uint32 sliderCount = m_sliders.Size();
		for( Uint32 i=0; i<sliderCount; ++i )
		{
			String type = m_sliders[i]->GetUserString( TXT("Type") );

			if( type == TXT("Float") )
			{
				Float* userValue = m_sliders[i]->GetUserData< Float >();
				RedGui::CRedGuiAdvancedSlider* slider = static_cast< RedGui::CRedGuiAdvancedSlider* >( m_sliders[i] );
				slider->SetValue( *userValue );
			}
			else if( type == TXT("Int") )
			{
				Int32* userValue = m_sliders[i]->GetUserData< Int32 >();
				RedGui::CRedGuiAdvancedSlider* slider = static_cast< RedGui::CRedGuiAdvancedSlider* >( m_sliders[i] );
				slider->SetValue( (Float)( *userValue ) );
			}
		}

		const Uint32 checkBoxCount = m_checkboxes.Size();
		for( Uint32 i=0; i<checkBoxCount; ++i )
		{
			Bool* userValue = m_checkboxes[i]->GetUserData< Bool >();
			RedGui::CRedGuiCheckBox* checkBox = static_cast< RedGui::CRedGuiCheckBox* >( m_checkboxes[i] );
			checkBox->SetChecked( ( *userValue ) );
		}
	}

	void CDebugWindowPhysics::NotifyOnSliderChanged( RedGui::CRedGuiEventPackage& eventPackage, Float value )
	{
		RedGui::CRedGuiControl* sender = eventPackage.GetEventSender();

		String type = sender->GetUserString( TXT("Type") );

		if( type == TXT("Float") )
		{
			Float* userValue = sender->GetUserData< Float >();
			( *userValue ) = value;
		}
		else if( type == TXT("Int") )
		{
			Int32* userValue = sender->GetUserData< Int32 >();
			( *userValue ) = (Int32)value;
		}

#if defined(USE_PHYSX) && !defined(NO_EDITOR)
		UpdatePhysxSettings();
#endif
	}

	void CDebugWindowPhysics::OnWindowOpened( CRedGuiControl* control )
	{
		RedGui::CRedGuiEventPackage eventPackage( this );
		NotifyOnTabChanged( eventPackage, m_tabs->GetActiveTab() );
	}

	void CDebugWindowPhysics::NotifyOnCheckedChanged( RedGui::CRedGuiEventPackage& eventPackage, Bool value )
	{
		CRedGuiControl* sender = eventPackage.GetEventSender();

		Bool* userValue = sender->GetUserData< Bool >();
		( *userValue ) = value;

#if defined(USE_PHYSX) && !defined(NO_EDITOR)
		UpdatePhysxSettings();
#endif
	}

	void CDebugWindowPhysics::NotifyButtonClicked( RedGui::CRedGuiEventPackage& eventPackage )
	{
//		CPhysicsWorldPhysXImpl::DumpPhysicalComponentsNamesGlobaly();
	}

	void CDebugWindowPhysics::NotifyButtonClickedStats( RedGui::CRedGuiEventPackage& eventPackage )
	{
		C2dArray* dump = GCollisionCache->DumpStatistics( false );
		int a = 0;
	}

}	// namespace DebugWindows

#endif	//NO_DEBUG_WINDOWS
#endif	//NO_RED_GUI
