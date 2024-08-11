
#pragma once

#ifndef NO_DEBUG_PAGES

#include "../../common/core/objectIterator.h"

#include "../../common/engine/debugCommandBox.h"
#include "../../common/engine/debugPageParam.h"
#include "../core/garbageCollector.h"

#include "objectsSnapshot.h"
#include "gameWorld.h"
#include "../engine/layerGroup.h"
#include "../engine/dynamicLayer.h"
#include "../engine/renderFrame.h"
#include "../core/set.h"

class CDebugPageMemoryBudget;

//////////////////////////////////////////////////////////////////////////

class CDebugPageLogInterface
{
public:
	virtual void Log( const String& str ) = 0;
};

class CDebugPageConsoleLog : public CDebugPageLogInterface
{
public:
	virtual void Log( const String& str )
	{
		LOG_GAME( TXT("%s"), str.AsChar() );
	}
};

class CDebugPageFileLog : public CDebugPageLogInterface
{
	String	m_path;
	Bool	m_clearFile;

public:
	CDebugPageFileLog( const String& name )
		: m_clearFile( false )
	{
		m_path = name;
	}

	virtual void Log( const String& str )
	{
		if ( !m_clearFile )
		{
			ClearFile();
			m_clearFile = true;
		}

		RED_MESSAGE("FIXME>>>>>")
#ifndef RED_PLATFORM_ORBIS
		FILE *f = _wfopen( m_path.AsChar(), TXT("a") );
#else
		FILE *f = fopen( UNICODE_TO_ANSI(m_path.AsChar() ), "a" );
#endif

		fwprintf( f, TXT("%s\n"), str.AsChar() );

		fclose( f );
	}

private:
	void ClearFile()
	{
#ifndef RED_PLATFORM_ORBIS
		FILE *f = _wfopen( m_path.AsChar(), TXT("w") );
#else
		FILE *f = fopen( UNICODE_TO_ANSI( m_path.AsChar() ), "w" );
#endif
		fclose( f );
	}
};

class CDebugPageLog : public CDebugPageLogInterface
{
	TDynArray< String, MC_Debug > m_lines;

	Int32							m_index;
	Int32							m_max;

	Int32							m_curr;
	Int32							m_linesNum;

public:
	CDebugPageLog( Uint32 max )
		: m_max( max )
		, m_index( 0 )
		, m_curr( 0 )
		, m_linesNum( 0 )
	{
		m_lines.Resize( m_max );
	}

	virtual void Log( const String& str )
	{
		m_lines[ m_index ] = str;

		m_index++;

		if ( m_index >= m_max )
		{
			m_index = 0;
		}

		m_curr = m_index;
	}

	void OnRender( Uint32 x, Uint32 y, Uint32 maxY, CRenderFrame *frame )
	{
		Color color( 200, 200, 200 );

		if ( m_linesNum == 0 )
		{
			m_linesNum = ( maxY - y ) / 15;
		}

		frame->AddDebugFrame( x - 5, y - 5, 500, maxY - y + 5, Color::WHITE );

		Uint32 start = Max( m_curr, m_linesNum );

		for ( Int32 i=0; i<m_linesNum; ++i )
		{
			Int32 it = start - m_linesNum + i;

			if ( it > m_max )
			{
				break;
			}

			frame->AddDebugScreenText( x, y, m_lines[ it ].AsChar(), color );
			y += 15;
		}
	}

	Bool OnInput( enum EInputKey key, enum EInputAction action )
	{
		if ( key == IK_Pad_LeftShoulder && action == IACT_Press )
		{
			m_curr = Clamp< Int32 >( m_curr - 1, 0, m_max );
			return true;
		}
		else if ( key == IK_Pad_RightShoulder && action == IACT_Press )
		{
			m_curr = Clamp< Int32 >( m_curr + 1, 0, m_max );
			return true;
		}

		return false;
	};
};

//////////////////////////////////////////////////////////////////////////

struct SMemorySnapshot
{
public:
	SLayerMemorySnapshot		m_dynamicLayer;
	SLayerGroupMemorySnapshot	m_worldLayers;

	ObjectsByClassesSnapshot	m_objectsState;

	static const Uint32 c_MaximumMemoryClasses = Red::MemoryFramework::k_MaximumMemoryClasses;

	Float		m_sizes[ c_MaximumMemoryClasses ];

	Float		m_memoryStatus;
	Float		m_worldSize;
	Float		m_gameSize;

	void Clear()
	{
		for ( Uint32 i=0; i<c_MaximumMemoryClasses; ++i )
		{
			m_sizes[ i ] = 0.f;
		}

		m_dynamicLayer = SLayerMemorySnapshot();
		m_worldLayers = SLayerGroupMemorySnapshot();

		m_memoryStatus = 0;
		m_gameSize = 0;
		m_worldSize = 0;

		m_objectsState = ObjectsByClassesSnapshot();
	}

	void Print( CDebugPageLogInterface* log, Bool cols )
	{
		log->Log( TXT("Memory snapshot") );
		log->Log( String::Printf( TXT("   >Curr memory: %1.2f"), m_memoryStatus ) );
		log->Log( String::Printf( TXT("   >Game: %1.2f"), m_gameSize ) );
		log->Log( String::Printf( TXT("   >World: %1.2f"), m_worldSize ) );

		Uint32 memClassesIt = 0;
		while ( memClassesIt < c_MaximumMemoryClasses )
		{
			Float size1 = memClassesIt + 0 < c_MaximumMemoryClasses ? m_sizes[ memClassesIt+0 ] : 0.f;
			Float size2 = memClassesIt + 1 < c_MaximumMemoryClasses ? m_sizes[ memClassesIt+1 ] : 0.f;
			Float size3 = memClassesIt + 2 < c_MaximumMemoryClasses ? m_sizes[ memClassesIt+2 ] : 0.f;

			AnsiChar name1[64] = {'\0'}; 
			Memory::GetMemoryClassName( memClassesIt+0, name1, ARRAY_COUNT( name1 ) );
			AnsiChar name2[64] = {'\0'}; 
			Memory::GetMemoryClassName( memClassesIt+1, name2, ARRAY_COUNT( name2 ) );
			AnsiChar name3[64] = {'\0'}; 
			Memory::GetMemoryClassName( memClassesIt+2, name3, ARRAY_COUNT( name3 ) );
			if ( cols )
			{
				log->Log( String::Printf( TXT("%30" ) RED_PRIWas TXT( " %6.2f;  %30" ) RED_PRIWas TXT( " %6.2f;  %30" ) RED_PRIWas TXT( " %6.2f"), name1, size1, name2, size2, name3, size3 ) );
			}
			else
			{
				log->Log( String::Printf( TXT("%30" ) RED_PRIWas TXT( " %6.2f;"), name1, size1 ) );
				log->Log( String::Printf( TXT("%30" ) RED_PRIWas TXT( " %6.2f;"), name2, size2 ) );
				log->Log( String::Printf( TXT("%30" ) RED_PRIWas TXT( " %6.2f;"), name3, size3 ) );
			}

			memClassesIt += 3;
		}

		RPrint( m_dynamicLayer, 0, log );
		RPrint( m_worldLayers, 0, 0, log );
	}

	static void PrintDiff( const SMemorySnapshot& a, const SMemorySnapshot& b, CDebugPageLogInterface* log )
	{
		log->Log( TXT("") );
		log->Log( TXT("") );

		log->Log( TXT("Memory snapshot diff") );

		log->Log( String::Printf( TXT("   >Curr memory: %1.2f | %1.2f | Diff %1.2f"), b.m_memoryStatus, a.m_memoryStatus, b.m_memoryStatus - a.m_memoryStatus ) );
		log->Log( String::Printf( TXT("   >Game: %1.2f | %1.2f | Diff %1.2f"), b.m_gameSize, a.m_gameSize, b.m_gameSize - a.m_gameSize ) );
		log->Log( String::Printf( TXT("   >World: %1.2f | %1.2f | Diff %1.2f"), b.m_worldSize, a.m_worldSize, b.m_worldSize - a.m_worldSize ) );

		log->Log( String::Printf( TXT("   >Dynamic layer" ) ) );
		log->Log( String::Printf( TXT("      >Static   : %1.3f | %1.3f | Diff %1.3f" ), b.m_dynamicLayer.m_memStatic, a.m_dynamicLayer.m_memStatic, b.m_dynamicLayer.m_memStatic - a.m_dynamicLayer.m_memStatic ) );
		log->Log( String::Printf( TXT("      >Serial   : %1.3f | %1.3f | Diff %1.3f" ), b.m_dynamicLayer.m_memSerialize, a.m_dynamicLayer.m_memSerialize, b.m_dynamicLayer.m_memSerialize - a.m_dynamicLayer.m_memSerialize ) );
		log->Log( String::Printf( TXT("      >Dynamic  : %1.3f | %1.3f | Diff %1.3f" ), b.m_dynamicLayer.m_memDynamic, a.m_dynamicLayer.m_memDynamic, b.m_dynamicLayer.m_memDynamic - a.m_dynamicLayer.m_memDynamic ) );
		log->Log( String::Printf( TXT("      >Entities : %d | %d | Diff %d" ), b.m_dynamicLayer.m_entitiesNum, a.m_dynamicLayer.m_entitiesNum, b.m_dynamicLayer.m_entitiesNum - a.m_dynamicLayer.m_entitiesNum ) );
		log->Log( String::Printf( TXT("      >Component: %d | %d | Diff %d" ), b.m_dynamicLayer.m_attachedCompNum, a.m_dynamicLayer.m_attachedCompNum, b.m_dynamicLayer.m_attachedCompNum - a.m_dynamicLayer.m_attachedCompNum ) );

		log->Log( String::Printf( TXT("   >World layers" ) ) );
		log->Log( String::Printf( TXT("      >Static   : %1.3f | %1.3f | Diff %1.3f" ), b.m_worldLayers.m_memStatic, a.m_worldLayers.m_memStatic, b.m_worldLayers.m_memStatic - a.m_worldLayers.m_memStatic ) );
		log->Log( String::Printf( TXT("      >Serial   : %1.3f | %1.3f | Diff %1.3f" ), b.m_worldLayers.m_memSerialize, a.m_worldLayers.m_memSerialize, b.m_worldLayers.m_memSerialize - a.m_worldLayers.m_memSerialize ) );
		log->Log( String::Printf( TXT("      >Dynamic  : %1.3f | %1.3f | Diff %1.3f" ), b.m_worldLayers.m_memDynamic, a.m_worldLayers.m_memDynamic, b.m_worldLayers.m_memDynamic - a.m_worldLayers.m_memDynamic ) );
		log->Log( String::Printf( TXT("      >Entities : %d | %d | Diff %d" ), b.m_worldLayers.m_entitiesNum, a.m_worldLayers.m_entitiesNum, b.m_worldLayers.m_entitiesNum - a.m_worldLayers.m_entitiesNum ) );
		log->Log( String::Printf( TXT("      >Component: %d | %d | Diff %d" ), b.m_worldLayers.m_attachedCompNum, a.m_worldLayers.m_attachedCompNum, b.m_worldLayers.m_attachedCompNum - a.m_worldLayers.m_attachedCompNum ) );

		for ( Uint32 i=0; i<c_MaximumMemoryClasses; ++i )
		{
			Float diff = b.m_sizes[ i ] - a.m_sizes[ i ];
			if ( MAbs( diff ) > 0.001f )
			{
				AnsiChar className[64] = {'\0'};
				Memory::GetMemoryClassName( i, className, ARRAY_COUNT( className ) );
				log->Log( String::Printf( TXT("%30" ) RED_PRIWas TXT( " %6.2f;"), className, diff ) );
			}
		}

		log->Log( TXT("") );

		ObjectsByClassesSnapshot::Compare( a.m_objectsState, b.m_objectsState );

		log->Log( TXT("") );
		log->Log( TXT("") );
	}

	void RPrint( const SLayerGroupMemorySnapshot& group, Int32 depth, Int32 maxDepth, CDebugPageLogInterface* log )
	{
		const Float invMb = 1.f / 1024.f / 1024.f;
		{
			String str;
			for ( Int32 i=0; i<depth; ++i )
			{
				str += TXT("\t");
			}
			str += String::Printf( TXT("%s; loaded %d, ms %1.3f; mse %1.3f; md %1.3f; s %d; l %d; e %d, c %d"), 
				group.m_name.AsChar(), (Uint32)group.m_loaded,
				group.m_memStatic * invMb, group.m_memSerialize * invMb, group.m_memDynamic * invMb,
				group.m_subGroupsSnapshot.Size(), group.m_layersSnapshot.Size(),
				group.m_entitiesNum, group.m_attachedCompNum );

			log->Log( str );
		}

		if ( depth > maxDepth )
		{
			return;
		}

		const Uint32 subSize = group.m_subGroupsSnapshot.Size();
		for ( Uint32 i=0; i<subSize; ++i )
		{
			const SLayerGroupMemorySnapshot& s = group.m_subGroupsSnapshot[ i ];

			RPrint( s, depth+1, maxDepth, log );
		}

		const Uint32 layerSize = group.m_layersSnapshot.Size();
		for ( Uint32 i=0; i<layerSize; ++i )
		{
			const SLayerMemorySnapshot& s = group.m_layersSnapshot[ i ];

			//RPrint( s, depth+1, log );
		}
	}

	void RPrint( const SLayerMemorySnapshot& layer, Int32 depth, CDebugPageLogInterface* log )
	{
		const Float invMb = 1.f / 1024.f / 1024.f;

		String str;
		for ( Int32 i=0; i<depth; ++i )
		{
			str += TXT("\t");
		}
		str += String::Printf( TXT("%s; ms %1.3f; mse %1.3f; md %1.3f; e %d; c %d"), 
			layer.m_name.AsChar(),
			layer.m_memStatic * invMb, layer.m_memSerialize * invMb, layer.m_memDynamic * invMb,
			layer.m_entitiesNum, layer.m_attachedCompNum );

		log->Log( str );
	}
};

//////////////////////////////////////////////////////////////////////////

class IDebugLogCommandBox : public IDebugCommandBox
{
protected:
	CDebugPageLogInterface* m_log;

public:
	IDebugLogCommandBox( IDebugCheckBox* parent, const String& name, CDebugPageLogInterface* log )
		: IDebugCommandBox( parent, name )
		, m_log( log )
	{

	}

protected:
	void Log( const String& str )
	{
		m_log->Log( str );
	}
};

class CDebugDumpLayersSizeCommandBox : public IDebugLogCommandBox
{
public:
	CDebugDumpLayersSizeCommandBox( IDebugCheckBox* parent, CDebugPageLogInterface* log )
		: IDebugLogCommandBox( parent, TXT("Layers size"), log )
	{};

	virtual void Process()
	{
		if ( GGame->GetActiveWorld() )
		{
			// Create and fill memory snapshot
			CWorld* world = GGame->GetActiveWorld();
		}
	}

private:
};

class CDebugDumpDynamicLayerCommandBox : public IDebugLogCommandBox
{
public:
	CDebugDumpDynamicLayerCommandBox( IDebugCheckBox* parent, CDebugPageLogInterface* log )
		: IDebugLogCommandBox( parent, TXT("Dynamic layer"), log )
	{};

	virtual void Process()
	{
		if ( GGame->GetActiveWorld() )
		{
			CDynamicLayer* l = GGame->GetActiveWorld()->GetDynamicLayer();
			if ( l )
			{
				const LayerEntitiesArray& entities = l->GetEntities();

				// Collect classes
				TSet< CClass* > classesSet;
				TDynArray< Uint32 > conters;

				const Uint32 size = entities.Size();
				for ( Uint32 i=0; i<size; ++i )
				{
					CEntity* ent = entities[ i ];
					if ( ent )
					{
						CClass* c = ent->GetClass();
						classesSet.Insert( c );
					}
				}

				// Log per class
				for ( TSet< CClass* >::iterator it = classesSet.Begin(); it != classesSet.End(); ++it )
				{
					CClass* c = *it;

					String str = String::Printf( TXT("Start class %s"), c->GetName().AsString().AsChar() );
					Log( str );

					Uint32 counter = 0;
					for ( Uint32 i=0; i<size; ++i )
					{
						CEntity* ent = entities[ i ];
						if ( ent && ent->GetClass() == c )
						{
							counter += 1;

							String templName = ent->GetEntityTemplate() ? ent->GetEntityTemplate()->GetDepotPath().AsChar() : TXT("Detached");
							String str = String::Printf( TXT("   %s - %s"), ent->GetFriendlyName().AsChar(), templName.AsChar() );

							Log( str );
						}
					}

					conters.PushBack( counter );
				}

				// Final
				Log( TXT("Summary") );
				Uint32 i = 0;
				for ( TSet< CClass* >::iterator it = classesSet.Begin(); it != classesSet.End(); ++it )
				{
					CClass* c = *it;

					String str = String::Printf( TXT("   %d, Class %s"), conters[ i ], c->GetName().AsString().AsChar() );
					Log( str );

					i += 1;
				}
			}
		}
	}
};


class CDebugDumpLayerStorageCommandBox : public IDebugLogCommandBox
{
public:
	CDebugDumpLayerStorageCommandBox( IDebugCheckBox* parent, CDebugPageLogInterface* log )
		: IDebugLogCommandBox( parent, TXT("Layer storage"), log )
	{};

	virtual void Process()
	{
		THashMap<CClass*,Uint32> sizeMap;
		THashMap<CClass*,Uint32> countMap;

		for ( THashMap<CGUID,SLayerStorageDumpInfo>::iterator it = GLayerStorageDumpInfoMap.Begin(); it != GLayerStorageDumpInfoMap.End(); ++it )
		{
			SLayerStorageDumpInfo info = it->m_second;
			sizeMap[info.m_class]+=info.m_size;
			countMap[info.m_class]++;
		}

		THashMap<CClass*,Uint32>::iterator it1 = sizeMap.Begin();
		THashMap<CClass*,Uint32>::iterator it2 = countMap.Begin();
		for ( ; it1 != sizeMap.End() && it2 != countMap.End(); ++it1, ++it2 )
		{
			Log( String::Printf( TXT("%s                       %d       %d"), it1->m_first->GetName().AsString().AsChar(), it1->m_second, it2->m_second ) );
		}
	}
};

class CDebugDumpFileDepotPathStringsSizeCommandBox : public IDebugLogCommandBox
{
public:
	CDebugDumpFileDepotPathStringsSizeCommandBox( IDebugCheckBox* parent, CDebugPageLogInterface* log )
		: IDebugLogCommandBox( parent, TXT("Depot file strings size"), log )
	{};

	virtual void Process()
	{
		Uint32 size = 0;

		for ( ObjectIterator< CResource > it; it; ++it )
		{
			size += static_cast< Uint32 >( (*it)->GetDepotPath().DataSize() );
		}

		Float s = size / ( 1024.f * 1024.f );

		String str = String::Printf( TXT("Depot file strings size: %1.2f"), s );
		Log( str );
	}
};

class CDebugDumpNodeNameStringsSizeCommandBox : public IDebugLogCommandBox
{
public:
	CDebugDumpNodeNameStringsSizeCommandBox( IDebugCheckBox* parent, CDebugPageLogInterface* log )
		: IDebugLogCommandBox( parent, TXT("Node name strings size"), log )
	{};

	virtual void Process()
	{
		Uint32 size = 0;

		for ( ObjectIterator< CNode> it; it; ++it )
		{
			size += static_cast< Uint32 >( (*it)->GetName().DataSize() );
		}

		Float s = size / ( 1024.f * 1024.f );

		String str = String::Printf( TXT("Node name strings size: %1.2f"), s );
		Log( str );
	}
};

#ifndef NO_LOG

class CDebugDumpAllObjectListCommandBox : public IDebugLogCommandBox
{
public:
	CDebugDumpAllObjectListCommandBox( IDebugCheckBox* parent, CDebugPageLogInterface* log )
		: IDebugLogCommandBox( parent, TXT("All object list"), log )
	{};

	virtual void Process()
	{
		CObject::DebugDumpList();
	}
};


class CDebugDumpRootsetObjectListCommandBox : public IDebugLogCommandBox
{
public:
	CDebugDumpRootsetObjectListCommandBox( IDebugCheckBox* parent, CDebugPageLogInterface* log )
		: IDebugLogCommandBox( parent, TXT("Rootset object list"), log )
	{};

	virtual void Process()
	{
		CObject::DebugDumpRootsetList();
	}
};

class CDebugDumpDefultObjectListCommandBox : public IDebugLogCommandBox
{
public:
	CDebugDumpDefultObjectListCommandBox( IDebugCheckBox* parent, CDebugPageLogInterface* log )
		: IDebugLogCommandBox( parent, TXT("Default object list"), log )
	{};

	virtual void Process()
	{
		CObject::DebugDumpDefaultObjectList();
	}
};

#endif

#ifdef ENABLE_EXTENDED_MEMORY_METRICS

class CDebugMemSnapshotCreateCommandBox : public IDebugCommandBox
{
	CDebugPageMemoryBudget* m_page;

public:
	CDebugMemSnapshotCreateCommandBox( IDebugCheckBox* parent, CDebugPageMemoryBudget* page )
		: IDebugCommandBox( parent, TXT("Create new") )
		, m_page( page )
	{};

	virtual void Process();
};

class CDebugMemSnapshotDeleteCommandBox : public IDebugCommandBox
{
	CDebugPageMemoryBudget* m_page;

public:
	CDebugMemSnapshotDeleteCommandBox( IDebugCheckBox* parent, CDebugPageMemoryBudget* page )
		: IDebugCommandBox( parent, TXT("Delete selected") )
		, m_page( page )
	{};

	virtual void Process();
};

class CDebugMemSnapshotCompareCommandBox : public IDebugCommandBox
{
	CDebugPageMemoryBudget* m_page;

public:
	CDebugMemSnapshotCompareCommandBox( IDebugCheckBox* parent, CDebugPageMemoryBudget* page )
		: IDebugCommandBox( parent, TXT("Compare") )
		, m_page( page )
	{};

	virtual void Process();
};

#endif

class CDebugDumpWorldArraysCommandBox : public IDebugLogCommandBox
{
public:
	CDebugDumpWorldArraysCommandBox( IDebugCheckBox* parent, CDebugPageLogInterface* log )
		: IDebugLogCommandBox( parent, TXT("World arrays"), log )
	{};

	virtual void Process()
	{
		CGameWorld* world = GCommonGame->GetActiveWorld();
		if ( world )
		{
			Uint32 spNum = world->GetSpawnPointComponentNum();
			Uint32 ipNum = world->GetInterestPointComponentNum();

			String str = String::Printf( TXT("World arrays:") );
			Log( str );
			str = String::Printf( TXT("   >Spawn points: %d"), spNum );
			Log( str );
			str = String::Printf( TXT("   >Interest points: %d"), ipNum );
			Log( str );
		}
	}
};

class CDebugDumpNpcsCommandBox : public IDebugLogCommandBox
{
public:
	CDebugDumpNpcsCommandBox( IDebugCheckBox* parent, CDebugPageLogInterface* log )
		: IDebugLogCommandBox( parent, TXT("Npc list"), log )
	{};

	virtual void Process()
	{
		const CCommonGame::TNPCCharacters& npcs = GCommonGame->GetNPCCharacters();
		
		Log( TXT("Npc list:") );

		Uint32 counter = 0;

		for ( CCommonGame::TNPCCharacters::const_iterator npcIt = npcs.Begin(); npcIt != npcs.End(); ++npcIt )
		{
			CNewNPC* npc = *npcIt;

			// Name
			String str = String::Printf( TXT("%d. %s"), counter, npc->GetFriendlyName().AsChar() );
			Log( str );

			// Template
			String templ = npc->GetEntityTemplate() ? npc->GetEntityTemplate()->GetDepotPath().AsChar() : TXT("Detached");
			str = String::Printf( TXT("   >Template: %s"), templ.AsChar() );
			Log( str );

			// Status
			String temp = ToString( npc->IsAttached() );
			str = String::Printf( TXT("   >Is attached: %s"), temp.AsChar() );
			Log( str );

			// State
			str = String::Printf( TXT("   >State: %s"), npc->GetCurrentStateName().AsString().AsChar() );
			Log( str );

			// Tags
			str = String::Printf( TXT("   >Tags: %s"), npc->GetTags().ToString().AsChar() );
			Log( str );

			counter += 1;
		}
	}
};

class CForceGCTimerCheckBox : public IDebugCheckBox
{
	Float	m_timer;
	Bool	m_checked;

public:
	CForceGCTimerCheckBox( IDebugCheckBox* parent )
		: IDebugCheckBox( parent, TXT("Force GC timer"), false, true )
		, m_timer( 0.f )
		, m_checked( false )
	{
		
	};

	virtual Bool IsChecked() const
	{
		return m_checked;
	}

	virtual void OnToggle()
	{
		IDebugCheckBox::OnToggle();

		m_checked = !m_checked;

		m_timer = 0.f;
	}

	virtual void OnTick( Float timeDelta )
	{
		IDebugCheckBox::OnTick( timeDelta );

		if ( IsChecked() )
		{
			m_timer += timeDelta;

			const Float duration = 10.f;

			if ( m_timer > duration )
			{
				m_timer = 0.f;

				SGarbageCollector::GetInstance().CollectNow();
			}
		}
	}
};

class CDebugDumpAnimEventsSizeByClassCommandBox : public IDebugLogCommandBox
{
public:
	CDebugDumpAnimEventsSizeByClassCommandBox( IDebugCheckBox* parent, CDebugPageLogInterface* log )
		: IDebugLogCommandBox( parent, TXT("Anim event size by class"), log )
	{};

	virtual void Process();
};



//////////////////////////////////////////////////////////////////////////

class CDebugLineBox : public IDebugCheckBox
{
public:
	CDebugLineBox( IDebugCheckBox* parent )
		: IDebugCheckBox( parent, TXT(""), false, false )
	{

	}

	void OnRender( CRenderFrame* frame, Uint32 x, Uint32 &y, Uint32 &counter, const RenderOptions& options )
	{
		IDebugCheckBox::OnRender( frame, x, y, counter, options );

		if ( y > options.m_maxY )
		{
			return;
		}

		Color color( 200, 200, 200, 200 );

		frame->AddDebugFrame( x, y - 3 * LINE_HEIGHT / 2, 400, 1, color );

		y -= LINE_HEIGHT / 2;
	}
};

//////////////////////////////////////////////////////////////////////////

#endif