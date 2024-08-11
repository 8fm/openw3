/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#ifndef NO_RED_GUI
#ifndef NO_DEBUG_WINDOWS

#include "redGuiGroupBox.h"
#include "redGuiPanel.h"
#include "redGuiManager.h"
#include "redGuiTreeNode.h"
#include "redGuiTreeView.h"
#include "redGuiScrollPanel.h"
#include "redGuiTab.h"
#include "redGuiList.h"
#include "redGuiHistogram.h"
#include "debugWindowsManager.h"
#include "debugWindowCallstackProfiler.h"
#include "debugWindowGameFrame.h"

namespace DebugWindows
{
	CDebugWindowGameFrame::CDebugWindowGameFrame() 
		: RedGui::CRedGuiWindow(200,200,800,600)
		, m_restPercent( 0.0 )
		, m_restTime( 0.0 )
		, m_showTree( false )
		, m_timeForSearch( 0.f )
	{
		SetCaption(TXT("Game frame"));

		GRedGui::GetInstance().EventTick.Bind(this, &CDebugWindowGameFrame::NotifyEventTick);

		RedGui::CRedGuiGroupBox* graphView = new RedGui::CRedGuiGroupBox(0, 0, GetWidth(), 200);
		graphView->SetMargin(Box2(5, 5, 5, 5));
		graphView->SetText(TXT("Graph visualization"));
		AddChild(graphView);
		graphView->SetDock(RedGui::DOCK_Top);

		m_chart = new RedGui::CRedGuiHistogram(5, 5, (graphView->GetWidth() - 10), graphView->GetHeight());
		m_chart->SetDock(RedGui::DOCK_Fill);
		m_chart->SetCheckCorrectPercent(false);
		graphView->AddChild(m_chart);

		m_modes = new RedGui::CRedGuiTab(0, 0, 350, 300);
		m_modes->SetMargin(Box2(5, 5, 5, 5));
		m_modes->SetDock(RedGui::DOCK_Fill);
		AddChild(m_modes);
		m_modes->AddTab(TXT("General view"));
		m_modes->AddTab(TXT("Detailed view"));
		
		RedGui::CRedGuiScrollPanel* generalPanel = m_modes->GetTabAt(0);
		{
			m_generalList = new RedGui::CRedGuiList(0, 0, 600, 300);
			m_generalList->SetMargin(Box2(5, 5, 5, 5));
			generalPanel->AddChild(m_generalList);
			m_generalList->SetDock(RedGui::DOCK_Fill);
			m_generalList->AppendColumn( TXT("NAME"), 200 );
			m_generalList->AppendColumn( TXT("TIME"), 100 );
			m_generalList->AppendColumn( TXT("PERCENT"), 100 );
			m_generalList->AppendColumn( TXT("TIME BUDGET"), 100 );
			m_generalList->AppendColumn( TXT("REST FROM BUDGET"), 150 );
			m_generalList->SetSelectionMode( RedGui::SM_None );
		}

		RedGui::CRedGuiScrollPanel* detailedPanel = m_modes->GetTabAt(1);
		{
			m_functionsTree = new RedGui::CRedGuiTreeView(0, 0, 600, 300);
			detailedPanel->AddChild(m_functionsTree);
			m_functionsTree->SetDock(RedGui::DOCK_Fill);
			m_functionsTree->SetHeaderVisible( false );
			m_functionsTree->EventDoubleClickedNode.Bind( this, &CDebugWindowGameFrame::NotifyEventNodeDoubleClicked);
		}

		CollectCouters();

		// set default tab
		m_modes->SetActiveTab( 0 );
	}

	CDebugWindowGameFrame::~CDebugWindowGameFrame()
	{
		GRedGui::GetInstance().EventTick.Unbind(this, &CDebugWindowGameFrame::NotifyEventTick);
	}

	void CDebugWindowGameFrame::CollectCouters()
	{
		m_allBudget.AddPerf( TXT("BaseEngineTick") );
		m_elems.Resize( FBE_Last );

		{
			SFrameBudgetElem& elem = m_elems[ FBE_Engine ];
			elem.SetTimeBudget( 1.f );
			elem.AddPerf( TXT("SoftHandleProcess") );
			elem.AddPerf( TXT("TaskManagerTick") );
			elem.AddPerf( TXT("ProcessPendingEvents") );
			elem.AddPerf( TXT("ContentManagerUpdate") );
			elem.AddPerf( TXT("ProcessInput") );
			elem.AddPerf( TXT("DebugPageManager") );
			elem.AddPerf( TXT("SoftHandleProcessor") );
			elem.AddPerf( TXT("ResourceUnloaderTick") );
			elem.AddPerf( TXT("GCTick") );
			elem.AddPerf( TXT("MarkerSystemsManager") );
			elem.AddPerf( TXT("Telemetry") );
			elem.AddPerf( TXT("SecondScreen") );
			elem.AddPerf( TXT("CreateEntityManager") );
			elem.AddPerf( TXT("ProcessScheduledLayerVisibilityChanges") );
			elem.AddPerf( TXT("UpdateInputManager") );
			elem.AddPerf( TXT("UpdateTimeManager") );
			elem.AddPerf( TXT("PhysicsDebuggerTick") );
			elem.AddPerf( TXT("ProcessCameraMovement") );
			elem.AddPerf( TXT("TickManagerAdvanceTime") );
			elem.AddPerf( TXT("UpdateLoadingState") );
			elem.AddPerf( TXT("LodSystem") );
			elem.AddPerf( TXT("RebuildDirtyAreas") );
			elem.AddPerf( TXT("CameraDirectorUpdate") );
			elem.AddPerf( TXT("EntityMotionManager") );
			elem.AddPerf( TXT("TriggerManagerUpdate") );
			elem.AddPerf( TXT("TagManagerUpdate") );
			elem.AddPerf( TXT("DelayedStuff") );
			elem.AddPerf( TXT("WorldStreamingTick") );
			elem.AddPerf( TXT("TerrainClipMapUpdate") );
			elem.AddPerf( TXT("FoliageManagerTick") );
			elem.AddPerf( TXT("GlobalWaterUpdate") );
			elem.AddPerf( TXT("DynamicCollisionCollector_Tick") );
			elem.AddPerf( TXT("DebugConsoleUpdate") );
			elem.AddPerf( TXT("TickManager_CollectJobs") );
		}

		{
			SFrameBudgetElem& elem = m_elems[ FBE_Gameplay ];
			elem.SetTimeBudget( 3.f );
			elem.AddPerf( TXT("R4GameCallTick") );
			elem.AddPerf( TXT("PropertyAnimationsTick") );
			elem.AddPerf( TXT("TickEffects") );
			elem.AddPerf( TXT("GameSystemsTick") );
			elem.AddPerf( TXT("TickBehTreeMachines") );
			elem.AddPerf( TXT("ReactionsManagerTick") );
			elem.AddPerf( TXT("BehTreeReactionsManagerTick") );
			elem.AddPerf( TXT("ActorsManagerUpdate") );
			elem.AddPerf( TXT("EntityPoolUpdate") );
			elem.AddPerf( TXT("NpcCameraCollision") );
			elem.AddPerf( TXT("ItemManagerTick") );
			elem.AddPerf( TXT("EntitiesTick") );
			elem.AddPerf( TXT("ActionAreaComponent_OnTickPostPhysics") );
			elem.AddPerf( TXT("ExtEventProcess") );
		}

		{
			SFrameBudgetElem& elem = m_elems[ FBE_Scripts ];
			elem.SetTimeBudget( 5.f );
			elem.AddPerf( TXT("AdvanceThreads") );
			elem.AddPerf( TXT("FireTimers") );
			elem.AddPerf( TXT("EntityEventProcess") );
			elem.AddPerf( TXT("ACProcessScriptNotifications") );
		}

		{
			SFrameBudgetElem& elem = m_elems[ FBE_Animation ];
			elem.SetTimeBudget( 5.f );
			elem.AddPerf( TXT("AnimationManager_Pre") );
			elem.AddPerf( TXT("AnimationManager_Post") );
			elem.AddPerf( TXT("PostPoseConstraintsSyncPart") );
			elem.AddPerf( TXT("ACUpdateAndSampleBehaviorMSP") );
			elem.AddPerf( TXT("ACUpdateAttachedComp2") );
			elem.AddPerf( TXT("UpdateAndSampleAnimationJobImmediate") );
			elem.AddPerf( TXT("UpdatePoseConstraintsJobImmediate") );
		}

		{
			SFrameBudgetElem& elem = m_elems[ FBE_Movement ];
			elem.SetTimeBudget( 3.f );
			elem.AddPerf( TXT("FinalizeMovement") );
			elem.AddPerf( TXT("PathLibTick") );
			elem.AddPerf( TXT("PathPlanerTick") );
			elem.AddPerf( TXT("FinalizeMovement_PreSeparation") );
			elem.AddPerf( TXT("FinalizeMovement_PostSeparation") );
			elem.AddPerf( TXT("MAC_UpdateSyncMovement") );
			elem.AddPerf( TXT("MAC_UpdateParallelMovement") );
		}

		{
			SFrameBudgetElem& elem = m_elems[ FBE_Physics ];
			elem.SetTimeBudget( 1.5f );
			elem.AddPerf( TXT("PhysicsBatchQueryManagerTick") );
			elem.AddPerf( TXT("Physics scene fetch") );
			elem.AddPerf( TXT("Physics scene cloth fetch") );
			elem.AddPerf( TXT("PhysicsStartNextFrameSim_Main") );
			elem.AddPerf( TXT("PhysicsSetWhileSceneProcessFlag_Main") );
			elem.AddPerf( TXT("PhysicsStartNextFrameSim_Secondary") );
			elem.AddPerf( TXT("ClothComponent_OnTickPrePhysicsPost") );
			elem.AddPerf( TXT("FurComponent_OnTickPrePhysicsPost") );
			elem.AddPerf( TXT("DestructionSystemComponent_OnTickPostPhysics") );
			elem.AddPerf( TXT("AnimatePrePhysicsRagdoll") );
			elem.AddPerf( TXT("ACUpdateAndSampleSkeletonRagdoll2") );
		}

		{
			SFrameBudgetElem& elem = m_elems[ FBE_UpdateTransform ];
			elem.SetTimeBudget( 7.f );
			elem.AddPerf( TXT("UpdateTransforms_Pre") );
			elem.AddPerf( TXT("UpdateTransforms_Post") );
			elem.AddPerf( TXT("UpdateTransforms_PostUpdate") );
		}

		{
			SFrameBudgetElem& elem = m_elems[ FBE_Sounds ];
			elem.SetTimeBudget( 2.5f );
			elem.AddPerf( TXT("SoundTick") );
			elem.AddPerf( TXT("SoundSystemListener") );
			elem.AddPerf( TXT("UpdateSoundListener") );
			elem.AddPerf( TXT("SoundEmitterComponent_OnTickPostUpdateTransform") );
			elem.AddPerf( TXT("SoundAmbientAreaComponent_OnTickPostUpdateTransform") );
		}

		{
			SFrameBudgetElem& elem = m_elems[ FBE_Renderer ];
			elem.SetTimeBudget( 3.f );
			elem.AddPerf( TXT("RenderTick") );
			elem.AddPerf( TXT("GenerateFrame") );
			elem.AddPerf( TXT("WorldRender") );
			elem.AddPerf( TXT("UmbraSceneTick") );
			elem.AddPerf( TXT("EnvironmentTick") );
		}

		{
			SFrameBudgetElem& elem = m_elems[ FBE_Hud ];
			elem.SetTimeBudget( 1.f );
			elem.AddPerf( TXT("VideoPlayerTick") );
			elem.AddPerf( TXT("GUIManagerTick") );
			elem.AddPerf( TXT("FlashPlayerTick") );
		}

		{
			SFrameBudgetElem& elem = m_elems[ FBE_RenderFence ];
			elem.SetTimeBudget( 0.5f );
			elem.AddPerf( TXT("RenderFence") );
		}

		{
			SFrameBudgetElem& elem = m_elems[ FBE_Editor ];
			elem.SetTimeBudget( 1.f );
			elem.AddPerf( TXT("RedGui") );
			elem.AddPerf( TXT("GenerateDebugFragments") );
		}

		// create chart
		{
			for(Uint32 i=0; i<m_elems.Size(); ++i)
			{
				m_chart->AddData(GetName(i), GetColor(i), 0.0f);
	
				Color color = GetColor( i );
				String text = GetName( i );
				m_generalList->AddItem( text, color );
			}
			// add rest of frame time
			m_chart->AddData(TXT("Rest of frame time"), Color::RED, 0.0f);
			m_generalList->AddItem( TXT("Rest of frame time"), Color::RED );
		}

		// create tree with detailed info
		{
			for(Uint32 i=0; i<m_elems.Size(); ++i)
			{
				RedGui::CRedGuiTreeNode* parentNode = m_functionsTree->AddRootNode( GetName( i ) );
				{
					const TDynArray< SFrameBudgetElem::SPerfCounterStatArray >& arrayAdd = m_elems[i].GetCountersAdd();
					for(Uint32 j=0; j<arrayAdd.Size(); ++j)
					{
						RedGui::CRedGuiTreeNode* child = parentNode->AddNode( arrayAdd[j].m_name );
						child->SetUserString( TXT("KeyName"), arrayAdd[j].m_name );
					}
				}
			}
		}
	}

	void CDebugWindowGameFrame::NotifyEventTick( RedGui::CRedGuiEventPackage& eventPackage, Float timeDelta )
	{
		if( GetVisible() == true )
		{
			m_timeForSearch -= timeDelta;
	
			if ( m_timeForSearch < 0.f )
			{
				CollectPerfs();
				m_timeForSearch = 10.f;
			}
	
			m_allBudget.Update();
	
			Float totalTime = Max( m_allBudget.GetAverageTime(), 0.0001f );
			Float accTime = 0.0;
	
			const Uint32 size = m_elems.Size();
			for ( Uint32 i=0; i<size; ++i )
			{
				m_elems[ i ].UpdateAndCalc( totalTime, accTime );
			}
	
			m_restTime = totalTime - accTime;
			m_restPercent = m_restTime / totalTime;
	
			// Update chart data and general panel
			{
				for(Uint32 i=0; i<m_elems.Size(); ++i)
				{
					Float percent = m_elems[i].GetPercent();
					m_chart->UpdateDate(GetName(i), percent);

					// update bottom general panel
					Float time = m_elems[ i ].GetAverageTime();
					Float timeBudget = m_elems[ i ].GetTimeBudget();

					// update rows on the list
					m_generalList->SetItemColor( i, GetColor(i) );
					m_generalList->SetItemText( GetName(i), i, 0 );
					m_generalList->SetItemText( String::Printf( TXT("%1.3f ms"), time ), i, 1 );
					m_generalList->SetItemText( String::Printf( TXT("%1.1f"), 100.0 * percent ), i, 2 );
					m_generalList->SetItemText( String::Printf( TXT("%1.1f"), timeBudget ), i, 3 );
					m_generalList->SetItemText( String::Printf( TXT("%1.1f"), timeBudget - time ), i, 4 );
				}
				m_chart->UpdateDate( TXT("Rest of frame time"), m_restPercent );

				// update last row
				m_generalList->SetItemColor( FBE_Last, Color::RED );
				m_generalList->SetItemText( TXT("Rest of frame time"), FBE_Last, 0 );
				m_generalList->SetItemText( String::Printf( TXT("%1.3f ms"), m_restTime ), FBE_Last, 1 );
				m_generalList->SetItemText( String::Printf( TXT("%1.1f"), 100.0 * m_restPercent ), FBE_Last, 2 );
			}

			// Update data in the tree
			{
				Uint64 freq;
				Red::System::Clock::GetInstance().GetTimer().GetFrequency( freq );
				Float m_freq = (Float)(freq/1000.0);

				if( m_modes->GetActiveTabIndex() == 1 )
				{
					TDynArray< RedGui::CRedGuiTreeNode* > rootNodes = m_functionsTree->GetRootNodes();

					for(Uint32 i=0; i<rootNodes.Size(); ++i)
					{
						if( rootNodes[i]->GetExpanded() == true )
						{
							const TDynArray< SFrameBudgetElem::SPerfCounterStatArray >& arrayAdd = m_elems[ i ].GetCountersAdd();
							TDynArray< RedGui::CRedGuiTreeNode* > nodeChildren = rootNodes[i]->GetChildrenNodes();

							for(Uint32 j=0; j<nodeChildren.Size(); ++j)
							{
								const SFrameBudgetElem::SPerfCounterStatArray& statChild = arrayAdd[j];

								String str = String::Printf( TXT("  %1.2f (%u)  "), statChild.GetAverageTime( m_freq ), statChild.CounterSize() );
								nodeChildren[j]->SetText( arrayAdd[j].m_name + TXT("     ") + str);
							}
						}
					}
				}
			}
		}
	}

	Color CDebugWindowGameFrame::GetColor( Uint32 i )
	{
		switch( i )
		{
		case FBE_Engine:
			return Color( 0, 200, 200 );
		case FBE_Gameplay:
			return Color( 200, 200, 200 );
		case FBE_Scripts:
			return Color( 200, 64, 200 );
		case FBE_Animation:
			return Color( 128, 255, 128 );
		case FBE_Movement:
			return Color( 255, 255, 128 );
		case FBE_Physics:
			return Color( 128, 255, 255 );
		case FBE_UpdateTransform:
			return Color( 255, 128, 255 );	
		case FBE_Sounds:
			return Color( 0, 128, 255 );
		case FBE_Renderer:
			return Color( 180, 100, 120 );
		case FBE_Hud:
			return Color( 128, 128, 255 );
		case FBE_RenderFence:
			return Color( 100, 100, 100 );
		case FBE_Editor:
			return Color( 0, 0, 100 );
		default:
			return Color( 255, 128, 128 );
		};
	}

	String CDebugWindowGameFrame::GetName( Uint32 i )
	{
		switch( i )
		{
		case FBE_Engine:
			return TXT("Engine");
		case FBE_Gameplay:
			return TXT("Gameplay");
		case FBE_Scripts:
			return TXT("Scripts");
		case FBE_Animation:
			return TXT("Animation");
		case FBE_Movement:
			return TXT("Movement");
		case FBE_Physics:
			return TXT("Physics");
		case FBE_UpdateTransform:
			return TXT("UpdateTransform");
		case FBE_Sounds:
			return TXT("Sound");
		case FBE_Renderer:
			return TXT("Renderer");
		case FBE_Hud:
			return TXT("Hud");
		case FBE_RenderFence:
			return TXT("RenderFence");
		case FBE_Editor:
			return TXT("Editor");
		default:
			return TXT("Error");
		};
	}

	void CDebugWindowGameFrame::CollectPerfs()
	{
		m_allBudget.CollectPerfs();

		const Uint32 size = m_elems.Size();
		for ( Uint32 i=0; i<size; ++i )
		{
			m_elems[ i ].CollectPerfs();
		}
	}

	void CDebugWindowGameFrame::NotifyEventNodeDoubleClicked( RedGui::CRedGuiEventPackage& eventPackage, RedGui::CRedGuiTreeNode* node )
	{
		RED_UNUSED( eventPackage );

		CDebugWindowCallstackProfiler* win = GDebugWin::GetInstance().GetDebugWindow< CDebugWindowCallstackProfiler >( DW_Counters );
		win->SetVisible( true );
		win->SelectCounter( node->GetUserString( TXT("KeyName") ) );
		win->UpLayerItem();
		win->MoveToTop();
	}

}	// namespace DebugWindows

#endif	// NO_DEBUG_WINDOWS
#endif	// NO_RED_GUI
