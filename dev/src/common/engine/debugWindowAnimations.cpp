/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#ifndef NO_RED_GUI
#ifndef NO_DEBUG_WINDOWS

#include "staticCamera.h"
#include "redGuiGroupBox.h"
#include "redGuiTab.h"
#include "redGuiGridLayout.h"
#include "redGuiButton.h"
#include "redGuiManager.h"
#include "redGuiLabel.h"
#include "redGuiPanel.h"
#include "redGuiScrollPanel.h"
#include "redGuiList.h"
#include "redGuiListItem.h"
#include "redGuiProgressBar.h"
#include "debugWindowAnimations.h"
#include "animationManager.h"
#include "entity.h"
#include "actorInterface.h"
#include "animatedComponent.h"
#include "cutscene.h"
#include "animationIterator.h"
#include "skeletalAnimation.h"
#include "game.h"
#include "skeleton.h"
#include "world.h"
#include "worldIterators.h"
#include "tickManager.h"
#include "baseEngine.h"
#include "../core/diskFile.h"
#include "../core/objectIterator.h"
#include "poseProvider.h"

namespace DebugWindows
{
	CDebugWindowAnimations::CDebugWindowAnimations()
		: RedGui::CRedGuiWindow( 200, 200, 1000, 600 )
	{
		GRedGui::GetInstance().EventTick.Bind( this, &CDebugWindowAnimations::NotifyOnTick );

		SetCaption( TXT("Animations") );

		RedGui::CRedGuiGroupBox* groupBox = new RedGui::CRedGuiGroupBox( 0, 0, 1000, 125 );
		groupBox->SetMargin( Box2( 3, 3, 3, 3 ) );
		groupBox->SetDock( RedGui::DOCK_Top );
		groupBox->SetText( TXT("Fast viewer") );
		AddChild( groupBox );

		RedGui::CRedGuiGridLayout* layout = new RedGui::CRedGuiGridLayout( 0, 0, 100, 100 );
		layout->SetDock( RedGui::DOCK_Fill );
		layout->SetDimensions( 3, 5 );
		groupBox->AddChild( layout );

		// add progress bars
		{
			m_totalSizeProgressBar = new RedGui::CRedGuiProgressBar( 0, 0, 100, 100 );
			m_totalSizeProgressBar->SetMargin( Box2( 2 ,2, 2, 2) );
			m_totalSizeProgressBar->SetShowProgressInformation( true );
			layout->AddChild( m_totalSizeProgressBar );

			m_streamingAnimsProgressBar = new RedGui::CRedGuiProgressBar( 0, 0, 100, 100 );
			m_streamingAnimsProgressBar->SetMargin( Box2( 2 ,2, 2, 2) );
			m_streamingAnimsProgressBar->SetShowProgressInformation( true );
			layout->AddChild( m_streamingAnimsProgressBar );

			m_loadedCutscenesProgressBar = new RedGui::CRedGuiProgressBar( 0, 0, 100, 100 );
			m_loadedCutscenesProgressBar->SetMargin( Box2( 2 ,2, 2, 2) );
			m_loadedCutscenesProgressBar->SetShowProgressInformation( true );
			layout->AddChild( m_loadedCutscenesProgressBar );

			m_animSizeProgressBar = new RedGui::CRedGuiProgressBar( 0, 0, 100, 100 );
			m_animSizeProgressBar->SetMargin( Box2( 2 ,2, 2, 2) );
			m_animSizeProgressBar->SetShowProgressInformation( true );
			layout->AddChild( m_animSizeProgressBar );

			m_streamedInAnimsProgressBar = new RedGui::CRedGuiProgressBar( 0, 0, 100, 100 );
			m_streamedInAnimsProgressBar->SetMargin( Box2( 2 ,2, 2, 2) );
			m_streamedInAnimsProgressBar->SetShowProgressInformation( true );
			layout->AddChild( m_streamedInAnimsProgressBar );

			m_loadedExDialogsProgressBar = new RedGui::CRedGuiProgressBar( 0, 0, 100, 100 );
			m_loadedExDialogsProgressBar->SetMargin( Box2( 2 ,2, 2, 2) );
			m_loadedExDialogsProgressBar->SetShowProgressInformation( true );
			layout->AddChild( m_loadedExDialogsProgressBar );

			m_motionExtractionSizeProgressBar = new RedGui::CRedGuiProgressBar( 0, 0, 100, 100 );
			m_motionExtractionSizeProgressBar->SetMargin( Box2( 2 ,2, 2, 2) );
			m_motionExtractionSizeProgressBar->SetShowProgressInformation( true );
			layout->AddChild( m_motionExtractionSizeProgressBar );

			m_usedStreamedInAnimsProgressBar = new RedGui::CRedGuiProgressBar( 0, 0, 100, 100 );
			m_usedStreamedInAnimsProgressBar->SetMargin( Box2( 2 ,2, 2, 2) );
			m_usedStreamedInAnimsProgressBar->SetShowProgressInformation( true );
			layout->AddChild( m_usedStreamedInAnimsProgressBar );

			m_streamedAnimSizeProgressBar = new RedGui::CRedGuiProgressBar( 0, 0, 100, 100 );
			m_streamedAnimSizeProgressBar->SetMargin( Box2( 2 ,2, 2, 2) );
			m_streamedAnimSizeProgressBar->SetShowProgressInformation( true );
			layout->AddChild( m_streamedAnimSizeProgressBar );

			m_compresedFramesSizeProgressBar = new RedGui::CRedGuiProgressBar( 0, 0, 100, 100 );
			m_compresedFramesSizeProgressBar->SetMargin( Box2( 2 ,2, 2, 2) );
			m_compresedFramesSizeProgressBar->SetShowProgressInformation( true );
			layout->AddChild( m_compresedFramesSizeProgressBar );

			m_activeAnimCountProgressBar = new RedGui::CRedGuiProgressBar( 0, 0, 100, 100 );
			m_activeAnimCountProgressBar->SetMargin( Box2( 2 ,2, 2, 2) );
			m_activeAnimCountProgressBar->SetShowProgressInformation( true );
			layout->AddChild( m_activeAnimCountProgressBar );

			m_nonStreamableAnimSizeProgressBar = new RedGui::CRedGuiProgressBar( 0, 0, 100, 100 );
			m_nonStreamableAnimSizeProgressBar->SetMargin( Box2( 2 ,2, 2, 2) );
			m_nonStreamableAnimSizeProgressBar->SetShowProgressInformation( true );
			layout->AddChild( m_nonStreamableAnimSizeProgressBar );

			m_totalPosesMemoryProgressBar = new RedGui::CRedGuiProgressBar( 0, 0, 100, 100 );
			m_totalPosesMemoryProgressBar->SetMargin( Box2( 2 ,2, 2, 2) );
			m_totalPosesMemoryProgressBar->SetShowProgressInformation( true );
			layout->AddChild( m_totalPosesMemoryProgressBar );

			m_wholeAnimDataMemoryProgressBar = new RedGui::CRedGuiProgressBar( 0, 0, 100, 100 );
			m_wholeAnimDataMemoryProgressBar->SetMargin( Box2( 2 ,2, 2, 2) );
			m_wholeAnimDataMemoryProgressBar->SetShowProgressInformation( true );
			layout->AddChild( m_wholeAnimDataMemoryProgressBar );

			m_totalAnimMemoryProgressBar = new RedGui::CRedGuiProgressBar( 0, 0, 100, 100 );
			m_totalAnimMemoryProgressBar->SetMargin( Box2( 2 ,2, 2, 2) );
			m_totalAnimMemoryProgressBar->SetShowProgressInformation( true );
			layout->AddChild( m_totalAnimMemoryProgressBar );
		}

		CreateTabs();
	}

	CDebugWindowAnimations::~CDebugWindowAnimations()
	{
		/* intentionally empty */
	}

	void CDebugWindowAnimations::CreateTabs()
	{
		m_tabs = new RedGui::CRedGuiTab( 0, 0, 100, 100 );
		m_tabs->SetMargin( Box2( 3, 3, 3, 3 ) );
		m_tabs->SetDock( RedGui::DOCK_Fill );
		AddChild( m_tabs );

		for( Uint32 i=0; i<TN_Count; ++i )
		{
			m_tabs->AddTab( EnumToString( static_cast< ETabName >( i ) ) );
		}

		CreatePosesTab();
		CreateComponentsTab();
		CreateAnimsetsTab();
		CreateCutscenesTab();

		m_tabs->SetActiveTab( TN_Poses );
	}

	String CDebugWindowAnimations::EnumToString( ETabName value ) const
	{
		switch( value )
		{
		case TN_Poses:
			return TXT("Poses");
		case TN_Components:
			return TXT("Components");
		case TN_RestDetails:
			return TXT("RestDetails");
		case TN_Animsets:
			return TXT("Animsets");
		case TN_Cutscenes:
			return TXT("Cutscenes");
		}

		return String::EMPTY;
	}

	void CDebugWindowAnimations::CreatePosesTab()
	{
		// create textures tab
		RedGui::CRedGuiScrollPanel* posesTab = m_tabs->GetTabAt( TN_Poses );
		if( posesTab != nullptr )
		{
			// create info panel about textures
			RedGui::CRedGuiPanel* posesGeneralInfo = new RedGui::CRedGuiPanel( 0, 0, 250, 30 );
			posesGeneralInfo->SetMargin( Box2(5, 5, 5, 5) );
			posesGeneralInfo->SetPadding( Box2(5, 5, 5, 5) );
			posesGeneralInfo->SetBackgroundColor( Color(20, 20, 20, 255) );
			posesGeneralInfo->SetDock( RedGui::DOCK_Top );
			posesTab->AddChild( posesGeneralInfo );
			{
				RedGui::CRedGuiGridLayout* layout = new RedGui::CRedGuiGridLayout( 0, 0, 100, 100 );
				layout->SetDock( RedGui::DOCK_Fill );
				layout->SetDimensions( 4, 1 );
				posesGeneralInfo->AddChild( layout );

				m_allocators = new RedGui::CRedGuiLabel( 0, 0, 100, 25 );
				m_allocators->SetText( TXT("Allocators: ") );
				layout->AddChild( m_allocators );

				m_totalPoses = new RedGui::CRedGuiLabel( 0, 0, 100, 25 );
				m_totalPoses->SetText( TXT("Total poses: ") );
				layout->AddChild( m_totalPoses );

				m_cachedPoses = new RedGui::CRedGuiLabel( 0, 0, 100, 25 );
				m_cachedPoses->SetText( TXT("Cached poses: ") );
				layout->AddChild( m_cachedPoses );

				m_allocPoses = new RedGui::CRedGuiLabel( 0, 0, 100, 25 );
				m_allocPoses->SetText( TXT("Alloc poses: ") );
				layout->AddChild( m_allocPoses );
			}

			m_posesList = new RedGui::CRedGuiList( 0, 0, 100, 100 );
			m_posesList->SetMargin( Box2(5, 5, 5, 5) );
			m_posesList->SetPadding( Box2(5, 5, 5, 5) );
			m_posesList->SetBackgroundColor( Color(20, 20, 20, 255) );
			m_posesList->SetDock( RedGui::DOCK_Fill );
			m_posesList->AppendColumn( TXT("Skeleton"), 200 );
			m_posesList->AppendColumn( TXT("Poses"), 100, RedGui::SA_Integer );
			m_posesList->AppendColumn( TXT("Poses [MB]"), 100, RedGui::SA_Real );
			m_posesList->AppendColumn( TXT("Cache"), 100, RedGui::SA_Integer );
			m_posesList->AppendColumn( TXT("Cache [MB]"), 100, RedGui::SA_Real );
			m_posesList->AppendColumn( TXT("Alloc"), 100, RedGui::SA_Integer );
			m_posesList->AppendColumn( TXT("Alloc [MB]"), 100, RedGui::SA_Real );
			m_posesList->AppendColumn( TXT("F Cached"), 100, RedGui::SA_Integer );
			m_posesList->AppendColumn( TXT("F Cached [MB]"), 100, RedGui::SA_Real );
			m_posesList->AppendColumn( TXT("F Alloc"), 100, RedGui::SA_Integer );
			m_posesList->AppendColumn( TXT("F Alloc [MB]"), 100, RedGui::SA_Real );
			m_posesList->SetSorting( true );
			m_posesList->SetSelectionMode( RedGui::SM_None );
			posesTab->AddChild( m_posesList );
		}
	}

	void CDebugWindowAnimations::CreateComponentsTab()
	{
		// create textures tab
		RedGui::CRedGuiScrollPanel* componentsTab = m_tabs->GetTabAt( TN_Components );
		if( componentsTab != nullptr )
		{
			// create info panel about textures
			RedGui::CRedGuiPanel* posesGeneralInfo = new RedGui::CRedGuiPanel( 0, 0, 250, 30 );
			posesGeneralInfo->SetMargin( Box2(5, 5, 5, 5) );
			posesGeneralInfo->SetPadding( Box2(5, 5, 5, 5) );
			posesGeneralInfo->SetBackgroundColor( Color(20, 20, 20, 255) );
			posesGeneralInfo->SetDock( RedGui::DOCK_Top );
			componentsTab->AddChild( posesGeneralInfo );
			{
				RedGui::CRedGuiGridLayout* layout = new RedGui::CRedGuiGridLayout( 0, 0, 100, 100 );
				layout->SetDock( RedGui::DOCK_Fill );
				layout->SetDimensions( 4, 1 );
				posesGeneralInfo->AddChild( layout );

				m_animatedComponents = new RedGui::CRedGuiLabel( 0, 0, 100, 25 );
				m_animatedComponents->SetText( TXT("Animated Components: ") );
				layout->AddChild( m_animatedComponents );

				m_activeComponents = new RedGui::CRedGuiLabel( 0, 0, 100, 25 );
				m_activeComponents->SetText( TXT("Active: ") );
				layout->AddChild( m_activeComponents );

				m_frozenComponents = new RedGui::CRedGuiLabel( 0, 0, 100, 25 );
				m_frozenComponents->SetText( TXT("Frozen: ") );
				layout->AddChild( m_frozenComponents );

				m_budgetedComponents = new RedGui::CRedGuiLabel( 0, 0, 100, 25 );
				m_budgetedComponents->SetText( TXT("Budgeted: ") );
				layout->AddChild( m_budgetedComponents );
			}

			m_componentsList = new RedGui::CRedGuiList( 0, 0, 100, 100 );
			m_componentsList->SetMargin( Box2(5, 5, 5, 5) );
			m_componentsList->SetPadding( Box2(5, 5, 5, 5) );
			m_componentsList->SetBackgroundColor( Color(20, 20, 20, 255) );
			m_componentsList->SetDock( RedGui::DOCK_Fill );
			m_componentsList->AppendColumn( TXT("Type"), 200 );
			m_componentsList->AppendColumn( TXT("Active"), 100, RedGui::SA_Integer );
			m_componentsList->AppendColumn( TXT("All"), 100, RedGui::SA_Integer );
			m_componentsList->AppendColumn( TXT("Frozen"), 100, RedGui::SA_Integer );
			m_componentsList->AppendColumn( TXT("Budgeted"), 100, RedGui::SA_Integer );
			m_componentsList->AppendColumn( TXT("Work"), 100, RedGui::SA_Integer );
			m_componentsList->AppendColumn( TXT("Cutscene"), 100, RedGui::SA_Integer );
			m_componentsList->AppendColumn( TXT("Beh Contrl"), 100, RedGui::SA_Integer );
			m_componentsList->SetSorting( true );
			m_componentsList->SetSelectionMode( RedGui::SM_None );
			componentsTab->AddChild( m_componentsList );

			// add all items
			m_componentsList->AddItem( TXT("Actors") );
			m_componentsList->AddItem( TXT("Heads") );
			m_componentsList->AddItem( TXT("Cameras") );
			m_componentsList->AddItem( TXT("Items") );
			m_componentsList->AddItem( TXT("Static cameras") );
			m_componentsList->AddItem( TXT("Rest") );
		}

		RedGui::CRedGuiScrollPanel* restTab = m_tabs->GetTabAt( TN_RestDetails );
		if( restTab != nullptr )
		{
			m_restList = new RedGui::CRedGuiList( 0, 0, 100, 200 );
			m_restList->SetMargin( Box2(5, 5, 5, 5) );
			m_restList->SetPadding( Box2(5, 5, 5, 5) );
			m_restList->SetBackgroundColor( Color(20, 20, 20, 255) );
			m_restList->SetDock( RedGui::DOCK_Top );
			m_restList->AppendColumn( TXT("Skeleton"), 200 );
			m_restList->AppendColumn( TXT("Count"), 100, RedGui::SA_Integer );
			m_restList->AppendColumn( TXT("Frozen"), 100, RedGui::SA_Integer );
			m_restList->AppendColumn( TXT("Budgeted"), 100, RedGui::SA_Integer );
			m_restList->SetSorting( true );
			m_restList->SetSelectionMode( RedGui::SM_Single );
			restTab->AddChild( m_restList );

			m_restTamplates = new RedGui::CRedGuiList( 0, 0, 100, 100 );
			m_restTamplates->SetMargin( Box2(5, 5, 5, 5) );
			m_restTamplates->SetPadding( Box2(5, 5, 5, 5) );
			m_restTamplates->SetBackgroundColor( Color(20, 20, 20, 255) );
			m_restTamplates->SetDock( RedGui::DOCK_Fill );
			m_restTamplates->AppendColumn( TXT("Template"), 200 );
			m_restTamplates->AppendColumn( TXT("Count"), 100, RedGui::SA_Integer );
			m_restTamplates->AppendColumn( TXT("Frozen"), 100, RedGui::SA_Integer );
			m_restTamplates->AppendColumn( TXT("Budgeted"), 100, RedGui::SA_Integer );
			m_restTamplates->SetSorting( true );
			m_restTamplates->SetSelectionMode( RedGui::SM_None );
			restTab->AddChild( m_restTamplates );
		}
	}

	void CDebugWindowAnimations::CreateAnimsetsTab()
	{
		// create textures tab
		RedGui::CRedGuiScrollPanel* animSetTab = m_tabs->GetTabAt( TN_Animsets );
		if( animSetTab != nullptr )
		{
			// create info panel about textures
			{
				RedGui::CRedGuiPanel* animSetGeneralInfo = new RedGui::CRedGuiPanel( 0, 0, 250, 30 );
				animSetGeneralInfo->SetMargin( Box2(5, 5, 5, 5) );
				animSetGeneralInfo->SetPadding( Box2(5, 5, 5, 5) );
				animSetGeneralInfo->SetBackgroundColor( Color(20, 20, 20, 255) );
				animSetGeneralInfo->SetDock( RedGui::DOCK_Top );
				animSetTab->AddChild( animSetGeneralInfo );

				RedGui::CRedGuiGridLayout* layout = new RedGui::CRedGuiGridLayout( 0, 0, 100, 100 );
				layout->SetDock( RedGui::DOCK_Fill );
				layout->SetDimensions( 1, 1 );
				animSetGeneralInfo->AddChild( layout );

				m_animsetCount = new RedGui::CRedGuiLabel( 0, 0, 100, 25 );
				m_animsetCount->SetText( TXT("Animset count: ") );
				layout->AddChild( m_animsetCount );
			}

			{
				RedGui::CRedGuiPanel* animSetDumpInfo = new RedGui::CRedGuiPanel( 0, 0, 250, 40 );
				animSetDumpInfo->SetMargin( Box2(5, 5, 5, 5) );
				animSetDumpInfo->SetPadding( Box2(5, 5, 5, 5) );
				animSetDumpInfo->SetBackgroundColor( Color(20, 20, 20, 255) );
				animSetDumpInfo->SetDock( RedGui::DOCK_Bottom );
				animSetTab->AddChild( animSetDumpInfo );

				RedGui::CRedGuiGridLayout* layout = new RedGui::CRedGuiGridLayout( 0, 0, 100, 100 );
				layout->SetDock( RedGui::DOCK_Fill );
				layout->SetDimensions( 1, 1 );
				animSetDumpInfo->AddChild( layout );

				animSetDumpButton= new RedGui::CRedGuiButton( 0, 0, 250, 30 );
				animSetDumpButton->SetText(TXT("Dump Animset info"));
				animSetDumpButton->EventButtonClicked.Bind(this, &CDebugWindowAnimations::DumpAnimsetsInfo);
				layout->AddChild( animSetDumpButton );
			}

			{
				m_animSetList = new RedGui::CRedGuiList( 0, 0, 100, 100 );
				m_animSetList->SetMargin( Box2(5, 5, 5, 5) );
				m_animSetList->SetPadding( Box2(5, 5, 5, 5) );
				m_animSetList->SetBackgroundColor( Color(20, 20, 20, 255) );
				m_animSetList->SetDock( RedGui::DOCK_Fill );
				m_animSetList->AppendColumn( TXT("Name"), 300 );
				m_animSetList->AppendColumn( TXT("Total used memory [MB]"), 200, RedGui::SA_Real );
				m_animSetList->AppendColumn( TXT("Streamed anim memory [MB]"), 200, RedGui::SA_Real );
				m_animSetList->AppendColumn( TXT("Non streamable anim memory [MB]"), 200, RedGui::SA_Real );
				m_animSetList->AppendColumn( TXT("Whole anim memory [MB]"), 200, RedGui::SA_Real );
				m_animSetList->AppendColumn( TXT("Active used memory [MB]"), 200, RedGui::SA_Real );
				m_animSetList->AppendColumn( TXT("Active animation count"), 200, RedGui::SA_Integer );
				m_animSetList->SetSorting( true );
				m_animSetList->SetSelectionMode( RedGui::SM_None );
				animSetTab->AddChild( m_animSetList );
			}
		}
	}

	void CDebugWindowAnimations::CreateCutscenesTab()
	{
		// create textures tab
		RedGui::CRedGuiScrollPanel* cutsceneTab = m_tabs->GetTabAt( TN_Cutscenes );
		if( cutsceneTab != nullptr )
		{
			RedGui::CRedGuiGridLayout* layout = new RedGui::CRedGuiGridLayout( 0, 0, 100, 100 );
			layout->SetDock( RedGui::DOCK_Fill );
			layout->SetDimensions( 1, 2 );
			cutsceneTab->AddChild( layout );

			m_cutsceneResourcesList = new RedGui::CRedGuiList( 0, 0, 100, 100 );
			m_cutsceneResourcesList->SetMargin( Box2(5, 5, 5, 5) );
			m_cutsceneResourcesList->SetPadding( Box2(5, 5, 5, 5) );
			m_cutsceneResourcesList->SetBackgroundColor( Color(20, 20, 20, 255) );
			m_cutsceneResourcesList->SetDock( RedGui::DOCK_Fill );
			m_cutsceneResourcesList->AppendColumn( TXT("Resource path"), 200 );
			m_cutsceneResourcesList->SetSelectionMode( RedGui::SM_None );
			layout->AddChild( m_cutsceneResourcesList );

			m_cutsceneInstancesList = new RedGui::CRedGuiList( 0, 0, 100, 100 );
			m_cutsceneInstancesList->SetMargin( Box2(5, 5, 5, 5) );
			m_cutsceneInstancesList->SetPadding( Box2(5, 5, 5, 5) );
			m_cutsceneInstancesList->SetBackgroundColor( Color(20, 20, 20, 255) );
			m_cutsceneInstancesList->SetDock( RedGui::DOCK_Fill );
			m_cutsceneInstancesList->AppendColumn( TXT("Instances"), 200 );
			m_cutsceneInstancesList->SetSelectionMode( RedGui::SM_None );
			layout->AddChild( m_cutsceneInstancesList );
		}
	}

	void CDebugWindowAnimations::NotifyOnTick( RedGui::CRedGuiEventPackage& eventPackage, Float deltaTime )
	{
		if( GetVisible() == true )
		{
			UpdateTabs();

			if( GAnimationManager != nullptr )
			{
				Int32 graphSize = GAnimationManager ? GAnimationManager->GetGraphStat().m_size : 0;
				Int32 instancesSize = GAnimationManager ? GAnimationManager->GetGraphInstanceStat().m_size : 0;
				Int32 animsStaticSize = GAnimationManager ? GAnimationManager->GetAnimStaticStat().m_size : 0;
				Uint32 totalStreamedAnimationsMemory = GAnimationManager ? GAnimationManager->GetCurrentPoolSize() : 0;

				Int32 graphNum = GAnimationManager ? GAnimationManager->GetGraphStat().m_num : 0;
				Int32 instancesNum = GAnimationManager ? GAnimationManager->GetGraphInstanceStat().m_num : 0;
				Int32 animsStaticNum = GAnimationManager ? GAnimationManager->GetAnimStaticStat().m_num : 0;

				Uint32 numStreamingAnimation = 0;
				Uint32 numLoadedAnimation = 0;
				Uint32 numStreamedInAnimation = 0;
				Uint32 numUsedStreamedInAnimation = 0;
				Float loadedAnimationSize = 0.f;
				CalcNumStreamingAndLoadedAnim( numStreamingAnimation, numLoadedAnimation, loadedAnimationSize );

				Uint32 numBCStreamingAnimation = 0;
				Uint32 numBCStreamedInAnimation = 0;
				Uint32 numBCUsedStreamedInAnimation = 0;
				CAnimationBufferBitwiseCompressed::GetStreamingNumbers(numBCStreamedInAnimation, numBCUsedStreamedInAnimation, numBCStreamingAnimation);
				numStreamingAnimation += numBCStreamingAnimation;
				numStreamedInAnimation += numBCStreamedInAnimation;
				numUsedStreamedInAnimation += numBCUsedStreamedInAnimation;

				const Float megInv = 1.0f / ( 1024.0f * 1024.0f );

				const Float poolSize = GAnimationManager ? (Float)GAnimationManager->GetMaxPoolSize() * megInv: 30.f;
				Float streamablePoolSize;
				Float streamableAnimLimit;
				{
					Uint32 uStreamablePoolSize;
					Uint32 uStreamableAnimLimit;
					CAnimationBufferBitwiseCompressed::GetLimitsForStreamedData(uStreamableAnimLimit, uStreamablePoolSize);
					streamablePoolSize = (Float)uStreamablePoolSize * megInv;
					streamableAnimLimit = (Float)uStreamableAnimLimit;
				}

				// update progress bars
				UpdateProgressBar( m_totalSizeProgressBar, m_totalMemory * megInv, poolSize, TXT("Total size"), TXT("MB") );
				UpdateProgressBar( m_streamingAnimsProgressBar, (Float)numStreamingAnimation, 5.0f, TXT("Streaming anims"), TXT("")  );
				UpdateProgressBar( m_loadedCutscenesProgressBar, m_csSize * megInv , poolSize, TXT("Loaded cutscenes"), TXT("MB") );

				UpdateProgressBar( m_animSizeProgressBar, m_totalAnimationsMemory * megInv, poolSize, TXT("Anim size"), TXT("MB") );
				UpdateProgressBar( m_streamedInAnimsProgressBar, (Float)numStreamedInAnimation, streamableAnimLimit, TXT("Streamed in anims"), TXT("") );
				UpdateProgressBar( m_loadedExDialogsProgressBar, m_exDialogSize * megInv, poolSize, TXT("Loaded ex dialogs"), TXT("MB") );

				UpdateProgressBar( m_motionExtractionSizeProgressBar, m_totalAnimationsMotionExMemory * megInv, 2.5f, TXT("Motion extraction size"), TXT("MB") );
				UpdateProgressBar( m_usedStreamedInAnimsProgressBar, (Float)numUsedStreamedInAnimation, streamableAnimLimit, TXT("Used streamed in anims"), TXT("") );
				UpdateProgressBar( m_streamedAnimSizeProgressBar, (m_totalAnimationsStreamedMemory + totalStreamedAnimationsMemory) * megInv, streamablePoolSize, TXT("Anim size - streamed"), TXT("MB") );

				String cfmStr = String::Printf( TXT("Compressed frames size %1.2f"), m_totalAnimationsCompressedFrameDataMemory * megInv );
				UpdateProgressBar( m_compresedFramesSizeProgressBar, m_totalAnimationsCompressedFrameMemory * megInv, 2.5f, cfmStr, TXT("MB") );
				UpdateProgressBar( m_activeAnimCountProgressBar, (Float)m_numActiveAnimations, 200.0f, TXT("Active animations"), TXT("") );
				UpdateProgressBar( m_nonStreamableAnimSizeProgressBar, (m_totalAnimationsMemory - (m_totalAnimationsStreamedMemory + totalStreamedAnimationsMemory)) * megInv, poolSize, TXT("Anim size - non streamable"), TXT("MB") );

				UpdateProgressBar( m_totalPosesMemoryProgressBar, m_totalPosesMemory * megInv, GAnimationManager->GetPosesPoolSize() * megInv, TXT("Poses"), TXT("MB") );
				UpdateProgressBar( m_wholeAnimDataMemoryProgressBar, m_wholeAnimationsMemory * megInv, 512.0f, TXT("Anim size - ALL (loaded and not)"), TXT("MB") ); // event not streamed in!
				UpdateProgressBar( m_totalAnimMemoryProgressBar, m_totalAnimationsMemory * megInv, poolSize, TXT("Anim size - total"), TXT("MB") );
			}
		}
	}

	void CDebugWindowAnimations::UpdateTabs()
	{
		UpdatePosesTab();
		UpdateComponentsTab();
		UpdateAnimsetsTab();
		UpdateCutscenesTab();
	}

	void CDebugWindowAnimations::UpdatePosesTab()
	{
		static const Float invMb = 1.f / ( 1024.f * 1024.f );

		Uint32 memTotal = 0;
		Uint32 memAlloc = 0;
		Uint32 memCached = 0;
		Uint32 numTotal = 0;
		Uint32 numAlloc = 0;
		Uint32 numCached = 0;

		// remove invalid rows
		{
			Uint32 poseCount = m_posesList->GetItemCount();
			for( Uint32 i=0; i<poseCount; ++i )
			{
				RedGui::CRedGuiListItem* item = m_posesList->GetItem(i);
				if( item != nullptr )
				{
					SSkeletonInfo* info = item->GetUserData< SSkeletonInfo >();
					if( info != nullptr )
					{
						if( info->m_skeleton.Get() == nullptr )
						{
							if ( info->m_stats.m_memTotal <= 0.0f || info->m_stats.m_numTotal <= 0 )
							{
								delete info;
								m_posesList->RemoveItem( i );

								if( i != 0 )
								{
									--poseCount;
									--i;
								}
							}
						}
					}
				}
			}
		}

		// Collect all skeletons
		{
			for ( ObjectIterator<CSkeleton> it; it; ++it )
			{
				// Find existing skeletons
				CSkeleton* skeleton = *it;
				SSkeletonInfo* info = nullptr;
	
				const Uint32 poseCount = m_posesList->GetItemCount();
				for( Uint32 j=0; j<poseCount; ++j )
				{
					SSkeletonInfo* test = m_posesList->GetItem(j)->GetUserData< SSkeletonInfo >();
					if( test != nullptr )
					{
						if ( test->m_skeleton.Get() == skeleton )
						{
							info = test;
							break;
						}
					}

				}
	
				// Add new
				if ( info == nullptr )
				{
					RedGui::CRedGuiListItem* item = new RedGui::CRedGuiListItem( TXT(""), new SSkeletonInfo( skeleton ) );
					m_posesList->AddItem( item );
				}
			}
		}

		// Process
		{
			const Uint32 poseCount = m_posesList->GetItemCount();
			for ( Uint32 i=0; i<poseCount; ++i )
			{
				SSkeletonInfo* skelInfo = m_posesList->GetItem(i)->GetUserData< SSkeletonInfo >();
				CSkeleton* skeleton = skelInfo->m_skeleton.Get();
				if( skeleton != nullptr )
				{
			
					CPoseProvider* poseAlloc = skeleton->GetPoseProvider();
					if ( poseAlloc != nullptr )
					{
						poseAlloc->GetStats( skelInfo->m_stats );
	
						numTotal += skelInfo->m_stats.m_numTotal;
						numAlloc += skelInfo->m_stats.m_numAlloc - skelInfo->m_stats.m_numCached;
						numCached += skelInfo->m_stats.m_numCached;
	
						memTotal += skelInfo->m_stats.m_memTotal;
						memAlloc += skelInfo->m_stats.m_memAlloc - skelInfo->m_stats.m_memCached;
						memCached += skelInfo->m_stats.m_memCached;
	
						// add
						m_posesList->SetItemText( skelInfo->m_name.AsChar(), i, 0 );
						m_posesList->SetItemText( ToString( skelInfo->m_stats.m_numTotal ), i, 1 );
						m_posesList->SetItemText( String::Printf( TXT("%1.3f"), skelInfo->m_stats.m_memTotal * invMb ), i, 2 );
						m_posesList->SetItemText( ToString( skelInfo->m_stats.m_numCached ), i, 3 );
						m_posesList->SetItemText( String::Printf( TXT("%1.3f"), skelInfo->m_stats.m_memCached * invMb ), i, 4 );
						m_posesList->SetItemText( ToString( skelInfo->m_stats.m_numAlloc ), i, 5 );
						m_posesList->SetItemText( String::Printf( TXT("%1.3f"), skelInfo->m_stats.m_memAlloc * invMb ), i, 6 );
						m_posesList->SetItemText( ToString( skelInfo->m_stats.m_numFreeCached ), i, 7 );
						m_posesList->SetItemText( String::Printf( TXT("%1.3f"), skelInfo->m_stats.m_memFreeCached * invMb ), i, 8 );
						m_posesList->SetItemText( ToString( skelInfo->m_stats.m_numFreeAlloc ), i, 9 );
						m_posesList->SetItemText( String::Printf( TXT("%1.3f"), skelInfo->m_stats.m_memFreeAlloc * invMb ), i, 10 );
					}
				}
			}
		}

		// update labels
		const Uint32 poseCount = m_posesList->GetItemCount();
		m_allocators->SetText( String::Printf( TXT("Allocators: %d"), poseCount ) );
		m_totalPoses->SetText( String::Printf( TXT("Total poses: %1.3f MB (%d)"), invMb * memTotal, numTotal ) );
		m_cachedPoses->SetText( String::Printf( TXT("Cached poses: %1.3f MB (%d)"), invMb * memCached, numCached ) );
		m_allocPoses->SetText( String::Printf( TXT("Alloc poses: %1.3f MB (%d)"), invMb * memAlloc, numAlloc ) );

		// update
		m_totalPosesMemory = memTotal;
	}

	void CDebugWindowAnimations::UpdateComponentsTab()
	{
		CClass* actorClass = SRTTI::GetInstance().FindClass( CName( TXT("CActor") ) );
		CClass* itemClass = SRTTI::GetInstance().FindClass( CName( TXT("CItemEntity") ) );

		struct SDetails
		{
			Uint32 m_all;
			Uint32 m_frozen;
			Uint32 m_budgeted;

			SDetails() : m_all( 0 ), m_frozen( 0 ), m_budgeted( 0 ) {}
		};

		SDetails defaultDetails;

		THashMap< CSkeleton*,THashMap<CEntityTemplate*,SDetails> > restSkeletonMap;
		restSkeletonMap.Reserve( 1024 );
		THashMap<CEntityTemplate*,SDetails> brokenTemplateMap;
		brokenTemplateMap.Reserve( 1024 );
		TDynArray<String> brokenEntities;
		brokenEntities.Reserve( 512 );

		if( actorClass != nullptr && itemClass != nullptr )
		{
			if( GGame != nullptr && GGame->IsActive() == true )
			{
				CWorld* world = GGame->GetActiveWorld();
				if( world != nullptr )
				{
					// reset
					Red::System::MemorySet( m_components, 0, CT_Count*sizeof(SComponentInfo) );

					// get animated components
					TDynArray< CAnimatedComponent* > animatedComponents;
					world->GetAttachedComponentsOfClass( animatedComponents );

					m_components[ CT_All ].m_num = animatedComponents.Size();

					const CTickManager* tickMgr = world->GetTickManager();
					if( tickMgr != nullptr )
					{
						for ( Uint32 i=0; i<animatedComponents.Size(); ++i )
						{
							CAnimatedComponent* ac = animatedComponents[ i ];

							const Bool isSuppressed = ac->IsTickSuppressed();
							if ( isSuppressed )
							{
								m_components[ CT_All ].m_frozenNum++;
							}

							const Bool isBudgeted = ac->IsTickBudgeted();
							if ( isBudgeted )
							{
								m_components[ CT_All ].m_budgetedNum++;
							}

							// Actor
							{
								if ( actorClass && ac->GetEntity()->GetClass()->IsA( actorClass ) )
								{
									m_components[ CT_Actor ].m_num++;
									if ( isSuppressed ) m_components[ CT_Actor ].m_frozenNum++;
									if ( isBudgeted ) m_components[ CT_Actor ].m_budgetedNum++;

									if ( ac->IsInCinematic() )
									{
										m_components[ CT_Actor ].m_inCs++;
									}

									IActorInterface* actor = ac->GetEntity()->QueryActorInterface();
									if ( actor )
									{
										if ( actor->IsWorking() ) m_components[ CT_Actor ].m_inWork++;
										if ( actor->IsInQuestScene() ) m_components[ CT_Actor ].m_inBehContr++;
									}

									continue;
								}
							}

							// Static Camera
							{
								CStaticCamera* cam = Cast< CStaticCamera >( ac->GetEntity() );
								if ( cam )
								{
									m_components[ CT_StaticCamera ].m_num++;
									if ( isSuppressed ) m_components[ CT_StaticCamera ].m_frozenNum++;
									if ( isBudgeted ) m_components[ CT_StaticCamera ].m_budgetedNum++;

									if ( ac->IsInCinematic() )
									{
										m_components[ CT_StaticCamera ].m_inCs++;
									}

									continue;
								}
							}

							// Camera
							{
								CCamera* cam = Cast< CCamera >( ac->GetEntity() );
								if ( cam )
								{
									m_components[ CT_Camera ].m_num++;
									if ( isSuppressed ) m_components[ CT_Camera ].m_frozenNum++;
									if ( isBudgeted ) m_components[ CT_Camera ].m_budgetedNum++;

									if ( ac->IsInCinematic() )
									{
										m_components[ CT_Camera ].m_inCs++;
									}

									continue;
								}
							}

							// Item
							{
								if ( itemClass && ac->GetEntity()->GetClass()->IsA( itemClass ) )
								{
									m_components[ CT_Item ].m_num++;
									if ( isSuppressed ) m_components[ CT_Item ].m_frozenNum++;
									if ( isBudgeted ) m_components[ CT_Item ].m_budgetedNum++;

									if ( ac->IsInCinematic() )
									{
										m_components[ CT_Item ].m_inCs++;
									}

									IActorInterface* actor = ac->GetEntity()->QueryActorInterface();
									if ( actor )
									{
										if ( actor->IsWorking() ) m_components[ CT_Item ].m_inWork++;
										if ( actor->IsInQuestScene() ) m_components[ CT_Item ].m_inBehContr++;
									}

									continue;
								}
							}

							// Rest
							m_components[ CT_Rest ].m_num++;
							if ( isSuppressed ) m_components[ CT_Rest ].m_frozenNum++;
							if ( isBudgeted ) m_components[ CT_Rest ].m_budgetedNum++;

							if ( ac->IsInCinematic() )
							{
								m_components[ CT_Rest ].m_inCs++;
							}

							if( ac->GetSkeleton() )
							{
								THashMap<CEntityTemplate*,SDetails>& entities = restSkeletonMap.GetRef( ac->GetSkeleton() );
								SDetails& count = entities.GetRef( ac->GetEntity()->GetEntityTemplate(), defaultDetails );
								++count.m_all;
								count.m_frozen += isSuppressed ? 1 : 0;
								count.m_budgeted += isBudgeted ? 1 : 0;
							}
							else if( ac->GetEntity()->GetEntityTemplate() )
							{
								SDetails& count = brokenTemplateMap.GetRef( ac->GetEntity()->GetEntityTemplate(), defaultDetails );
								++count.m_all;
								count.m_frozen += isSuppressed ? 1 : 0;
								count.m_budgeted += isBudgeted ? 1 : 0;
							}
							else
							{
								brokenEntities.PushBack( ac->GetEntity()->GetName() );
							}


							IActorInterface* actor = ac->GetEntity()->QueryActorInterface();
							if ( actor )
							{
								if ( actor->IsWorking() ) m_components[ CT_Rest ].m_inWork++;
								if ( actor->IsInQuestScene() ) m_components[ CT_Rest ].m_inBehContr++;
							}
						}
					}
				}
			}
		}

		// update list
		for( Uint32 i=1; i<CT_Count; ++i )
		{
			m_componentsList->SetItemText( ToString( m_components[i].m_num - m_components[i].m_frozenNum ), i-1, 1 );
			m_componentsList->SetItemText( ToString( m_components[i].m_num ), i-1, 2 );
			m_componentsList->SetItemText( ToString( m_components[i].m_frozenNum ), i-1, 3 );
			m_componentsList->SetItemText( ToString( m_components[i].m_budgetedNum ), i-1, 4 );
			m_componentsList->SetItemText( ToString( m_components[i].m_inWork ), i-1, 5 );
			m_componentsList->SetItemText( ToString( m_components[i].m_inCs ), i-1, 6 );
			m_componentsList->SetItemText( ToString( m_components[i].m_inBehContr ), i-1, 7 );
		}

		struct SRestItem
		{
			Bool		m_isSkeleton;
			const void*	m_pointer;

			SRestItem( Bool skeleton, const void* p )
				: m_isSkeleton( skeleton )
				, m_pointer( p ) {}
		};

		TDynArray<Uint32> toRemove;
		for( Uint32 i = 0, size = m_restList->GetItemCount(); i < size; ++i )
		{
			RedGui::CRedGuiListItem* item = m_restList->GetItem( i );
			SRestItem* restItem = item->GetUserData<SRestItem>();
			if( restItem )
			{
				if( restItem->m_isSkeleton )
				{
					THashMap<CEntityTemplate*,SDetails>* templates = restSkeletonMap.FindPtr( (CSkeleton*)restItem->m_pointer );

					if( templates )
					{
						Uint32 count = 0;
						Uint32 frozen = 0;
						Uint32 budgeted = 0;
						if( (Int32)i == m_restList->GetSelection() )
						{
							TDynArray<Uint32> tToRemove;
							for( Uint32 j = 0, tsize = m_restTamplates->GetItemCount(); j < tsize; ++j )
							{
								RedGui::CRedGuiListItem* titem = m_restTamplates->GetItem( j );

								SDetails* tCount = templates->FindPtr( titem->GetUserData<CEntityTemplate>() );
								if( tCount )
								{
									templates->Erase( titem->GetUserData<CEntityTemplate>() );

									item->SetText( ToString( tCount->m_all ), 1 );
									item->SetText( ToString( tCount->m_frozen ), 2 );
									item->SetText( ToString( tCount->m_budgeted ), 3 );
									count += tCount->m_all;
									frozen += tCount->m_frozen;
									budgeted += tCount->m_budgeted;
								}
								else
								{
									tToRemove.PushBack( j );
								}
							}

							for( Int32 j = tToRemove.SizeInt() - 1; j >= 0; --j )
							{
								m_restTamplates->RemoveItem( tToRemove[j] );
							}

							for( auto it = templates->Begin(), end = templates->End(); it != end; ++it )
							{
								CEntityTemplate* templ = (*it).m_first;
								Uint32 index = m_restTamplates->AddItem( templ->GetFile()->GetFileName(), Color::WHITE, templ );
								m_restTamplates->SetItemText( ToString( (*it).m_second.m_all ), index, 1 );
								m_restTamplates->SetItemText( ToString( (*it).m_second.m_frozen ), index, 2 );
								m_restTamplates->SetItemText( ToString( (*it).m_second.m_budgeted ), index, 3 );
								count += (*it).m_second.m_all;
								frozen += (*it).m_second.m_frozen;
								budgeted += (*it).m_second.m_budgeted;
							}
						}
						else
						{
							for( auto it = templates->Begin(), end = templates->End(); it != end; ++it )
							{
								count += (*it).m_second.m_all;
								frozen += (*it).m_second.m_frozen;
								budgeted += (*it).m_second.m_budgeted;
							}
						}

						restSkeletonMap.Erase( (CSkeleton*)restItem->m_pointer );

						item->SetText( ToString( count ), 1 );
						item->SetText( ToString( frozen ), 2 );
						item->SetText( ToString( budgeted ), 3 );
					}
					else
					{
						if( (Int32)i == m_restList->GetSelection() )
						{
							m_restTamplates->RemoveAllItems();
						}

						delete restItem;
						toRemove.PushBack( i );
					}
				}
				else
				{
					SDetails* count = brokenTemplateMap.FindPtr( (CEntityTemplate*)restItem->m_pointer );

					if( count )
					{
						brokenTemplateMap.Erase( (CEntityTemplate*)restItem->m_pointer );

						item->SetText( ToString( count->m_all ), 1 );
						item->SetText( ToString( count->m_frozen ), 2 );
						item->SetText( ToString( count->m_budgeted ), 3 );
					}
					else
					{
						delete restItem;
						toRemove.PushBack( i );
					}

					if( (Int32)i == m_restList->GetSelection() )
					{
						m_restTamplates->RemoveAllItems();
					}
				}
			}
			else
			{
				if( !brokenEntities.RemoveFast( item->GetText() ) )
				{
					toRemove.PushBack( i );
				}

				if( (Int32)i == m_restList->GetSelection() )
				{
					m_restTamplates->RemoveAllItems();
				}
			}
		}

		for( Int32 i = toRemove.SizeInt() - 1; i >= 0; --i )
		{
			m_restList->RemoveItem( toRemove[i] );
		}

		for( auto it = restSkeletonMap.Begin(), end = restSkeletonMap.End(); it != end; ++it )
		{
			const CSkeleton* skeleton = (*it).m_first;
			if( skeleton->GetFile() )
			{
				Uint32 index = m_restList->AddItem( skeleton->GetFile()->GetFileName(), Color::WHITE, new SRestItem( true, skeleton ) );

				Uint32 count = 0;
				Uint32 frozen = 0;
				Uint32 budgeted = 0;
				for ( auto i = it->m_second.Begin(), end = it->m_second.End(); i != end; ++i )
				{
					count += i->m_second.m_all;
					frozen += i->m_second.m_frozen;
					budgeted += i->m_second.m_budgeted;
				}

				m_restList->SetItemText( ToString( count ), index, 1 );
				m_restList->SetItemText( ToString( frozen ), index, 2 );
				m_restList->SetItemText( ToString( budgeted ), index, 3 );
			}
		}

		for( auto it = brokenTemplateMap.Begin(), end = brokenTemplateMap.End(); it != end; ++it )
		{
			const CEntityTemplate* entTemplate = (*it).m_first;
			if( entTemplate->GetFile() )
			{
				Uint32 index = m_restList->AddItem( entTemplate->GetFile()->GetFileName(), Color::YELLOW, new SRestItem( false, entTemplate ) );
				m_restList->SetItemText( ToString( (*it).m_second.m_all ), index, 1 );
				m_restList->SetItemText( ToString( (*it).m_second.m_frozen ), index, 2 );
				m_restList->SetItemText( ToString( (*it).m_second.m_budgeted ), index, 3 );
			}
		}

		for( auto it = brokenEntities.Begin(), end = brokenEntities.End(); it != end; ++it )
		{
			m_restList->AddItem( *it, Color::RED );
		}

		// update labels
		m_animatedComponents->SetText( String::Printf( TXT("Animated Components: %d"), m_components[ CT_All ].m_num ) );
		m_activeComponents->SetText( String::Printf( TXT("Active: %d"), m_components[ CT_All ].m_num - m_components[ CT_All ].m_frozenNum ) );
		m_frozenComponents->SetText( String::Printf( TXT("Frozen: %d"), m_components[ CT_All ].m_frozenNum ) );
		m_budgetedComponents->SetText( String::Printf( TXT("Budgeted: %d"), m_components[ CT_All ].m_budgetedNum ) );
	}

	void CDebugWindowAnimations::UpdateAnimsetsTab()
	{
		const CClass* dialogsetClass = SRTTI::GetInstance().FindClass( CName( TXT("CStorySceneDialogset" ) ) );

		// reset 
		m_totalMemory = 0;
		m_totalAnimationsMemory = 0;
		m_totalAnimationsStreamedMemory = 0;
		m_wholeAnimationsMemory = 0;
		m_totalAnimationsMotionExMemory = 0;
		m_totalAnimationsCompressedFrameMemory = 0;
		m_totalAnimationsCompressedFrameDataMemory = 0;
		m_usedAnimationsMemory = 0;
		m_numActiveAnimations = 0;
		m_csSize = 0;
		m_exDialogSize = 0;

		// Unloaded animation set
		{
			Uint32 animCount = m_animSetList->GetItemCount();
			for( Uint32 i=0; i<animCount; ++i )
			{
				RedGui::CRedGuiListItem* item = m_animSetList->GetItem(i);
				if( item != nullptr )
				{
					SAnimSetInfo* animSetInfo = item->GetUserData< SAnimSetInfo >();
					if( animSetInfo != nullptr )
					{
						if( animSetInfo->m_set.Get() == nullptr )
						{
							m_animSetList->RemoveItem( i );

							if( i != 0 )
							{
								--i;
								--animCount;
							}
						}
					}
				}
			}
		}

		// Collect all animations sets
		{
			for ( ObjectIterator< CSkeletalAnimationSet > it; it; ++it )
			{
				CSkeletalAnimationSet* animSet = *it;
				SAnimSetInfo* info = nullptr;

				const Uint32 animCount = m_animSetList->GetItemCount();
				for( Uint32 j=0; j<animCount; ++j )
				{
					SAnimSetInfo* testAnimSetInfo = m_animSetList->GetItem( j )->GetUserData< SAnimSetInfo >();
					if( testAnimSetInfo != nullptr )
					{
						if( testAnimSetInfo->m_set.Get() == animSet )
						{
							info = testAnimSetInfo;
							break;
						}
					}
				}

				if( info == nullptr )
				{
					RedGui::CRedGuiListItem* item = new RedGui::CRedGuiListItem( TXT(""), new SAnimSetInfo( animSet ) );
					m_animSetList->AddItem( item );
				}
			}
		}

		// Process fade
		{
			const Uint32 animCount = m_animSetList->GetItemCount();
			for( Uint32 i=0; i<animCount; ++i )
			{
				SAnimSetInfo* animSetInfo = m_animSetList->GetItem( i )->GetUserData< SAnimSetInfo >();
				if( animSetInfo != nullptr )
				{
					CSkeletalAnimationSet* animSet = animSetInfo->m_set.Get();
					if( animSet != nullptr )
					{
						// More animations
						TDynArray< CSkeletalAnimationSetEntry* > animations;
						animSet->GetAnimations( animations );

						// Count active animations
						Uint32 numUsedAnimations = 0;
						Uint32 totalDataSize = 0;
						Uint32 totalAnimationDataSize = 0;
						Uint32 totalAnimationDataSizeStreamed = 0;
						Uint32 wholeAnimationDataSize = 0;
						Uint32 totalAnimationMotionExDataSize = 0;
						Uint32 totalAnimationsCompressedFrameMemory = 0;
						Uint32 totalAnimationsCompressedFrameDataMemory = 0;
						Uint32 usedAnimationDataSize = 0;

						for ( Uint32 k=0; k<animations.Size(); k++ )
						{
							CSkeletalAnimationSetEntry* entry = animations[k];
							if ( entry && entry->GetAnimation() )
							{
								// Count total animation size
								CSkeletalAnimation* anim = entry->GetAnimation();

								AnimMemStats stats;
								anim->GetMemStats( stats );

								const Uint32 animSize = stats.m_animBufferNonStreamable + stats.m_animBufferStreamableLoaded;
								totalDataSize += animSize + stats.m_compressedPose + stats.m_motionExtraction;
								totalAnimationDataSize += animSize;
								totalAnimationDataSizeStreamed += stats.m_animBufferStreamableLoaded;
								wholeAnimationDataSize += stats.m_animBufferNonStreamable + stats.m_animBufferStreamableWhole;
								totalAnimationsCompressedFrameMemory += stats.m_compressedPose;
								totalAnimationsCompressedFrameDataMemory += stats.m_compressedPoseData;
								totalAnimationMotionExDataSize += stats.m_motionExtraction;

								// Used ?
								if ( anim->GetLastTouchTime() == GEngine->GetCurrentEngineTick() )
								{
									usedAnimationDataSize += animSize;
									numUsedAnimations += 1;
								}
							}
						}

						// Update stats
						animSetInfo->m_totalUsedMemory = totalDataSize;
						animSetInfo->m_totalUsedMemoryForAnimation = totalAnimationDataSize;
						animSetInfo->m_totalUsedMemoryForAnimationStreamed = totalAnimationDataSizeStreamed;
						animSetInfo->m_wholeAnimationDataMemory = wholeAnimationDataSize;
						animSetInfo->m_totalUsedMemoryForMotionEx = totalAnimationMotionExDataSize;
						animSetInfo->m_activeUsedMemory = usedAnimationDataSize;
						animSetInfo->m_totalUsedMemoryForCompressedFrame = totalAnimationsCompressedFrameMemory;
						animSetInfo->m_totalUsedMemoryForCompressedFrameData = totalAnimationsCompressedFrameDataMemory;

						// Change count
						if ( numUsedAnimations > animSetInfo->m_numActiveAnimations ) 
						{
							animSetInfo->m_numActiveAnimations = numUsedAnimations;
							animSetInfo->m_highLightColor = Color::RED;
						}
						else if ( numUsedAnimations < animSetInfo->m_numActiveAnimations )
						{
							animSetInfo->m_numActiveAnimations = numUsedAnimations;
							animSetInfo->m_highLightColor = Color::GREEN;
						}

						if ( animSet->IsA< CCutsceneTemplate >() )
						{
							m_csSize += animSetInfo->m_totalUsedMemory;
						}
						else if ( animSet->GetClass() == dialogsetClass )
						{
							m_exDialogSize += animSetInfo->m_totalUsedMemory;
						}
					}

					m_totalMemory += animSetInfo->m_totalUsedMemory;
					m_totalAnimationsMemory += animSetInfo->m_totalUsedMemoryForAnimation;
					m_totalAnimationsStreamedMemory += animSetInfo->m_totalUsedMemoryForAnimationStreamed;
					m_wholeAnimationsMemory += animSetInfo->m_wholeAnimationDataMemory;
					m_totalAnimationsMotionExMemory += animSetInfo->m_totalUsedMemoryForMotionEx;
					m_totalAnimationsCompressedFrameMemory += animSetInfo->m_totalUsedMemoryForCompressedFrame;
					m_totalAnimationsCompressedFrameDataMemory += animSetInfo->m_totalUsedMemoryForCompressedFrameData;
					m_usedAnimationsMemory += animSetInfo->m_activeUsedMemory;
					m_numActiveAnimations += animSetInfo->m_numActiveAnimations;
				}	
			}
		}

		// update gui
		{
			const Uint32 animCount = m_animSetList->GetItemCount();
			for( Uint32 i=0; i<animCount; ++i )
			{
				SAnimSetInfo* animSetInfo = m_animSetList->GetItem( i )->GetUserData< SAnimSetInfo >();
				if( animSetInfo != nullptr )
				{
					m_animSetList->SetItemText( animSetInfo->m_name, i, 0 );
					m_animSetList->SetItemText( String::Printf( TXT("%1.3f"), animSetInfo->m_totalUsedMemory / ( 1024.0f*1024.0f ) ), i, 1 );
					m_animSetList->SetItemText( String::Printf( TXT("%1.3f"), animSetInfo->m_totalUsedMemoryForAnimationStreamed / ( 1024.0f*1024.0f ) ), i, 2 );
					m_animSetList->SetItemText( String::Printf( TXT("%1.3f"), (animSetInfo->m_totalUsedMemory - animSetInfo->m_totalUsedMemoryForAnimationStreamed) / ( 1024.0f*1024.0f ) ), i, 3 );
					m_animSetList->SetItemText( String::Printf( TXT("%1.3f"), animSetInfo->m_wholeAnimationDataMemory / ( 1024.0f*1024.0f ) ), i, 4 );
					m_animSetList->SetItemText( String::Printf( TXT("%1.3f"), animSetInfo->m_activeUsedMemory / ( 1024.0f*1024.0f ) ), i, 5 );
					m_animSetList->SetItemText( ToString( animSetInfo->m_numActiveAnimations ), i, 6 );
					m_animSetList->SetItemColor( i, animSetInfo->m_color );
				}
			}

			// update general label
			m_animsetCount->SetText( String::Printf( TXT("Animset count: %d"), animCount ) );
		}
	}

	void CDebugWindowAnimations::DumpAnimsetsInfo(RedGui::CRedGuiEventPackage& eventPackage)
	{
		if( m_animSetList != nullptr)
		{
			if( GGame != nullptr )
			{
				CWorld* world = GGame->GetActiveWorld();
				if( world != nullptr )
				{
					CResource::FactoryInfo< C2dArray > info;
					C2dArray* debugDumpInfo = info.CreateResource();
					String depoPath = GFileManager->GetDataDirectory();
					String worldPath = world->DepotPath();
					String outPath = depoPath + worldPath;
					outPath += TXT("_AnimsetsDump.csv");

					String outFile = String::EMPTY;
					String row = String::EMPTY;
					// first row with column names
					for ( Uint32 i=0; i< m_animSetList->GetColumnCount(); ++i )
					{
						row += m_animSetList->GetColumnLabel( i );
						row += TXT(";");
						if ( i == m_animSetList->GetColumnCount() - 1 )
						{
							row += TXT("\n");
						}
					}
					outFile += row;

					for (Uint32 i = 0; i < m_animSetList->GetItemCount(); ++i)
					{
						RedGui::CRedGuiListItem* item = m_animSetList->GetItem( i );
						row = String::EMPTY;
						for ( Uint32 j=0; j<m_animSetList->GetColumnCount(); ++j )
						{
							const String& col = item->GetText( j );
							row += col;
							row += TXT(";");
							if ( j == m_animSetList->GetColumnCount()-1 )
							{
								row += TXT("\n");
							}
						}
						outFile += row;
					}
					GFileManager->SaveStringToFile( outPath, outFile );
				}
			}
		}
	}

	void CDebugWindowAnimations::UpdateCutscenesTab()
	{
		// resources
		m_cutsceneResourcesList->RemoveAllItems();
		for ( ObjectIterator< CCutsceneTemplate > it; it; ++it )
		{
			m_cutsceneResourcesList->AddItem( (*it)->GetDepotPath() );
		}

		// instances
		if( GGame != nullptr )
		{
			TDynArray< String > csInstances;
			GGame->CollectCutsceneInstancesName( csInstances );

			m_cutsceneInstancesList->RemoveAllItems();
			for ( Uint32 i=0; i<csInstances.Size(); ++i )
			{
				m_cutsceneResourcesList->AddItem( csInstances[i] );
			}
		}
	}

	void CDebugWindowAnimations::CalcNumStreamingAndLoadedAnim( Uint32& numStr, Uint32& numLoaded, Float& loadedAnimationSize ) const
	{
		for ( AnimationIterator it( false ); it; ++it )
		{
			CSkeletalAnimation* anim = (*it);
			if ( anim )
			{
				if ( anim->IsLoaded() )
				{
					numLoaded++;
					loadedAnimationSize += anim->GetDataSize();
				}

				if ( anim->HasStreamingPending() )
				{
					numStr++;
					loadedAnimationSize += anim->GetDataSize();
				}
			}
		}
	}

	void CDebugWindowAnimations::UpdateProgressBar( RedGui::CRedGuiProgressBar* progress, Float value, Float range, const String& text, const String& unit )
	{
		progress->SetProgressPosition( value );
		progress->SetProgressRange( range );
		Color color = Lerp( (Float)( value/range ), Color( 0, 255, 0 ).ToVector(), Color( 255, 0, 0 ).ToVector() );
		progress->SetProgressBarColor( color );
		progress->SetProgressInformation( String::Printf( TXT("%s %1.3f / %1.3f %s"), text.AsChar(), value, range, unit.AsChar() ) );
	}

	void CDebugWindowAnimations::OnWindowOpened( CRedGuiControl* control )
	{
		// update all
		UpdatePosesTab();
		UpdateComponentsTab();
		UpdateAnimsetsTab();
		UpdateCutscenesTab();
	}

}	// namespace DebugWindows

#endif	//NO_DEBUG_WINDOWS
#endif	//NO_RED_GUI
