/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#ifndef NO_RED_GUI
#ifndef NO_DEBUG_WINDOWS
#ifdef USE_UMBRA

#include "umbraScene.h"

#include "redGuiButton.h"
#include "redGuiLabel.h"
#include "redGuiManager.h"
#include "redGuiCheckBox.h"
#include "redGuiGroupBox.h"
#include "redGuiPanel.h"
#include "redGuiProgressBar.h"
#include "redGuiSaveFileDialog.h"
#include "redGuiSlider.h"
#include "debugWindowUmbra.h"

#include "renderCommands.h"
#include "../renderer/renderElementMap.h"
#include "renderFrame.h"
#include "renderProxy.h"
#include "world.h"
#include "worldIterators.h"
#include "baseEngine.h"
#include "inputKeys.h"
#include "rawInputManager.h"
#include "meshComponent.h"
#include "entity.h"
#include "layer.h"
#include "layerInfo.h"
#include "mesh.h"

namespace DebugWindows
{
	EMemoryClass CDebugWindowUmbra::MemoryClassesToTrack[] = 
	{ 
		MC_UmbraGeneric,
		MC_UmbraTomeCollection,
		MC_UmbraTome,
		MC_UmbraBuffer,
		MC_UmbraObjectCache,
		MC_UmbraVisibleChunks,
		MC_UmbraGates,
		MC_UmbraQueryAdditionalMemory,
		MC_UmbraQuery,
		MC_UmbraObjectIDMap,
	};

	/// Rendering panel movement keys
	enum EUmbraCameraMovementKeys
	{
		UCMK_Forward,
		UCMK_Back,
		UCMK_StrafeLeft,
		UCMK_StrafeRight,
		UCMK_Up,
		UCMK_Down,
		UCMK_Sprint,
	};

	CDebugWindowUmbra::CDebugWindowUmbra() 
		: RedGui::CRedGuiWindow( 100, 100, 400, 420 )
		, m_resetOcclusionCamera( false )
		, m_attachOcclusionCameraToRenderCamera( true )
		, m_maxMemOverall( 0.f )
		, m_cameraSpeedMultiplier( 1.f )
		, m_distanceMultiplier( CUmbraScene::DEFAULT_UMBRA_DISTANCE_MULTIPLIER )
	{
		SetCaption( TXT("Umbra") );

		GRedGui::GetInstance().EventTick.Bind( this, &CDebugWindowUmbra::NotifyOnTick );
		GRedGui::GetInstance().EventViewportInput.Bind( this, &CDebugWindowUmbra::NotifyViewportInput );
		GRedGui::GetInstance().EventViewportClick.Bind( this, &CDebugWindowUmbra::NotifyViewportClick );
		GRedGui::GetInstance().EventViewportGenerateFragments.Bind( this, &CDebugWindowUmbra::NotifyViewportGenerateFragments );

		m_attachOcclusionCamera = new RedGui::CRedGuiCheckBox( 0, 0, 100, 25 );
		m_attachOcclusionCamera->SetText( TXT("Attach occlusion camera to render camera") );
		m_attachOcclusionCamera->SetChecked( true );
		m_attachOcclusionCamera->SetMargin( Box2( 10, 10, 5, 5 ) );
		m_attachOcclusionCamera->SetDock( RedGui::DOCK_Top );
		m_attachOcclusionCamera->EventCheckedChanged.Bind( this, &CDebugWindowUmbra::NotifyChangeAttachCamera );
		AddChild( m_attachOcclusionCamera );

		m_saveFileDialog = new RedGui::CRedGuiSaveFileDialog();
		m_saveFileDialog->AddFilter(TXT("Text file"), TXT("txt"));
		m_saveFileDialog->EventFileOK.Bind( this, &CDebugWindowUmbra::NotifyEventFileOK );

		m_dumpMeshesBtn = new RedGui::CRedGuiButton( 0, 0, 100, 25 );
		m_dumpMeshesBtn->SetText( TXT("DumpMeshes") );
		m_dumpMeshesBtn->SetMargin( Box2( 10, 10, 5, 5 ) );
		m_dumpMeshesBtn->SetDock( RedGui::DOCK_Top );
		m_dumpMeshesBtn->EventButtonClicked.Bind( this, &CDebugWindowUmbra::DumpVisibleMeshes );
		AddChild( m_dumpMeshesBtn );

		m_timeStatsGroupBox = new RedGui::CRedGuiGroupBox( 0, 0, 100, 100 );
		m_timeStatsGroupBox->SetMargin( Box2( 5, 5, 5, 5 ) );
		m_timeStatsGroupBox->SetDock( RedGui::DOCK_Top );
		m_timeStatsGroupBox->SetText( TXT("Umbra statistics") );
		AddChild( m_timeStatsGroupBox );

		m_reMapStatsGroupBox = new RedGui::CRedGuiGroupBox( 0, 0, 100, 500 );
		m_reMapStatsGroupBox->SetMargin( Box2( 5, 5, 5, 5 ) );
		m_reMapStatsGroupBox->SetDock( RedGui::DOCK_Top );
		m_reMapStatsGroupBox->SetText( TXT("RenderElementMap statistics") );
		AddChild( m_reMapStatsGroupBox );

		m_shadowStatsGroupBox = new RedGui::CRedGuiGroupBox( 0, 0, 100, 200 );
		m_shadowStatsGroupBox->SetMargin( Box2( 5, 5, 5, 5 ) );
		m_shadowStatsGroupBox->SetDock( RedGui::DOCK_Top );
		m_shadowStatsGroupBox->SetText( TXT("Shadow statistics") );
		AddChild( m_shadowStatsGroupBox );

		Uint32 memStatsHeight = ARRAY_COUNT( MemoryClassesToTrack ) * 40;
		m_memoryStatsGroupBox = new RedGui::CRedGuiGroupBox( 0, 0, 100, memStatsHeight );
		m_memoryStatsGroupBox->SetMargin( Box2( 5, 5, 5, 5 ) );
		m_memoryStatsGroupBox->SetDock( RedGui::DOCK_Top );
		m_memoryStatsGroupBox->SetText( TXT("Memory usage by Umbra") );
		AddChild( m_memoryStatsGroupBox );

		m_queryThresholdLabel = new CRedGuiUmbraStatLabel( 0, 0, 100, 20, TXT("Query threshold: ") );
		m_queryThresholdLabel->SetMargin( Box2( 5, 5, 0, 0 ) );
		m_queryThresholdLabel->SetDock( RedGui::DOCK_Top );
		AddChild( m_queryThresholdLabel );

		m_queryThresholdSlider = new RedGui::CRedGuiSlider( 0, 0, 100, 20 );
		m_queryThresholdSlider->SetMargin( Box2( 5, 5, 5, 5 ) );
		m_queryThresholdSlider->SetDock( RedGui::DOCK_Top );
		m_queryThresholdSlider->SetMinValue( -1.0f );
		m_queryThresholdSlider->SetMaxValue( 100.0f );
		m_queryThresholdSlider->SetStepValue( 1.0f );
		m_queryThresholdSlider->SetValue( -1.0f );
		m_queryThresholdSlider->EventScroll.Bind( this, &CDebugWindowUmbra::QueryThresholdChanged );
		AddChild( m_queryThresholdSlider );

		m_queryThresholdLabel->Update( String::Printf( TXT("%1.2f"), m_queryThresholdSlider->GetValue() ) );

		// create tooltip for distance multiplier
		RedGui::CRedGuiPanel* sliderToolTip = new RedGui::CRedGuiPanel(0, 0, 200, 50);
		sliderToolTip->SetBackgroundColor(Color(0, 0, 0));
		sliderToolTip->SetVisible(false);
		sliderToolTip->AttachToLayer(TXT("Pointers"));
		sliderToolTip->SetPadding(Box2(5, 5, 5, 5));
		sliderToolTip->SetAutoSize(true);

		RedGui::CRedGuiLabel* info = new RedGui::CRedGuiLabel(10, 10, 10, 15);
		info->SetText(TXT("In order to save the property value, please explicitly CHECK OUT AND SAVE the world - this is for testing purposes only."), Color::WHITE);
		(*sliderToolTip).AddChild( info );

		m_distanceMultiplierLabel = new CRedGuiUmbraStatLabel( 0, 0, 100, 20, TXT("Distance multiplier: ") );
		m_distanceMultiplierLabel->SetNeedToolTip(true);
		m_distanceMultiplierLabel->SetToolTip( sliderToolTip );
		m_distanceMultiplierLabel->SetMargin( Box2( 5, 5, 0, 0 ) );
		m_distanceMultiplierLabel->SetDock( RedGui::DOCK_Top );
		AddChild( m_distanceMultiplierLabel );

		m_distanceMultiplierSlider = new RedGui::CRedGuiSlider( 0, 0, 100, 20 );
		m_distanceMultiplierSlider->SetMargin( Box2( 5, 5, 5, 5 ) );
		m_distanceMultiplierSlider->SetDock( RedGui::DOCK_Top );
		m_distanceMultiplierSlider->SetMinValue( 0.5f );
		m_distanceMultiplierSlider->SetMaxValue( 5.0f );
		m_distanceMultiplierSlider->SetStepValue( 0.5f );
		
		if ( GGame && GGame->GetActiveWorld() && GGame->GetActiveWorld()->GetUmbraScene() )
		{
			m_distanceMultiplier = GGame->GetActiveWorld()->GetUmbraScene()->GetDistanceMultiplier();
		}
		m_distanceMultiplierSlider->SetValue( m_distanceMultiplier );
		m_distanceMultiplierSlider->EventScroll.Bind( this, &CDebugWindowUmbra::DistanceMultiplierChanged );
		AddChild( m_distanceMultiplierSlider );

		m_distanceMultiplierLabel->Update( String::Printf( TXT("%1.2f"), m_distanceMultiplierSlider->GetValue() ) );

		CreateStatsControls();
		CreateProgressControls();
	}

	CDebugWindowUmbra::~CDebugWindowUmbra()
	{
		for ( Uint32 i = 0; i < m_memInfos.Size(); ++i )
		{
			delete m_memInfos[i];
			m_memInfos[i] = nullptr;
		}
		/*intentionally empty*/
	}

	void CDebugWindowUmbra::NotifyOnTick( RedGui::CRedGuiEventPackage& eventPackage, Float timeDelta )
	{
		RED_UNUSED( eventPackage );

		if( !GetVisible() )
		{
			return;
		}

		FillStatsControls();
		UpdateGenerateUmbraProgress();

		if ( GGame && GGame->GetActiveWorld() )
		{
			if ( CUmbraScene* umbraScene = GGame->GetActiveWorld()->GetUmbraScene() )
			{
				m_distanceMultiplier = umbraScene->GetDistanceMultiplier();
				m_distanceMultiplierSlider->SetValue( m_distanceMultiplier );
				m_distanceMultiplierLabel->Update( String::Printf( TXT("%1.2f"), m_distanceMultiplier ) );
			}
		}

		if ( m_resetOcclusionCamera )
		{
			m_cameraSpeed = Vector::ZEROS;
			for ( Uint32 i = 0; i < 7; ++i ) 
			{
				m_moveKeys[i] = false;
			}
			m_resetOcclusionCamera = false;
		}

		if ( m_attachOcclusionCameraToRenderCamera )
		{
			m_cameraPosition	= m_lastRCPosition;
			m_cameraRotation	= m_lastRCRotation;
			m_cameraFov			= m_lastRCFOV;
			m_cameraAspect		= m_lastRCAspect;
			m_cameraNearPlane	= m_lastRCNearPlane;
			m_cameraFarPlane	= m_lastRCFarPlane;
			return;
		}

		Float speed = 2.0f *m_cameraSpeedMultiplier;
		Float accel = 4.0f;

		Vector startSpeed(speed,speed,speed);
		Vector acceleration(accel,accel,accel);

		if ( m_moveKeys[ UCMK_Sprint ] )
		{
			startSpeed *= 4.f;
			acceleration *= 4.f;
		}

		// calculate input direction vector
		Vector direction(	(Float)(m_moveKeys[ UCMK_StrafeRight ] - m_moveKeys[ UCMK_StrafeLeft ]),
			(Float)(m_moveKeys[ UCMK_Forward ] - m_moveKeys[ UCMK_Back]),
			(Float)(m_moveKeys[ UCMK_Up ] - m_moveKeys[ UCMK_Down ]) );

		// start with default speed if not moving before
		startSpeed *= direction;
		if ( m_cameraSpeed.X == 0 ) 
			m_cameraSpeed.X = startSpeed.X;
		if ( m_cameraSpeed.Y == 0 ) 
			m_cameraSpeed.Y = startSpeed.Y;
		if ( m_cameraSpeed.Z == 0 ) 
			m_cameraSpeed.Z = startSpeed.Z;

		// zero out camera speed if we are stopped moving in that direction (instant stop)
		m_cameraSpeed *= Vector( MAbs( direction.X ), MAbs( direction.Y ), MAbs( direction.Z ) );

		// time delta for camera
		const Float cameraTimeDelta = GEngine->GetLastTimeDelta();

		// update current camera speed
		m_cameraSpeed += direction * acceleration * cameraTimeDelta;

		// update current camera position
		Matrix rot = m_cameraRotation.ToMatrix();
		Vector x = rot.GetAxisX();
		Vector y = rot.GetAxisY();
		Vector z = Vector::EZ;
		Vector delta = (x * m_cameraSpeed.X) + ( y * m_cameraSpeed.Y ) + ( z * m_cameraSpeed.Z );
		m_cameraPosition += delta * cameraTimeDelta * m_cameraSpeedMultiplier;
	}

	void CDebugWindowUmbra::NotifyChangeAttachCamera( RedGui::CRedGuiEventPackage& eventPackage, Bool value )
	{
		RED_UNUSED( eventPackage );

		m_attachOcclusionCameraToRenderCamera = value;
		m_resetOcclusionCamera = true;
	}

	void CreateLabelControl( RedGui::CRedGuiGroupBox* groupBox, CRedGuiUmbraStatLabel*& label, const String& text )
	{
		label = new CRedGuiUmbraStatLabel( 0, 0, 100, 20, text );
		label->SetMargin( Box2( 5, 5, 0, 0 ) );
		label->SetDock( RedGui::DOCK_Top );
		groupBox->AddChild( label );
	}

	void CreateEmptyLine( RedGui::CRedGuiGroupBox* groupBox )
	{
		RedGui::CRedGuiLabel* label = new RedGui::CRedGuiLabel( 0, 0, 100, 20 );
		label->SetMargin( Box2( 5, 5, 0, 0 ) );
		label->SetDock( RedGui::DOCK_Top );
		groupBox->AddChild( label );
	}

	void CDebugWindowUmbra::CreateStatsControls()
	{
		// time stats
		CreateLabelControl( m_timeStatsGroupBox, m_occlusionTime,				TXT("Occlusion time: ") );
		CreateLabelControl( m_timeStatsGroupBox, m_occlusionQueryTime,			TXT("    query: ") );
		CreateLabelControl( m_timeStatsGroupBox, m_occlusionDynamicObjectsTime,	TXT("    dynamic objects: ") );
		CreateLabelControl( m_timeStatsGroupBox, m_visibilityByDistanceTime,	TXT("Time determining visibility by distance: ") );
		CreateLabelControl( m_timeStatsGroupBox, m_furthestProxiesTime,			TXT("Furthest proxies time (all/occl/dist/coll): ") );

		// RenderElementMap stats
		CreateLabelControl( m_reMapStatsGroupBox, m_visibleObjects,			TXT("Visible cooked objects: ") );
		CreateEmptyLine( m_reMapStatsGroupBox );

		CreateLabelControl( m_reMapStatsGroupBox, m_reMapStaticProxies,		TXT("  static proxies (rendered (registered)): ") );
		CreateLabelControl( m_reMapStatsGroupBox, m_reMapDynamicProxies,	TXT("  dynamic proxies (rendered/occluded (registered)): ") );
		CreateLabelControl( m_reMapStatsGroupBox, m_reMapFurthestProxies,	TXT("  furthest proxies (rendered/occluded (registered)): ") );
		CreateLabelControl( m_reMapStatsGroupBox, m_reMapDynamicDecals,		TXT("  dynamic decals (rendered/occluded (registered)): ") );
		CreateEmptyLine( m_reMapStatsGroupBox );

		// further statistics
		CreateLabelControl( m_reMapStatsGroupBox, m_labelReMapStatsStaticMeshes,			TXT("static meshes: ") );
		CreateLabelControl( m_reMapStatsGroupBox, m_labelReMapStatsDynamicMeshes,			TXT("dynamic meshes: ") );
		CreateLabelControl( m_reMapStatsGroupBox, m_labelReMapStatsMeshesNotInObjectCache,	TXT("not in object cache: ") );
		CreateLabelControl( m_reMapStatsGroupBox, m_labelReMapStatsApex,					TXT("apex: ") );
		CreateLabelControl( m_reMapStatsGroupBox, m_labelReMapStatsBakedDecals,				TXT("baked decals: ") );
		CreateLabelControl( m_reMapStatsGroupBox, m_labelReMapStatsNonBakedDecals,			TXT("non-baked decals: ") );
		CreateLabelControl( m_reMapStatsGroupBox, m_labelReMapStatsBakedDimmers,			TXT("baked dimmers: ") );
		CreateLabelControl( m_reMapStatsGroupBox, m_labelReMapStatsNonBakedDimmers,			TXT("non-baked dimmers: ") );
		CreateLabelControl( m_reMapStatsGroupBox, m_labelReMapStatsBakedStripes,			TXT("baked stripes: ") );
		CreateLabelControl( m_reMapStatsGroupBox, m_labelReMapStatsNonBakedStripes,			TXT("non-baked stripes: ") );
		CreateLabelControl( m_reMapStatsGroupBox, m_labelReMapStatsFlares,					TXT("flares: ") );
		CreateLabelControl( m_reMapStatsGroupBox, m_labelReMapStatsFur,						TXT("fur: ") );
		CreateLabelControl( m_reMapStatsGroupBox, m_labelReMapStatsParticles,				TXT("particles: ") );
		CreateLabelControl( m_reMapStatsGroupBox, m_labelReMapStatsBakedPointLights,		TXT("baked point lights: ") );
		CreateLabelControl( m_reMapStatsGroupBox, m_labelReMapStatsNonBakedPointLights,		TXT("non-baked point lights: ") );
		CreateLabelControl( m_reMapStatsGroupBox, m_labelReMapStatsBakedSpotLights,			TXT("baked spot lights: ") );
		CreateLabelControl( m_reMapStatsGroupBox, m_labelReMapStatsNonBakedSpotLights,		TXT("non-baked spot lights: ") );
		
		// shadows
		CreateLabelControl( m_shadowStatsGroupBox, m_shadowLabel,					TXT("Shadows: ") );
		CreateLabelControl( m_shadowStatsGroupBox, m_lblShadowQueryTime,			TXT("Shadow query: ") );
		CreateLabelControl( m_shadowStatsGroupBox, m_lblShadowVisibleStatic,		TXT("Collected static: ") );
		CreateLabelControl( m_shadowStatsGroupBox, m_lblShadowVisibleDynamic,		TXT("Collected dynamic: ") );

		CreateLabelControl( m_shadowStatsGroupBox, m_lblSTStatic,					TXT("Shadows (static): ") );
		CreateLabelControl( m_shadowStatsGroupBox, m_lblSTStaticDistance,			TXT("  Distance: ") );
		CreateLabelControl( m_shadowStatsGroupBox, m_lblSTStaticCollection,			TXT("  Collection: ") );
		CreateLabelControl( m_shadowStatsGroupBox, m_lblSTStaticCulledByDistance,	TXT("  CulledDistance: ") );
		CreateLabelControl( m_shadowStatsGroupBox, m_lblSTDynamic,					TXT("Dynamic: ") );
		CreateLabelControl( m_shadowStatsGroupBox, m_lblSTDynamicDistance,			TXT("  Distance: ") );
		CreateLabelControl( m_shadowStatsGroupBox, m_lblSTDynamicUmbra,				TXT("  Umbra: ") );
		CreateLabelControl( m_shadowStatsGroupBox, m_lblSTDynamicCollection,		TXT("  Collection: ") );
	}

	void CDebugWindowUmbra::FillStatsControls()
	{
		// draw stats
		if ( GGame != nullptr && GGame->GetActiveWorld() != nullptr && GGame->GetActiveWorld()->GetRenderSceneEx() != nullptr )
		{
			const SceneRenderingStats stat = GGame->GetActiveWorld()->GetRenderSceneEx()->GetRenderStats();

			m_visibleObjects->Update( stat.m_visibleObjects );
			m_occlusionTime->Update( String::Printf( TXT("%1.3f ms"), stat.m_occlusionTimeQuery + stat.m_occlusionTimeDynamicObjects ) );
			m_occlusionQueryTime->Update( String::Printf( TXT("%1.3fms"), stat.m_occlusionTimeQuery ) );
			m_occlusionDynamicObjectsTime->Update( String::Printf( TXT("%1.3fms"), stat.m_occlusionTimeDynamicObjects ) );
			m_visibilityByDistanceTime->Update( String::Printf( TXT("%1.3fms"), stat.m_occlusionTimeVisibilityByDistance ) );
			m_furthestProxiesTime->Update( String::Printf( TXT("%1.3fms / %1.3fms / %1.3fms / %1.3fms"), stat.m_furthestProxiesTime, stat.m_furthestProxiesOcclusionTime, stat.m_furthestProxiesDistanceTime, stat.m_furthestProxiesCollectionTime ) );

			// RenderElementMap
			m_reMapStaticProxies->Update( String::Printf( TXT("%d, (%d)"), stat.m_renderedStaticProxies, stat.m_registeredStaticProxies ) );
			m_reMapDynamicProxies->Update( String::Printf( TXT("%d / %d, (%d)"), stat.m_renderedDynamicProxies, stat.m_occludedDynamicProxies, stat.m_registeredDynamicProxies ) );
			m_reMapFurthestProxies->Update( String::Printf( TXT("%d / %d (%d)"), stat.m_renderedFurthestProxies, stat.m_occludedFurthestProxies, stat.m_registeredFurthestProxies ) );
			m_reMapDynamicDecals->Update( String::Printf( TXT("%d / %d, (%d)"), stat.m_renderedDynamicDecals, stat.m_occludedDynamicDecals, stat.m_registeredDynamicDecals ) );

			m_labelReMapStatsStaticMeshes->Update( stat.m_reMapStatsStaticMeshes );
			m_labelReMapStatsDynamicMeshes->Update( stat.m_reMapStatsDynamicMeshes );
			m_labelReMapStatsMeshesNotInObjectCache->Update( stat.m_reMapStatsMeshesNotInObjectCache );
			m_labelReMapStatsApex->Update( stat.m_reMapStatsApex );
			m_labelReMapStatsBakedDecals->Update( stat.m_reMapStatsBakedDecals );
			m_labelReMapStatsNonBakedDecals->Update( stat.m_reMapStatsNonBakedDecals );
			m_labelReMapStatsBakedDimmers->Update( stat.m_reMapStatsBakedDimmers );
			m_labelReMapStatsNonBakedDimmers->Update( stat.m_reMapStatsNonBakedDimmers );
			m_labelReMapStatsBakedStripes->Update( stat.m_reMapStatsBakedStripes );
			m_labelReMapStatsNonBakedStripes->Update( stat.m_reMapStatsNonBakedStripes );
			m_labelReMapStatsFlares->Update( stat.m_reMapStatsFlares );
			m_labelReMapStatsFur->Update( stat.m_reMapStatsFur );
			m_labelReMapStatsParticles->Update( stat.m_reMapStatsParticles );
			m_labelReMapStatsBakedPointLights->Update( stat.m_reMapStatsBakedPointLights );
			m_labelReMapStatsNonBakedPointLights->Update( stat.m_reMapStatsNonBakedPointLights );
			m_labelReMapStatsBakedSpotLights->Update( stat.m_reMapStatsBakedSpotLights );
			m_labelReMapStatsNonBakedSpotLights->Update( stat.m_reMapStatsNonBakedSpotLights );

			// shadows
			m_lblShadowQueryTime->Update( String::Printf( TXT("%1.3fms"), stat.m_shadowQueryTime ) );

			Double totalStatic = m_shadowStaticObjectsSamplesCount * m_avgShadowTimeStaticObjects;
			totalStatic += stat.m_stStatic;
			Double totalDynamic = m_shadowDynamicObjectsSamplesCount * m_avgShadowTimeDynamicObjects;
			totalDynamic += stat.m_stDynamic;

			++m_shadowStaticObjectsSamplesCount;
			++m_shadowDynamicObjectsSamplesCount;

			m_avgShadowTimeStaticObjects = totalStatic / m_shadowStaticObjectsSamplesCount;
			m_avgShadowTimeDynamicObjects = totalDynamic / m_shadowDynamicObjectsSamplesCount;

			m_maxShadowTimeStaticObjects = Max< Double >( m_maxShadowTimeStaticObjects, stat.m_stStatic );
			m_maxShadowTimeDynamicObjects = Max< Double >( m_maxShadowTimeDynamicObjects, stat.m_stDynamic );

			m_lblSTStatic->Update( String::Printf( TXT("%1.3fms (max: %1.3fms, avg: %1.3fms)"), stat.m_stStatic, m_maxShadowTimeStaticObjects, m_avgShadowTimeStaticObjects ) );
			m_lblSTStaticDistance ->Update( String::Printf( TXT("%1.3fms"), stat.m_stStaticDistance ) );
			m_lblSTStaticCollection->Update( String::Printf( TXT("%1.3fms"), stat.m_stStaticCollection ) );
			m_lblSTStaticCulledByDistance->Update( String::Printf( TXT("%u"), stat.m_stStaticCulledByDistance ) );
			
			m_lblSTDynamic->Update( String::Printf( TXT("%1.3fms (max: %1.3fms, avg: %1.3fms)"), stat.m_stDynamic, m_maxShadowTimeDynamicObjects, m_avgShadowTimeDynamicObjects ) );
			m_lblSTDynamicDistance ->Update( String::Printf( TXT("%1.3fms"), stat.m_stDynamicDistance ) );
			m_lblSTDynamicUmbra ->Update( String::Printf( TXT("%1.3fms"), stat.m_stDynamicUmbra ) );
			m_lblSTDynamicCollection->Update( String::Printf( TXT("%1.3fms"), stat.m_stDynamicCollection ) );

			m_lblShadowVisibleStatic->Update( stat.m_renderedShadowStaticProxies );
			m_lblShadowVisibleDynamic->Update( stat.m_renderedShadowDynamicProxies );

			Double total = stat.m_shadowQueryTime + stat.m_stStatic + stat.m_stDynamic;
			m_shadowLabel->Update( String::Printf( TXT("%1.3fms"), total ) );
		}
		else
		{
			m_visibleObjects->Update( 0 );
			m_occlusionTime->Update( TXT("0 ms") );
			m_occlusionQueryTime->Update( TXT("0 ms") );
			m_occlusionDynamicObjectsTime->Update( TXT("0 ms") );
			m_visibilityByDistanceTime->Update( TXT("0 ms") );
			m_furthestProxiesTime->Update( TXT( "0 ms" ) );

			// RenderElementMap
			m_reMapStaticProxies->Update( TXT("0, (0)") );
			m_reMapDynamicProxies->Update( TXT("0 / 0, (0)") );
			m_reMapFurthestProxies->Update( TXT("0 / 0, (0)") );
			m_reMapDynamicDecals->Update( TXT("0 / 0, (0)") );

			m_labelReMapStatsStaticMeshes->Update( 0 );
			m_labelReMapStatsDynamicMeshes->Update( 0 );
			m_labelReMapStatsMeshesNotInObjectCache->Update( 0 );
			m_labelReMapStatsApex->Update( 0 );
			m_labelReMapStatsBakedDecals->Update( 0 );
			m_labelReMapStatsNonBakedDecals->Update( 0 );
			m_labelReMapStatsBakedDimmers->Update( 0 );
			m_labelReMapStatsNonBakedDimmers->Update( 0 );
			m_labelReMapStatsBakedStripes->Update( 0 );
			m_labelReMapStatsNonBakedStripes->Update( 0 );
			m_labelReMapStatsFlares->Update( 0 );
			m_labelReMapStatsFur->Update( 0 );
			m_labelReMapStatsParticles->Update( 0 );
			m_labelReMapStatsBakedPointLights->Update( 0 );
			m_labelReMapStatsNonBakedPointLights->Update( 0 );
			m_labelReMapStatsBakedSpotLights->Update( 0 );
			m_labelReMapStatsNonBakedSpotLights->Update( 0 );

			// shadows
			m_lblShadowQueryTime->Update( TXT("0 ms") );
			m_lblSTStatic->Update( TXT("0 ms (max: 0 ms, avg: 0 ms)") );
			m_lblSTStaticDistance ->Update( TXT(" 0ms") );
			m_lblSTStaticCollection->Update( TXT("0 ms") );
			m_lblSTStaticCulledByDistance->Update( TXT("0") );
			m_lblSTDynamic->Update( TXT("0 ms (max: 0 ms, avg: 0 ms)") );
			m_lblSTDynamicDistance ->Update( TXT("0 ms") );
			m_lblSTDynamicUmbra ->Update( TXT("0 ms") );
			m_lblSTDynamicCollection->Update( TXT("0 ms") );
			m_shadowLabel->Update( TXT("0 ms") );
		}
	}

	void CDebugWindowUmbra::UpdateGenerateUmbraProgress()
	{
	}

	void CDebugWindowUmbra::UpdateMemInfo( Float currentMemory, Float maxMemory, RedGui::CRedGuiProgressBar* progressBar, const String& txt, Float& maxValue )
	{
		maxValue = Max< Float >( maxValue, currentMemory );

		Float inMB = currentMemory / ( 1024.f * 1024.f );
		Float maxInMB = maxValue / ( 1024.f * 1024.f );

		progressBar->SetProgressRange( maxMemory );
		progressBar->SetProgressPosition( currentMemory );
		
		// create progress color
		Float percentage = currentMemory / maxMemory;
		Color c = Color::GREEN;
		if ( percentage >= 0.66f )
		{
			c = Color::RED;
		}
		else if ( percentage >= 0.33f )
		{
			c = Color::YELLOW;
		}
		progressBar->SetProgressBarColor( c );
		progressBar->SetProgressInformation( String::Printf( TXT( "%s: %1.3f%% - %1.3f MB (max: %1.3f MB)" ), txt.AsChar(), percentage, inMB, maxInMB ) );
	}

	void CDebugWindowUmbra::CreateProgressControls()
	{
		m_overallProgressBar = new RedGui::CRedGuiProgressBar( 0, 0, 100, 15 );
		m_overallProgressBar->SetMargin( Box2( 5, 5, 5, 5 ) );
		m_overallProgressBar->SetDock( RedGui::DOCK_Top );
		m_overallProgressBar->SetShowProgressInformation( true );
		m_memoryStatsGroupBox->AddChild( m_overallProgressBar );

		for ( Uint32 i = 0; i < ARRAY_COUNT( MemoryClassesToTrack ); ++i )
		{
			MemoryInfo* info = new MemoryInfo();
			RedGui::CRedGuiProgressBar* pb = new RedGui::CRedGuiProgressBar( 0, 0, 100, 15 );
			pb->SetMargin( Box2( 5, 5, 5, 5 ) );
			pb->SetDock( RedGui::DOCK_Top );
			pb->SetShowProgressInformation( true );
			info->memoryBar = pb;
			info->memClass = MemoryClassesToTrack[ i ];
			info->maxSoFar = 0.0f;
			m_memoryStatsGroupBox->AddChild( pb );
			m_memInfos.PushBack( info );
		}
	}

	void CDebugWindowUmbra::NotifyViewportInput( RedGui::CRedGuiEventPackage& eventPackage, IViewport* view, enum EInputKey key, enum EInputAction action, Float data )
	{
		Bool process = RIM_IS_KEY_DOWN( IK_Alt );
		if ( process && !m_attachOcclusionCameraToRenderCamera )
		{
			if ( ( m_mouseButtonFlags == 2) && key == IK_MouseX )
			{
				m_cameraRotation.Yaw -= data * 0.25f;
			}
			else if ( ( m_mouseButtonFlags == 2) && key == IK_MouseY )
			{
				m_cameraRotation.Pitch -= data * 0.25f;
			}
			else if ( ( m_mouseButtonFlags == 1) && key == IK_MouseY )
			{
				Vector forward;
				EulerAngles angles( 0, 0, m_cameraRotation.Yaw );
				angles.ToAngleVectors( &forward, NULL, NULL );
				m_cameraPosition -= forward * data * 0.05f;
			}
			else if ( ( m_mouseButtonFlags == 1) && key == IK_MouseX )
			{
				m_cameraRotation.Yaw -= data * 0.25f;
			}
			else if ( (m_mouseButtonFlags == 3) && key == IK_MouseY )
			{
				Vector up( 0,0,1 );
				m_cameraPosition -= up * data * 0.05f;
			}
			else if ( (m_mouseButtonFlags == 3) && key == IK_MouseX )
			{
				Vector side;
				EulerAngles angles( 0, 0, m_cameraRotation.Yaw );
				angles.ToAngleVectors( NULL, &side, NULL );
				m_cameraPosition += side * data * 0.05f;
			}
			else if ( key == IK_MouseZ && ( m_mouseButtonFlags == 2 ) )
			{
				m_cameraSpeedMultiplier = Clamp<Float>( m_cameraSpeedMultiplier + data*0.125f, 0.125f, 100.0f );
			}

			// Movement keys
			if ( key == IK_W ) m_moveKeys[ UCMK_Forward ] = ( action == IACT_Press );
			if ( key == IK_S ) m_moveKeys[ UCMK_Back ] = ( action == IACT_Press );
			if ( key == IK_A ) m_moveKeys[ UCMK_StrafeLeft ] = ( action == IACT_Press );
			if ( key == IK_D ) m_moveKeys[ UCMK_StrafeRight ] = ( action == IACT_Press );
			if ( key == IK_Q ) m_moveKeys[ UCMK_Up ] = ( action == IACT_Press );
			if ( key == IK_E ) m_moveKeys[ UCMK_Down ] = ( action == IACT_Press );
			if ( key == IK_LShift || key == IK_RShift ) m_moveKeys[ UCMK_Sprint ] = ( action == IACT_Press );

			eventPackage.SetAsProcessed();
		}

		Bool processPadKey = RIM_IS_KEY_DOWN( IK_Pad_LeftTrigger );
		Float processPadAxis = RIM_GET_AXIS_VALUE( IK_Pad_LeftTrigger );
		if ( processPadAxis && !m_attachOcclusionCameraToRenderCamera )
		{
			Float rAxisX = RIM_GET_AXIS_VALUE( IK_Pad_RightAxisX );
			if ( rAxisX != 0.0f )
			{
				m_cameraRotation.Yaw += data * 0.25f;
			}
			Float rAxisY = RIM_GET_AXIS_VALUE( IK_Pad_RightAxisY );
			if ( rAxisY != 0.0f )
			{
				m_cameraRotation.Pitch += data * 0.25f;
			}

			Float lAxisX = RIM_GET_AXIS_VALUE( IK_Pad_LeftAxisX );
			// Movement keys
			if ( lAxisX != 0.0f )
			{
				m_moveKeys[ UCMK_StrafeLeft ] = lAxisX < 0.0f;
				m_moveKeys[ UCMK_StrafeRight ] = lAxisX > 0.0f;
			}
			Float lAxisY = RIM_GET_AXIS_VALUE( IK_Pad_LeftAxisY );
			if ( lAxisY != 0.0f )
			{
				m_moveKeys[ UCMK_Forward ] = lAxisY > 0.0f;
				m_moveKeys[ UCMK_Back ] = lAxisY < 0.0f;
			}

			m_moveKeys[ UCMK_Sprint ] = ( lAxisX > 0.5f || lAxisX < -0.5f || lAxisY > 0.5f || lAxisY < -0.5f );

			eventPackage.SetAsProcessed();
		}
	}

	void CDebugWindowUmbra::NotifyViewportClick( RedGui::CRedGuiEventPackage& eventPackage, IViewport* view, Int32 button, Bool state, Vector2 mousePosition )
	{
		RED_UNUSED( eventPackage );

		// Update mouse flags
		if ( state )
		{
			m_mouseButtonFlags |= 1 << button;
		}
		else
		{
			m_mouseButtonFlags &= ~(1 << button);
		}
	}

	void CDebugWindowUmbra::NotifyViewportGenerateFragments( RedGui::CRedGuiEventPackage& eventPackage, IViewport* view, CRenderFrame* frame )
	{
		RED_UNUSED( eventPackage );

		const CRenderCamera& camera = frame->GetFrameInfo().m_camera;
		m_lastRCPosition			= camera.GetPosition();
		m_lastRCRotation			= camera.GetRotation();
		m_lastRCFOV					= camera.GetFOV();
		m_lastRCAspect				= camera.GetAspect();
		m_lastRCNearPlane			= camera.GetNearPlane();
		m_lastRCFarPlane			= camera.GetFarPlane();
		m_lastRCReversedProjection	= camera.IsReversedProjection();

		if ( !m_attachOcclusionCameraToRenderCamera )
		{
			frame->GetFrameInfo().m_occlusionCamera.Set( m_cameraPosition, m_cameraRotation, m_cameraFov, m_cameraAspect, m_cameraNearPlane, m_cameraFarPlane );
			frame->GetFrameInfo().m_occlusionCamera.SetReversedProjection( m_lastRCReversedProjection );
		}
	}

	void CDebugWindowUmbra::QueryThresholdChanged( RedGui::CRedGuiEventPackage& eventPackage, Float value )
	{
		m_queryThresholdLabel->Update( String::Printf( TXT("%1.2f"), value ) );

		if ( GGame && GGame->GetActiveWorld() )
		{
			if ( IRenderScene* renderScene = GGame->GetActiveWorld()->GetRenderSceneEx() )
			{
				(new CRenderCommand_UpdateQueryThreshold( renderScene, value ))->Commit();
			}
		}
	}

	void CDebugWindowUmbra::DistanceMultiplierChanged( RedGui::CRedGuiEventPackage& eventPackage, Float value )
	{
		m_distanceMultiplier = value;
		m_distanceMultiplierLabel->Update( String::Printf( TXT("%1.2f"), m_distanceMultiplier ) );

		if ( GGame && GGame->GetActiveWorld() )
		{
			if ( CUmbraScene* umbraScene = GGame->GetActiveWorld()->GetUmbraScene() )
			{
				umbraScene->SetDistanceMultiplier( m_distanceMultiplier );
			}
		}
	}

	void CDebugWindowUmbra::DumpVisibleMeshes( RedGui::CRedGuiEventPackage& eventPackage )
	{
		m_saveFileDialog->SetVisible(true);
	}

	void CDebugWindowUmbra::NotifyEventFileOK( RedGui::CRedGuiEventPackage& eventPackage )
	{
#ifndef NO_EDITOR
		if ( CWorld* world = GGame->GetActiveWorld() )
		{
			CUmbraScene* umbraScene = world->GetUmbraScene();
			const TObjectCache& objectCache = umbraScene->GetObjectCache();

			TLoadedComponentsMap componentMap;

			TDynArray< CMeshComponent* > components;
			world->GetAttachedComponentsOfClass< CMeshComponent >( components );
			for ( auto& component : components )
			{
				if ( !component->GetEntity() )
				{
					continue;
				}
				if ( !component->GetLayer() || !component->GetLayer()->GetLayerInfo() )
				{
					continue;
				}

				CMesh* mesh = component->GetMeshNow();
				if ( !mesh )
				{
					continue;
				}

				Uint32 transformHash = UmbraHelpers::CalculateTransformHash( component->GetLocalToWorld() );
				Uint32 modelId = GetHash( mesh->GetDepotPath() );
				TObjectCacheKeyType objectCacheKey = UmbraHelpers::CompressToKeyType( modelId, transformHash );
				TObjectIdType objectId;
				if ( !objectCache.Find( objectCacheKey, objectId ) )
				{
					continue;
				}

				ComponentDesctiprion desc;
				desc.meshDepoPath	= mesh->GetDepotPath();
				desc.layerPath		= component->GetLayer()->GetLayerInfo()->GetDepotPath();
				desc.entityName		= component->GetEntity()->GetName();
				desc.chunkCount		= mesh->GetChunks().Size();
				desc.triangleCount	= 0;
				desc.vertexCount	= 0;

				componentMap.Insert( objectId, desc );
			}

			String path = String::EMPTY;
			GDepot->GetAbsolutePath( path );
			path += m_saveFileDialog->GetFileName();

			( new CRenderCommand_DumpVisibleMeshes( world->GetRenderSceneEx(), componentMap, path ) )->Commit();
		}
		
		GRedGui::GetInstance().MessageBox( TXT("File has been saved correctly."), TXT("Success"), RedGui::MESSAGEBOX_Info );
#endif // NO_EDITOR
	}
}	// namespace DebugWindows

#endif  // USE_UMBRA
#endif	// NO_DEBUG_WINDOWS
#endif	// NO_RED_GUI
