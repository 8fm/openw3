/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#if !defined( NO_DEBUG_PAGES ) && defined( ENABLE_EXTENDED_MEMORY_METRICS )

#include "../../common/engine/debugCommandBox.h"
#include "../../common/engine/debugPageParam.h"

#include "debugPageMemoryBudget.h"
#include "debugPageMemoryBudgetCommands.h"

#ifndef NO_DEBUG_WINDOWS
#include "../../common/engine/debugWindowsManager.h"
#include "../engine/debugPageManagerBase.h"
#include "../engine/dynamicLayer.h"
#include "../engine/renderFrame.h"
#endif

//////////////////////////////////////////////////////////////////////////

CDebugClassChoice::CDebugClassChoice( IDebugCheckBox* parent )
	: IDebugChoice( parent, TXT("Class list") )
{
	TDynArray< CClass* > classes;
	SRTTI::GetInstance().EnumClasses( CObject::GetStaticClass(), classes );

	for ( Uint32 i=0; i<classes.Size(); ++i )
	{
		if ( classes[ i ] == CNewNPC::GetStaticClass() )
		{
			m_index = i;
			m_selected = classes[ m_index ]->GetName().AsString();

			return;
		}
	}

	m_index = 0;
	m_selected = classes[ m_index ]->GetName().AsString();
}

void CDebugClassChoice::OnNext()
{
	TDynArray< CClass* > classes;
	SRTTI::GetInstance().EnumClasses( CObject::GetStaticClass(), classes );

	if ( m_index + 1 < classes.Size() )
	{
		m_index += 1;
	}
	else
	{
		m_index = 0;
	}

	m_selected = classes[ m_index ]->GetName().AsString();
}

void CDebugClassChoice::OnPrev()
{
	TDynArray< CClass* > classes;
	SRTTI::GetInstance().EnumClasses( CObject::GetStaticClass(), classes );

	if ( m_index - 1 > 0 )
	{
		m_index -= 1;
	}
	else
	{
		m_index = classes.Size() - 1;
	}

	m_selected = classes[ m_index ]->GetName().AsString();
}

String CDebugClassChoice::GetSelection() const
{
	return m_selected;
}

//////////////////////////////////////////////////////////////////////////

CDebugPageMemoryBudget::CDebugPageMemoryBudget()
	: IDebugPage( TXT("Memory") )
	, m_rightTree( NULL )
	, m_log( 50 )
	, m_sliderDelete( NULL )
	, m_sliderCompareOne( NULL )
	, m_sliderCompareTwo( NULL )
	, m_fileLog( TXT("memoryDebugPage.log") )
{

}

void CDebugPageMemoryBudget::OnPageShown()
{
	IDebugPage::OnPageShown();
}

void CDebugPageMemoryBudget::OnTick( Float timeDelta )
{
	IDebugPage::OnTick( timeDelta );

	TickAllocStats();

	if ( m_rightTree )
	{
		m_rightTree->OnTick( timeDelta );
	}
}

Bool CDebugPageMemoryBudget::OnViewportInput( IViewport* view, enum EInputKey key, enum EInputAction action, Float data )
{
#ifndef NO_RED_GUI
#ifndef NO_DEBUG_WINDOWS
	if( action == IACT_Press && key == IK_Enter )
	{
		GDebugWin::GetInstance().SetVisible( true );
	}
	return true;
#endif	// NO_DEBUG_WINDOWS
#endif	// NO_RED_GUI

	if ( key == IK_Pad_LeftThumb && action == IACT_Press )
	{
		if ( GGame->IsPaused() )
		{
			GGame->Unpause( TXT( "CDebugPageMemoryBudget" ) );
		}
		else
		{
			GGame->Pause( TXT( "CDebugPageMemoryBudget" ) );
		}

		return true;
	}

	if ( m_rightTree && m_rightTree->OnInput( key, action, data ) )
	{
		return true;
	}

	if ( m_log.OnInput( key, action ) )
	{
		return true;
	}

	// Not processed
	return false;
}

void CDebugPageMemoryBudget::OnViewportGenerateFragments( IViewport *view, CRenderFrame *frame )
{
#ifndef NO_DEBUG_WINDOWS
	String message = TXT("This debug page is destined to remove. Click key 'Enter' to open new debug framework.");

	frame->AddDebugRect(49, 80, 502, 20, Color( 100, 100, 100, 255 ) );
	frame->AddDebugScreenFormatedText( 60, 95, Color( 255, 255, 255, 255 ), TXT("Debug Message Box") );

	frame->AddDebugRect(50, 100, 500, 50, Color( 0, 0, 0, 255 ) );
	frame->AddDebugFrame(50, 100, 500, 50, Color( 100, 100, 100, 255 ) );

	frame->AddDebugScreenFormatedText( 70, 120, Color( 255, 0, 0, 255 ), message.AsChar() );

	frame->AddDebugScreenFormatedText( 275, 140, Color( 255, 255, 255 ), TXT(">> OK <<") );
	return;
#endif

	Uint32 xLeft = 55;
	Uint32 xRight = 605;

	Uint32 startY = 95;

	// Alloc stats
	{
		const Uint32 offset = 70;
		frame->AddDebugScreenFormatedText( 175,					80, TXT("Allocs: %u"),		m_allocs );
		frame->AddDebugScreenFormatedText( 175 + offset,		80, TXT("Deallocs: %u"),	m_deallocs );
		frame->AddDebugScreenFormatedText( 175 + 2 * offset,	80, TXT("Reallocs: %u"),	m_reallocs );
	}

	// Left side
	{
		Uint32 leftY = startY;

		// Main memory bar
		{
			RED_MESSAGE( "To do! Show total memory budget" );
		}

		// Most important memory stats
		{
			// Dynamic layer
			{
				Uint32 entitiesNum = 0;
				Uint32 layerDynamicData = 0;
				Uint32 layerAllData = 0;
				Uint32 layerDynamicObjects = 0;

				CWorld* world = GGame->GetActiveWorld();
				if ( world )
				{
					CDynamicLayer* layer = world->GetDynamicLayer();
					if ( layer )
					{
						entitiesNum = layer->GetEntities().Size();
						layerDynamicData += layer->CalcObjectDynamicDataSize();
						layerAllData += layerDynamicData;
						layerDynamicObjects = layer->CalcObjectsSize();
					}

					if ( world->GetWorldLayers() )
					{
						layerAllData += world->GetWorldLayers()->CalcObjectDynamicDataSize();
					}
				}

				const Float ds = layerDynamicData / 1024.f / 1024.f;
				const Float as = (layerAllData - layerDynamicData) / 1024.f / 1024.f;
				const Float dob = layerDynamicObjects / 1024.f / 1024.f;
				frame->AddDebugScreenFormatedText( xLeft, leftY, TXT("Layers data: %1.2f, Dynamic layer data: %1.2f, entities on dynamic layer: %1.2f, entities num: %d"), as, ds, dob, entitiesNum );

				leftY += 20;
			}

			// Meshes
			{
				//...
			}

			// Textures
			{
				//...
			}

			// Npcs
			{
				const Uint32 npcNum = GCommonGame->GetNPCCharacters().Size();
				frame->AddDebugScreenFormatedText( xLeft, leftY, TXT("Npcs: %d"), npcNum );

				leftY += 20;
			}
		}
	}
		
	// Right side
	{
		Uint32 rightY = startY;

		// Commands
		{
			const Uint32 height = 100;

			if ( !m_rightTree )
			{
				const Uint32 width = 500;
					
				m_rightTree = new CDebugOptionsTree( xRight, rightY, width, height, this );

				// Dumps
				{
					{
						IDebugCheckBox* group = new IDebugCheckBox( NULL, TXT("Dump to log"), true, false );
						m_rightTree->AddRoot( group );

						new CDebugDumpDynamicLayerCommandBox( group, &m_log );
						new CDebugDumpLayerStorageCommandBox( group, &m_log );
						new CDebugDumpLayersSizeCommandBox( group, &m_log );
						new CDebugDumpFileDepotPathStringsSizeCommandBox( group, &m_log );
						new CDebugDumpNodeNameStringsSizeCommandBox( group, &m_log );
#ifndef NO_LOG
						new CDebugDumpAllObjectListCommandBox( group, &m_log );
						new CDebugDumpRootsetObjectListCommandBox( group, &m_log );
						new CDebugDumpDefultObjectListCommandBox( group, &m_log );
#endif
						new CDebugDumpWorldArraysCommandBox( group, &m_log );
						new CDebugDumpNpcsCommandBox( group, &m_log );
						new CDebugDumpAnimEventsSizeByClassCommandBox( group, &m_log );
					}
					{
						IDebugCheckBox* group = new IDebugCheckBox( NULL, TXT("Dump to console log"), true, false );
						m_rightTree->AddRoot( group );

						new CDebugDumpDynamicLayerCommandBox( group, &m_consoleLog );
						new CDebugDumpLayerStorageCommandBox( group, &m_consoleLog );
						new CDebugDumpLayersSizeCommandBox( group, &m_consoleLog );
						new CDebugDumpFileDepotPathStringsSizeCommandBox( group, &m_consoleLog );
						new CDebugDumpNodeNameStringsSizeCommandBox( group, &m_consoleLog );
#ifndef NO_LOG
						new CDebugDumpAllObjectListCommandBox( group, &m_consoleLog );
						new CDebugDumpRootsetObjectListCommandBox( group, &m_consoleLog );
						new CDebugDumpDefultObjectListCommandBox( group, &m_consoleLog );
#endif
						new CDebugDumpWorldArraysCommandBox( group, &m_consoleLog );
						new CDebugDumpNpcsCommandBox( group, &m_consoleLog );
						new CDebugDumpAnimEventsSizeByClassCommandBox( group, &m_consoleLog );
					}
					{
						IDebugCheckBox* group = new IDebugCheckBox( NULL, TXT("Dump to file log"), true, false );
						m_rightTree->AddRoot( group );

						new CDebugDumpDynamicLayerCommandBox( group, &m_fileLog );
						new CDebugDumpLayerStorageCommandBox( group, &m_fileLog );
						new CDebugDumpLayersSizeCommandBox( group, &m_fileLog );
						new CDebugDumpFileDepotPathStringsSizeCommandBox( group, &m_fileLog );
						new CDebugDumpNodeNameStringsSizeCommandBox( group, &m_fileLog );
#ifndef NO_LOG
						new CDebugDumpAllObjectListCommandBox( group, &m_fileLog );
						new CDebugDumpRootsetObjectListCommandBox( group, &m_fileLog );
						new CDebugDumpDefultObjectListCommandBox( group, &m_fileLog );
#endif
						new CDebugDumpWorldArraysCommandBox( group, &m_fileLog );
						new CDebugDumpNpcsCommandBox( group, &m_fileLog );
						new CDebugDumpAnimEventsSizeByClassCommandBox( group, &m_fileLog );
					}
				}

				// Memory snapshots
				{
					IDebugCheckBox* group = new IDebugCheckBox( NULL, TXT("Memory snapshots"), true, false );
					m_rightTree->AddRoot( group );

					{
						new CDebugMemSnapshotCreateCommandBox( group, this );
					}

					new CDebugLineBox( group );

					{
						new CDebugMemSnapshotDeleteCommandBox( group, this );
						m_sliderDelete = new CDebugSliderIntWithValParam( group, TXT("Del"), 0, 1 );
					}

					new CDebugLineBox( group );

					{
						new CDebugMemSnapshotCompareCommandBox( group, this );
						m_sliderCompareOne = new CDebugSliderIntWithValParam( group, TXT("Sel One"), 0, 1 );
						m_sliderCompareTwo = new CDebugSliderIntWithValParam( group, TXT("Sel Two"), 0, 1 );
					}
				}

				// GC
				{
					IDebugCheckBox* group = new IDebugCheckBox( NULL, TXT("GC"), true, false );
					m_rightTree->AddRoot( group );

					TDynArray< String, MC_Debug > list;
					list.PushBack( TXT("CNewNPC") );
					list.PushBack( TXT("CActor") );
					list.PushBack( TXT("CEntity") );
					CDebugStringChoice* choice = new CDebugStringChoice( group, TXT("Class"), list );
					choice->SetSelection( TXT("CNewNPC") );
#ifndef RED_FINAL_BUILD
					new CDebugForceDebugCGCommandBox( group, choice );
#endif
					new CForceGCTimerCheckBox( group );
				}

				// Prog
				{
					//IDebugCheckBox* group = new IDebugCheckBox( NULL, TXT("Prog"), true, false );
					//m_rightTree->AddRoot( group );
				}
			}

			m_rightTree->OnRender( frame );

			rightY += height + 15;
		}

		// Log
		{
			const Uint32 height = 380;
			m_log.OnRender( xRight + 5, rightY, rightY + height, frame );

			rightY += height;
		}
	}
}

void CDebugPageMemoryBudget::OnMemSnapshotAdded()
{
	/* Debug Page are not used.
	// Create
	Uint32 index = static_cast< Uint32 >( m_snapshots.Grow( 1 ) );
	SMemorySnapshot& snapshot = m_snapshots[ index ];

	const Float invMb = 1.f / ( 1024.0f * 1024.0f );

	// Fill
	if ( GGame->GetActiveWorld() )
	{
		// Create and fill memory snapshot
		CWorld* world = GGame->GetActiveWorld();
		snapshot.Clear();

		// Memory
		Red::MemoryFramework::RuntimePoolMetrics allMetrics;
		SRedMemory::GetInstance().GetMetricsCollector().PopulateAllMetrics( allMetrics );
		snapshot.m_memoryStatus = allMetrics.m_totalBytesAllocated * invMb;

		// Memory classes
		{
			for ( Uint32 i=0; i<SMemorySnapshot::c_MaximumMemoryClasses; ++i )
			{
				Uint64 allocatedBytes = allMetrics.m_allocatedBytesPerMemoryClass[ i ];
				snapshot.m_sizes[ i ] = allocatedBytes * invMb;
			}
		}

		// Dynamic layer
		CDynamicLayer* layer = world->GetDynamicLayer();
		layer->CalcMemSnapshot( snapshot.m_dynamicLayer );

		// Layers
		CLayerGroup* group = world->GetWorldLayers();
		group->CalcMemSnapshot( snapshot.m_worldLayers );

		// World
		snapshot.m_worldSize = world->CalcObjectDynamicDataSize() * invMb;

		// Game
		snapshot.m_gameSize = GGame->CalcObjectDynamicDataSize() * invMb;

		// Objects
		snapshot.m_objectsState.DoSnapshot();
	}

	// Refresh
	RefreshMemSliders();

	snapshot.Print( &m_consoleLog, true );
	snapshot.Print( &m_fileLog, true );
	*/
}

void CDebugPageMemoryBudget::OnMemSnapshotDeleted()
{
	if ( m_sliderDelete )
	{
		Int32 num = (Int32)m_sliderDelete->GetValue();

		if ( num < m_snapshots.SizeInt() )
		{
			m_snapshots.RemoveAt( num );

			RefreshMemSliders();
		}
	}
}

void CDebugPageMemoryBudget::OnMemSnapshotCompare()
{
	if ( m_sliderCompareOne && m_sliderCompareTwo )
	{
		Int32 numOne = (Int32)m_sliderCompareOne->GetValue();
		Int32 numTwo = (Int32)m_sliderCompareTwo->GetValue();

		if ( numOne < m_snapshots.SizeInt() && numTwo < m_snapshots.SizeInt() )
		{
			SMemorySnapshot& s1 = m_snapshots[ numOne ];
			SMemorySnapshot& s2 = m_snapshots[ numTwo ];

			SMemorySnapshot::PrintDiff( s1, s2, &m_consoleLog );
			SMemorySnapshot::PrintDiff( s1, s2, &m_fileLog );
		}
	}
}

void CDebugPageMemoryBudget::RefreshMemSliders()
{
	Uint32 size = Max< Uint32 >( m_snapshots.SizeInt() - 1, 1 );

	m_sliderDelete->SetMax( size );
	m_sliderCompareOne->SetMax( size );
	m_sliderCompareTwo->SetMax( size );
}

void CDebugPageMemoryBudget::TickAllocStats()
{
	/* Debug Page are used.
	Red::MemoryFramework::RuntimePoolMetrics allMetrics;
	SRedMemory::GetInstance().GetMetricsCollector().PopulateAllMetrics( allMetrics );
	m_countedAllocs += static_cast< Uint32 >( allMetrics.m_allocationsPerFrame );
	m_countedDeallocs += static_cast< Uint32 >( allMetrics.m_bytesDeallocatedPerFrame );

	++m_frameCounter;

	if ( m_frameCounter == 15 )
	{
		m_frameCounter = 0;

		m_allocs = m_countedAllocs >> 4;
		m_deallocs = m_countedDeallocs >> 4;
		m_reallocs = m_countedReallocs >> 4;

		m_countedAllocs = 0;
		m_countedReallocs = 0;
		m_countedDeallocs = 0;
	}*/
}

void CreateDebugPageMemoryBudget()
{
	IDebugPage* page = new CDebugPageMemoryBudget();
	IDebugPageManagerBase::GetInstance()->RegisterDebugPage( page );
}

#endif