/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "layer.h"
#include "layerGroup.h"
#include "appearanceComponent.h"
#include "jobSpawnEntity.h"
#include "mesh.h"
#include "pathlibGenerationManager.h"
#include "pathlibWorld.h"
#include "game.h"
#include "meshComponent.h"
#include "persistentEntity.h"
#include "componentIterator.h"
#include "dynamicLayer.h"
#include "renderer.h"

#include "sectorData.h"
#include "sectorDataBuilder.h"

#include "destructionSystemComponent.h"

#include "../core/loadingJobManager.h"
#include "../core/dependencyLoader.h"
#include "../core/versionControl.h"
#include "../core/depot.h"
#include "../core/memoryFileAnalizer.h"
#include "../core/dependencySaver.h"
#include "../core/configFileManager.h"
#include "../core/cooker.h"
#include "../core/scriptSnapshot.h"
#include "../core/dataError.h"
#include "../core/resourceUsage.h"
#include "../core/debugPageHTMLDoc.h"
#include "layerInfo.h"
#include "world.h"

IMPLEMENT_ENGINE_CLASS( CLayer );

EntitySpawnInfo::EntitySpawnInfo()
	: m_template( nullptr )
	, m_resource( nullptr )
	, m_entityClass( nullptr )
	, m_spawnPosition( 0,0,0 )
	, m_spawnRotation( 0,0,0 )
	, m_spawnScale( 1,1,1 )
	, m_detachTemplate( false )
	, m_previewOnly( false )
	, m_entityFlags( 0 )
	, m_canThrowOutUnusedComponents( false )
	, m_entityNotSavable( false )
	, m_importantEntity( false )
	, m_forceNonStreamed( false )
	, m_encounterEntryGroup( -1 )
{}

EntitySpawnInfo::EntitySpawnInfo( EntitySpawnInfo&& c )
	: m_name( std::move( c.m_name ) )
	, m_appearances( std::move( c.m_appearances ) )
	, m_tags( std::move( c.m_tags) )
	, m_template( c.m_template )
	, m_handler( std::move( c.m_handler ) )
	, m_resource( c.m_resource )
	, m_entityClass( c.m_entityClass )
	, m_spawnPosition( c.m_spawnPosition )
	, m_spawnRotation( c.m_spawnRotation )
	, m_spawnScale( c.m_spawnScale )
	, m_idTag( c.m_idTag )
	, m_guid( c.m_guid )
	, m_detachTemplate( c.m_detachTemplate )
	, m_previewOnly( c.m_previewOnly )
	, m_entityFlags( c.m_entityFlags )
	, m_canThrowOutUnusedComponents( c.m_canThrowOutUnusedComponents )
	, m_entityNotSavable( c.m_entityNotSavable )
	, m_importantEntity( c.m_importantEntity )
	, m_forceNonStreamed( c.m_forceNonStreamed )
	, m_encounterEntryGroup( c.m_encounterEntryGroup )
{}
EntitySpawnInfo & EntitySpawnInfo::operator=( EntitySpawnInfo && c )
{
	if( this != &c )
	{
		m_name = std::move( c.m_name );
		m_appearances = std::move( c.m_appearances );
		m_tags = std::move( c.m_tags);
		m_template = c.m_template;
		m_handler = std::move( c.m_handler );
		m_resource = c.m_resource;
		m_entityClass = c.m_entityClass;
		m_spawnPosition = c.m_spawnPosition;
		m_spawnRotation = c.m_spawnRotation;
		m_spawnScale = c.m_spawnScale;
		m_idTag = c.m_idTag;
		m_guid = c.m_guid;
		m_detachTemplate = c.m_detachTemplate;
		m_previewOnly = c.m_previewOnly;
		m_entityFlags = c.m_entityFlags;
		m_canThrowOutUnusedComponents = c.m_canThrowOutUnusedComponents;
		m_entityNotSavable = c.m_entityNotSavable;
		m_importantEntity = c.m_importantEntity;
		m_forceNonStreamed = c.m_forceNonStreamed;
		m_encounterEntryGroup = c.m_encounterEntryGroup;
	}
	
	return *this;
}

EntitySpawnInfo::~EntitySpawnInfo()
{}

ISpawnEventHandler::~ISpawnEventHandler()
{
	if ( m_next )
	{
		delete m_next;
	}
}


void ISpawnEventHandler::OnPostAttach( CEntity* entity )
{}

Bool ISpawnEventHandler::IsA( CName eventClass )
{
	return false;
}

void EntitySpawnInfo::AddHandler( ISpawnEventHandler* handler )
{
	// this code is simple, but it reverse handlers order (which we don't like, because its counterintuitive)
	//handler->m_next = m_handler;
	//m_handler = handler;
	// this code is slow, but it works fine
	if ( !m_handler )
	{
		m_handler.Reset( handler );
	}
	else
	{
		ISpawnEventHandler* itHandler = m_handler.Get();
		while ( itHandler->m_next )
		{
			itHandler = itHandler->m_next;
		}
		itHandler->m_next = handler;
	}
}

void EntitySpawnInfo::OnPostAttach( CEntity* entity ) const
{
	ISpawnEventHandler* handler = m_handler.Get();
	while ( handler )
	{
		handler->OnPostAttach( entity );
		handler = handler->m_next;
	}
}

///////////////////////////////////////////////////////////////////////////////
// CLayer
///////////////////////////////////////////////////////////////////////////////
CLayer::CLayer()
	: m_world( NULL )
    , m_cached( true )
	, m_layerInfo( NULL )
	, m_sectorData( nullptr )
	, m_sectorDataId( 0 )
{
#ifndef NO_EDITOR_EVENT_SYSTEM
	SEvents::GetInstance().RegisterListener( CNAME( EntityTemplateChanged ), this );
#endif
}

#ifndef NO_EDITOR_RESOURCE_SAVE

void CLayer::OnResourceSavedInEditor()
{
	// Pass to base class
	TBaseClass::OnResourceSavedInEditor();

	// Remove empty entities
	RemoveEmptyEntityPointers();

	// Update all entities
	for ( Uint32 i=0; i<m_entities.Size(); ++i )
	{
		CEntity* entity = m_entities[i];
		entity->OnLayerSavedInEditor();
	}
}

#endif

void CLayer::OnSave()
{
	TBaseClass::OnSave();

#ifndef NO_EDITOR_WORLD_SUPPORT

	// Append layer info object
	if ( m_layerInfo != NULL )
	{
		m_layerInfo->AppendLayerInfoObject();
	}

#endif
}

void CLayer::OnSerialize( IFile& file )
{
#ifndef RED_FINAL_BUILD
	// Prevent entities to be duplicated in the entity array
	// This is a serious issue and we don't know yet why there are duplicated entities in the m_entites array
	if ( file.IsWriter() && !file.IsGarbageCollector() )
	{
		for ( Uint32 i=0; i<m_entities.Size(); ++i )
		{
			CEntity* entity = m_entities[i];
			if ( entity )
			{
				// NULL the duplicates
				for ( Uint32 j=i+1; j<m_entities.Size(); ++j )
				{
					if ( m_entities[j] == entity )
					{
						RED_HALT( "Entity '%ls' from index %d duplicated at index %d", 
							entity->GetFriendlyName().AsChar(), i, j );
						m_entities[j] = nullptr;
					}
				}
			}
		}

		RemoveEmptyEntityPointers();
	}
#endif

	// Pass to base class
	TBaseClass::OnSerialize( file );

	// Remove empty entries
#ifndef NO_RESOURCE_IMPORT
	if ( file.IsReader() )
	{
		RemoveEmptyEntityPointers();
	}
#endif
}

namespace Helper
{
	class LayerCookingStats
	{
	public:
		struct LayerStats
		{
			String		m_depotPath;
			Uint32		m_numEntitiesDestroyed;
			Uint32		m_numEntitiesDetached;
			Uint32		m_numEntitiesUnstreamed;
			Uint32		m_numEntitiesGameplay;
			Uint32		m_numEntitiesLeft;
			Uint32		m_numComponentsExtracted;

			LayerStats( const String& info )
				: m_depotPath( info )
			{
				m_numEntitiesDestroyed = 0;
				m_numEntitiesDetached = 0;
				m_numEntitiesUnstreamed = 0;
				m_numEntitiesGameplay = 0;
				m_numEntitiesLeft = 0;
				m_numComponentsExtracted = 0;
			}
		};

		struct MergedStats
		{
			Uint32		m_numEntitiesDestroyed;
			Uint32		m_numEntitiesDetached;
			Uint32		m_numEntitiesUnstreamed;
			Uint32		m_numEntitiesGameplay;
			Uint32		m_numEntitiesNoExtractFlag;
			Uint32		m_numEntitiesLeft;
			Uint32		m_numComponentsExtracted;
			Uint32		m_sizeOfStreamingDataInitial;
			Uint32		m_sizeOfStreamingDataFinal;
			Uint32		m_numComponentsTotal;
			Uint32		m_numComponentsStatic;
			Uint32		m_numComponentsTemplated;
			Uint32		m_numComponentsStreamable;
			Uint32		m_sizeOfSectorData;
			Uint32		m_sizeOfExtractedComponents;

			MergedStats()
				: m_numEntitiesDestroyed( 0 )
				, m_numEntitiesDetached( 0 )
				, m_numEntitiesUnstreamed( 0 )
				, m_numEntitiesGameplay( 0 )
				, m_numEntitiesLeft( 0 )
				, m_numComponentsExtracted( 0 )
				, m_sizeOfStreamingDataInitial( 0 )
				, m_sizeOfStreamingDataFinal( 0 )
				, m_sizeOfSectorData( 0 )
				, m_sizeOfExtractedComponents( 0 ) 
				, m_numComponentsTotal( 0 )
				, m_numComponentsStatic( 0 )
				, m_numComponentsTemplated( 0 )
				, m_numComponentsStreamable( 0 )
				, m_numEntitiesNoExtractFlag( 0 )
			{}
		};

		struct CompStats
		{
			const CClass*	m_class;
			Uint32			m_numTotal;
			Uint32			m_numStatic;
			Uint32			m_numStreamable;
			Uint32			m_numExtracted;
			Uint32			m_numFromTemplate;

			CompStats( const CClass* compClass )
				: m_class( compClass )
				, m_numTotal( 0 )
				, m_numStatic( 0 )
				, m_numStreamable( 0 )
				, m_numExtracted( 0 )
				, m_numFromTemplate( 0 )
			{}
		};

		struct EntityTemplateStats
		{
			const CDiskFile*	m_file;
			const CClass*		m_class;
			Uint32				m_count;

			EntityTemplateStats()
				: m_count(1)
				, m_file(nullptr)
				, m_class(nullptr)
			{}
		};

		struct EntityLeftStats
		{
			const CClass*		m_class;
			Uint32				m_count;

			EntityLeftStats()
				: m_count(1)
				, m_class(nullptr)
			{}
		};

		String						m_world;
		MergedStats					m_total;
		TDynArray< LayerStats >		m_layers;
		LayerStats*					m_currentLayer;

		THashMap< const CDiskFile*, EntityTemplateStats* >	m_templateStatsMap;
		THashMap< const CClass*, EntityLeftStats* >			m_entitiesLeftMap;
		THashMap< const CClass*, CompStats* >				m_compStatsMap;
		TDynArray< CompStats* >								m_compStats;

		LayerCookingStats( const String& world )
			: m_world( world )
		{
			m_total = MergedStats();
			m_currentLayer = nullptr;
			m_layers.ClearFast();
			m_compStats.ClearPtr();
			m_entitiesLeftMap.ClearPtr();
			m_templateStatsMap.ClearPtr();
			m_compStatsMap.Clear();
		}

		void StartLayer( const String& depotPath )
		{
			m_layers.PushBack( LayerStats( depotPath ) );
			m_currentLayer = &m_layers.Back();
		}

		CompStats* GetCompStats( const CClass* compClass )
		{
			CompStats* stats = nullptr;
			m_compStatsMap.Find( compClass, stats );
			if ( !stats )
			{
				stats = new CompStats( compClass );
				m_compStatsMap.Insert( compClass, stats );
				m_compStats.PushBack( stats );
			}
			return stats;
		}

		void ReportInitialEntity( CEntity* entity )
		{
			m_total.m_sizeOfStreamingDataInitial += (Uint32)entity->GetLocalStreamedComponentDataBuffer().GetSize();
		}

		void ReportDetachedEntity( CEntity* )
		{
			if ( m_currentLayer )
				m_currentLayer->m_numEntitiesDetached += 1;

			m_total.m_numEntitiesDetached += 1;
		}

		void ReportDestroyedEntity( CEntity* )
		{
			if ( m_currentLayer )
				m_currentLayer->m_numEntitiesDestroyed += 1;

			m_total.m_numEntitiesDestroyed += 1;
		}

		void ReportDestramedEntity( CEntity* )
		{
			if ( m_currentLayer )
				m_currentLayer->m_numEntitiesUnstreamed += 1;

			m_total.m_numEntitiesUnstreamed += 1;
		}

		void ReportEntityNoCookFlag( CEntity* )
		{
			m_total.m_numEntitiesNoExtractFlag += 1;
		}

		void ReportKeptEntity( CEntity* entity )
		{
			if ( entity->GetEntityTemplate() )
			{
				// this is an entity from a template entity, report the template that kept us on the level
				const CDiskFile* entityTemplateFile = entity->GetEntityTemplate()->GetFile();
				if ( entityTemplateFile )
				{
					// create extraction stats
					EntityTemplateStats* stats = nullptr;
					m_templateStatsMap.Find( entityTemplateFile, stats );
					if ( !stats )
					{
						stats = new EntityTemplateStats();
						stats->m_class = entity->GetClass();
						stats->m_file = entityTemplateFile;
						stats->m_count = 1;

						m_templateStatsMap.Insert( entityTemplateFile, stats );
					}
					else
					{
						stats->m_count += 1;
					}
				}
			}
			else
			{
				// this is a normal entity - report why we could not been processed
				EntityLeftStats* stats = nullptr;
				m_entitiesLeftMap.Find( entity->GetClass(), stats );
				if ( !stats )
				{
					stats = new EntityLeftStats();
					stats->m_class = entity->GetClass();
					stats->m_count = 1;
					m_entitiesLeftMap.Insert( entity->GetClass(), stats );
				}
				else
				{
					stats->m_count += 1;
				}
			}
		}

		void ReportGameplayEntity( CEntity* entity )
		{
			if ( m_currentLayer )
				m_currentLayer->m_numEntitiesGameplay += 1;

			m_total.m_numEntitiesGameplay += 1;
		}

		void ReportComponentExtracted( CComponent* comp )
		{
			if ( m_currentLayer )
				m_currentLayer->m_numComponentsExtracted += 1;

			m_total.m_numComponentsExtracted += 1;

			CompStats* stats = GetCompStats( comp->GetClass() );
			stats->m_numExtracted += 1;

			m_total.m_sizeOfExtractedComponents += comp->GetClass()->GetSize() + comp->GetClass()->GetScriptDataSize();
		}

		void ReportSectorData( const CSectorData* data )
		{
			m_total.m_sizeOfSectorData += data->CalcMemorySize();
		}

		void ReportFinalEntity( CEntity* entity )
		{
			m_total.m_sizeOfStreamingDataFinal += (Uint32)entity->GetLocalStreamedComponentDataBuffer().GetSize();

			if ( m_currentLayer )
				m_currentLayer->m_numEntitiesLeft += 1;

			m_total.m_numEntitiesLeft += 1;

			for ( const CComponent* comp : entity->GetComponents() )
			{
				CompStats* stats = GetCompStats( comp->GetClass() );

				stats->m_numTotal += 1;
				m_total.m_numComponentsTotal += 1;

				if ( entity->GetEntityTemplate() != nullptr )
				{
					stats->m_numFromTemplate += 1;
					m_total.m_numComponentsTemplated += 1;
				}
				else if ( comp->HasComponentFlag( CF_StreamedComponent ) )
				{
					stats->m_numStreamable += 1;
					m_total.m_numComponentsStreamable += 1;
				}
				else
				{
					stats->m_numStatic += 1;
					m_total.m_numComponentsStatic += 1;
				}
			}
		}

		void Dump()
		{
			RED_LOG( WCC, TXT("-----------------------------------------------------------------------") );
			RED_LOG( WCC, TXT("-- Per layer cooking stats for %ls"), m_world.AsChar() );
			RED_LOG( WCC, TXT("-----------------------------------------------------------------------") );
			RED_LOG( WCC, TXT("|  Des  |  Det  |  Uns  |  Ext  |  Gam  |  Left | Name ") );
			RED_LOG( WCC, TXT("-----------------------------------------------------------------------") );

			::Sort( m_layers.Begin(), m_layers.End(), [](const LayerStats& a, const LayerStats& b) { return a.m_depotPath < b.m_depotPath; } );
			for ( const auto& it : m_layers )
			{
				RED_LOG( WCC, TXT("| %5d | %5d | %5d | %5d | %5d | %5d | %ls "),
					it.m_numEntitiesDestroyed, it.m_numEntitiesDetached, 
					it.m_numEntitiesUnstreamed, it.m_numComponentsExtracted,
					it.m_numEntitiesGameplay, it.m_numEntitiesLeft, it.m_depotPath.AsChar() );
			}

			RED_LOG( WCC, TXT("") );

			{
				RED_LOG( WCC, TXT("-----------------------------------------------------------------------") );
				RED_LOG( WCC, TXT("-- Per component cooking stats for %ls"), m_world.AsChar() );
				RED_LOG( WCC, TXT("-----------------------------------------------------------------------") );
				RED_LOG( WCC, TXT("|  Total  | Streamed |  Static | Templated | Name") );
				RED_LOG( WCC, TXT("-----------------------------------------------------------------------") );

				::Sort( m_compStats.Begin(), m_compStats.End(), [](const CompStats* a, const CompStats* b) { return a->m_numTotal > b->m_numTotal; } );
				for ( const auto* it : m_compStats )
				{
					RED_LOG( WCC, TXT("| %7d | %8d | %7d | %9d | %ls"),
						it->m_numTotal, 
						it->m_numStreamable, it->m_numStatic, it->m_numFromTemplate,
						it->m_class->GetName().AsChar() );
				}

				RED_LOG( WCC, TXT("") );
			}

			{
				RED_LOG( WCC, TXT("-----------------------------------------------------------------------") );
				RED_LOG( WCC, TXT("-- Templates that were not detached") );
				RED_LOG( WCC, TXT("-----------------------------------------------------------------------") );
				RED_LOG( WCC, TXT("|  Count  | Entity Class + Template |") );
				RED_LOG( WCC, TXT("-----------------------------------------------------------------------") );

				TDynArray< EntityTemplateStats* > entityTemplateStats;
				for ( auto it = m_templateStatsMap.Begin(); it != m_templateStatsMap.End(); ++it )
				{
					entityTemplateStats.PushBack( (*it).m_second );
				}

				::Sort( entityTemplateStats.Begin(), entityTemplateStats.End(), [](EntityTemplateStats* a, EntityTemplateStats*b) { return a->m_count > b->m_count;	} );
				for ( const auto* etStats : entityTemplateStats )
				{
					RED_LOG( WCC, TXT("| %7d | %ls : %ls"),
						etStats->m_count, 
						etStats->m_class->GetName().AsChar(), 
						etStats->m_file->GetDepotPath().AsChar() );
				}

				RED_LOG( WCC, TXT("") );
			}

			{
				RED_LOG( WCC, TXT("-----------------------------------------------------------------------") );
				RED_LOG( WCC, TXT("-- Entities that were not converted") );
				RED_LOG( WCC, TXT("-----------------------------------------------------------------------") );
				RED_LOG( WCC, TXT("|  Count  | Entity Class") );
				RED_LOG( WCC, TXT("-----------------------------------------------------------------------") );

				TDynArray< EntityLeftStats* > stats;
				for ( auto it = m_entitiesLeftMap.Begin(); it != m_entitiesLeftMap.End(); ++it )
				{
					stats.PushBack( (*it).m_second );
				}

				::Sort( stats.Begin(), stats.End(), [](EntityLeftStats* a, EntityLeftStats* b) { return a->m_count > b->m_count;	} );
				for ( const auto* etStats : stats )
				{
					RED_LOG( WCC, TXT("| %7d | %ls"),
						etStats->m_count, 
						etStats->m_class->GetName().AsChar() );
				}

				RED_LOG( WCC, TXT("") );
			}


			RED_LOG( WCC, TXT("-----------------------------------------------------------------------") );
			RED_LOG( WCC, TXT("Totals for %ls:"), m_world.AsChar() );
			RED_LOG( WCC, TXT("  Components extracted: %d"), m_total.m_numComponentsExtracted );
			RED_LOG( WCC, TXT("  Components left: %d"), m_total.m_numComponentsTotal );
			RED_LOG( WCC, TXT("    - from templates: %d"), m_total.m_numComponentsTemplated );
			RED_LOG( WCC, TXT("    - static: %d"), m_total.m_numComponentsStatic );
			RED_LOG( WCC, TXT("    - streamable: %d"), m_total.m_numComponentsStreamable );
			RED_LOG( WCC, TXT("") );
			RED_LOG( WCC, TXT("  Entities detached: %d"), m_total.m_numEntitiesDetached );
			RED_LOG( WCC, TXT("  Entities unstreamed: %d"), m_total.m_numEntitiesUnstreamed );
			RED_LOG( WCC, TXT("  Entities destroyed: %d"), m_total.m_numEntitiesDestroyed );
			RED_LOG( WCC, TXT("  Entities left: %d (%d gameplay)"), m_total.m_numEntitiesLeft, m_total.m_numEntitiesGameplay );
			RED_LOG( WCC, TXT("  Entities marked NoExtract: %d"), m_total.m_numEntitiesNoExtractFlag );
			RED_LOG( WCC, TXT("") );
			RED_LOG( WCC, TXT("  Final streaming data: %1.2f KB"), m_total.m_sizeOfStreamingDataFinal / 1024.0f );
			RED_LOG( WCC, TXT("  Removed entity streaming data: %1.2f KB"), ((Double)m_total.m_sizeOfStreamingDataInitial - (Double)m_total.m_sizeOfStreamingDataFinal) / 1024.0f );
			RED_LOG( WCC, TXT("  Removed component data: %1.2f KB"), m_total.m_sizeOfExtractedComponents / 1024.0f );
			RED_LOG( WCC, TXT("  Added sector data: %1.2f KB"), m_total.m_sizeOfSectorData / 1024.0f );
			RED_LOG( WCC, TXT("") );

			const Uint32 sizeBefore = m_total.m_sizeOfStreamingDataInitial + m_total.m_sizeOfExtractedComponents;
			const Uint32 sizeAfter = m_total.m_sizeOfStreamingDataFinal + m_total.m_sizeOfSectorData;
			RED_LOG( WCC, TXT("  Net gain: %1.2f KB"), ( sizeBefore - sizeAfter ) / 1024.0f );
		}
	};

	class LayerCookingStatsCollector
	{
	public:
		static LayerCookingStatsCollector& GetInstance()
		{
			static LayerCookingStatsCollector theInstance;
			return theInstance;
		}

		LayerCookingStats& GetStatsForLayer( const String& layerDepotPath )
		{
			if ( layerDepotPath.BeginsWith( TXT("levels\\") ) )
			{
				const String worldPath = layerDepotPath.StringAfter( TXT("\\") ).StringBefore( TXT("\\") );
				if ( !worldPath.Empty() )
				{
					for ( LayerCookingStats* stat : m_stats )
					{
						if ( stat->m_world == worldPath )
						{
							stat->StartLayer( layerDepotPath );
							return *stat;
						}
					}

					LayerCookingStats* newStats = new LayerCookingStats( worldPath );
					newStats->StartLayer( layerDepotPath );
					m_stats.PushBack( newStats );
					return *newStats;
				}
			}

			static LayerCookingStats dummyStats(String::EMPTY);
			return dummyStats;
		}

		void DumpAll()
		{
			for ( LayerCookingStats* stat : m_stats )
			{
				stat->Dump();
			}
		}

	private:
		TDynArray< LayerCookingStats* >	m_stats;
	};

}

void DumpLayerCookingStats()
{
	Helper::LayerCookingStatsCollector::GetInstance().DumpAll();
}

//! Debug info
#ifndef NO_DEBUG_PAGES

namespace Helper
{
	template< typename T >	
	static const AnsiChar* GetEnumOptionName( const T value )
	{
		static CEnum* enumType = static_cast< CEnum* >( GetTypeObject<T>() );
		if ( enumType )
		{
			CName ret;
			if ( enumType->FindName( (Int32) value, ret ) )
				return ret.AsAnsiChar();

			return "Unknown value";
		}

		return "Unknown enum";
	}
}

void CLayer::OnDebugPageInfo( class CDebugPageHTMLDocument& doc )
{
	TBaseClass::OnDebugPageInfo( doc );

	// link to layer lists
	{
		CDebugPageHTMLInfoBlock block( doc, "Layer entities" );
		block.Info( "Number of entities: " ).Writef( "%d", m_entities.Size() );

		const StringAnsi path = UNICODE_TO_ANSI( GetDepotPath().AsChar() );
		if ( !path.Empty() )
		{
			block.Info( "Layer data: " ).Link( "/layer/%hs", path.AsChar()).Write(path.AsChar());
		}
	}

	// layer info stuff
	if ( m_layerInfo != nullptr )
	{
		CDebugPageHTMLInfoBlock block( doc, "Layer information" );

		block.Info( "Attached: " ).Write( IsAttached() ? "Yes" : "No" );
		block.Info( "Build tag: " ).Write( Helper::GetEnumOptionName< ELayerBuildTag >( m_layerInfo->GetLayerBuildTag() ) );

		Char buf[64];
		m_layerInfo->GetGUID().ToString(buf, ARRAY_COUNT(buf));
		block.Info( "GUID: " ).Write( UNICODE_TO_ANSI(buf) );
	}

	// do we have layer storage ?
	if ( m_layerInfo && m_layerInfo->GetLayerStorage() )
	{
		CLayerStorage* storage = m_layerInfo->GetLayerStorage();

		CDebugPageHTMLInfoBlock block( doc, "Layer storage" );
		block.Info( "Number of entities " ).Writef( "%d", storage->GetNumEntities() );
		block.Info( "Number of components " ).Writef( "%d", storage->GetNumComponents() );
		block.Info( "Size of data: " ).Writef( "%1.2f KB", storage->GetDataSize() / 1024.0f );
	}
}
#endif

#ifndef NO_RESOURCE_COOKING

namespace Helper
{
	void ForceUpdateTransformForEntity( CNode* node )
	{
		SUpdateTransformContext context;
		node->UpdateTransformNode( context, true );
		context.CommitChanges();

		auto childs = node->GetChildAttachments();
		for ( auto it = childs.Begin(); it != childs.End(); ++it )
		{
			if ( (*it)->GetChild() )
				ForceUpdateTransformForEntity( (*it)->GetChild() );
		}

		CEntity* ent = node->AsEntity();
		if ( ent )
		{
			auto comps = ent->GetComponents();
			for ( CComponent* comp : comps )
			{
				 if ( !comp->GetTransformParent() )
				 {
					 ForceUpdateTransformForEntity( comp );
				 }
			}
		}
	}

}

void CLayer::OnCook( class ICookerFramework& cooker )
{
#ifndef NO_EDITOR
	TBaseClass::OnCook( cooker );

	// extract layer info
	CLayerInfo* layerInfo = CLayerGroup::GrabRawDynamicLayerInfo( GetDepotPath() );
	if ( !layerInfo )
	{
		ERR_ENGINE( TXT("Layer '%ls' cannot be cooked because it does not contain layer info"), GetDepotPath().AsChar() );
		return;
	}

	// on quest layers we need to be a little bit more careful
	Bool environmentLayer = false;
	if ( layerInfo->GetLayerBuildTag() == LBT_EnvIndoor || layerInfo->GetLayerBuildTag() == LBT_EnvOutdoor || layerInfo->GetLayerBuildTag() == LBT_EnvUnderground )
	{
		environmentLayer = true;
	}

	// create data holder
	if ( !m_sectorData )
	{
		m_sectorData = new CSectorData();
	}

	// create data builder
	CSectorDataBuilder builder( m_sectorData );

	// get the stats containers
	Helper::LayerCookingStats& stats = Helper::LayerCookingStatsCollector::GetInstance().GetStatsForLayer( GetDepotPath() );

	// detach entities that can be detached
	TDynArray< CEntity* > updatedEntities;
	THashMap< const CEntity*, THandle< CEntityTemplate > > detachedTemplatesMap;
	for ( CEntity* entity : m_entities )
	{
		stats.ReportInitialEntity( entity );

		CEntityTemplate* et = entity->GetEntityTemplate();
		Helper::ForceUpdateTransformForEntity( entity );

		if ( et && et->IsDetachable() )
		{
			detachedTemplatesMap.Insert( entity, et );
			// detach the template
			if ( entity->ShouldBeStreamed() )
			{
				entity->CreateStreamedComponents(SWN_DoNotNotifyWorld);
				//entity->UpdateStreamingDistance();<- this is fucking up the stremaing distance in cook
				entity->DetachTemplate( true );
				entity->UpdateStreamedComponentDataBuffers(true);
			}
			else
			{
				entity->DetachTemplate( true );
			}

			// make sure all transforms are up to date in this entity BEFORE we start extracting shit
			// this needs to happen AFTER detaching ;]
			Helper::ForceUpdateTransformForEntity(entity);
			entity->ForceUpdateBoundsNode();

			// make sure to update the streaming components buffers
			updatedEntities.PushBack( entity );
			stats.ReportDetachedEntity( entity );
		}
		else if ( et )
		{
			stats.ReportKeptEntity( entity );
		}
	}

	// process entities
	LayerEntitiesArray entitiesToKeep;
	for ( CEntity* entity : m_entities )
	{
		// we can only simplify the entities without templates
		if ( nullptr != entity->GetEntityTemplate() )
		{
			entitiesToKeep.PushBack(entity);
			continue;
		}

		//skip entities marked for not cooking
		if ( entity->CheckStaticFlag( ESF_NoCompExtract ) )
		{
			entitiesToKeep.PushBack(entity);
			stats.ReportEntityNoCookFlag( entity );
			stats.ReportKeptEntity( entity );
			continue;
		}

		// we should not touch non-vanila entities that are outside env layers
		if ( !entity->CanExtractComponents( environmentLayer ) )
		{
			entitiesToKeep.PushBack(entity);
			stats.ReportGameplayEntity( entity );
			stats.ReportKeptEntity( entity );
			continue;
		}

		// make sure all components are there
		entity->CreateStreamedComponents( SWN_DoNotNotifyWorld );

		// make sure all components have proper transforms
		Helper::ForceUpdateTransformForEntity(entity);

		// get current components from entity
		TDynArray< THandle< CComponent > > components;
		components.Resize( entity->GetComponents().Size() );
		for ( Uint32 i=0; i<entity->GetComponents().Size(); ++i )
		{
			components[i] = entity->GetComponents()[i];
		}

		// start processing
		Bool hasComponentsRemoved = false;
		TDynArray< CComponent* > componentsExtracted;
		for ( Uint32 i=0; i<components.Size(); ++i )
		{
			CComponent* comp = components[i];

			// component can be "lost" during extraction (inter-component dependencies, watch out for it)
			if ( !comp || comp->IsDestroyed() || comp->GetEntity() != entity )
				continue;

			// component extraction
			if ( comp->RemoveOnCookedBuild() || builder.ExtractComponent( comp ) )
			{
				componentsExtracted.PushBack( comp );
			}
		}

		// delete extracted components from entity
		// we deffer the deletion after extraction to make sure that we have access to ALL components during exctraction
		// this is required to capture some inter-component dependencies
		for ( Uint32 i=0; i<componentsExtracted.Size(); ++i )
		{
			CComponent* comp = componentsExtracted[i];

			// streamable component
			if ( comp->HasComponentFlag( CF_StreamedComponent ) )
			{
				entity->RemoveComponentFromStreaming( comp );
			}

			// component was extracted, remove it from the entity
			entity->RemoveComponent( comp );

			// stats
			hasComponentsRemoved = true;
			stats.ReportComponentExtracted( comp );
		}

		// if the entity was modified (components were extracted we need to repopulate internal buffers)
		if ( hasComponentsRemoved )
		{
			// do we have any streamable components left ? If not make this entity non-streamable
			Bool hasAnyStreamableComponents = false;
			for ( auto it : entity->GetComponents() )
			{
				if ( it->HasComponentFlag( CF_StreamedComponent ) )
				{
					hasAnyStreamableComponents = true;
					break;
				}
			}

			// if we don't have components at all entity can be destroyed
			if ( entity->GetComponents().Empty() )
			{
				THandle< CEntityTemplate > et;
				String templatePath;
				if ( detachedTemplatesMap.Find( entity, et ) ) 
				{
					if ( et ) 
					{
						templatePath = et->GetDepotPath();
					}
					else
					{
						templatePath = TXT("ERROR, template got removed before destroying entity");
						RED_LOG_ERROR( WCC, TXT("Template got removed before destroying entity") );
					}
				}
				else
				{
					templatePath = TXT("No template found");
				}
				RED_LOG( WCC, TXT("Destroying entity that got all components extracted: '%ls' from '%ls'. Entity template: '%ls' "), 
					entity->GetName().AsChar(),
					GetDepotPath().AsChar(),
					templatePath.AsChar() );
				stats.ReportDestroyedEntity( entity );
			}
			// update the stremable stuff or totaly remove it
			else if ( hasAnyStreamableComponents )
			{
				//entity->UpdateStreamingDistance(); <- this is fucking up the stremaing distance in cook
				entity->UpdateStreamedComponentDataBuffers(true);
				entitiesToKeep.PushBack( entity );
			}
			// we don't have any streamable components left, disable streaming on entity
			else
			{
				entity->SetStreamed( false );
				stats.ReportDestramedEntity( entity );
				entitiesToKeep.PushBack( entity );
			}
		}
		else
		{
			// nothing removed, keep it
			entitiesToKeep.PushBack( entity );
		}
	}

	// If we haven't extracted anything get rid of the data storage 
	if ( !m_sectorData->HasData() )
	{
		delete m_sectorData;
		m_sectorData = nullptr;
	}
	else
	{
		stats.ReportSectorData( m_sectorData );

		// Collect used resources
		TDynArray< String > usedResources;
		builder.GetUsedResources( usedResources );

		// Report resources used in the sector data as hard dependencies - those resources are mostly meshes
		for ( auto it = usedResources.Begin(); it != usedResources.End(); ++it )
		{
			const String& path = *it;

			if ( path.EndsWith( TXT("w2mesh") ) || path.EndsWith( TXT("xbm") ) )
			{
				cooker.ReportHardDependency( *it );
			}
			else
			{
				cooker.ReportSoftDependency( *it );
			}
		}
	}

	// Report meshes used by streamable geometry
	for ( CEntity* entity : m_entities )
	{
		TDynArray< String > entityStreamableResources;
		entity->CollectResourcesInStreaming( entityStreamableResources );

		for ( const String& path : entityStreamableResources )
		{
			if ( path.EndsWith( TXT("w2mesh") ) || path.EndsWith( TXT("xbm") ) )
			{
				cooker.ReportHardDependency( path );
			}
			else
			{
				cooker.ReportSoftDependency( path );
			}
		}
	}

	// Strip local data buffers from entities that will use templates any way
	for ( CEntity* entity : m_entities )
	{
		// we can only simplify the entities without templates
		if ( nullptr != entity->GetEntityTemplate() )
		{
			entity->PurgeStreamingBuffer();
		}
	}

	// Use the new entity list
	const Uint32 numOriginalEntities = m_entities.Size();
	m_entities = entitiesToKeep;

	// Report final entities
	for ( CEntity* entity : m_entities )
	{
		stats.ReportFinalEntity( entity );
	}

	// cleanup
	delete layerInfo;
#endif
}
#endif

void CLayer::RemoveEmptyEntityPointers()
{
	// This is removes any entity pointers from the internal list where the entities failed to be create for whatever reason
	// (bad templates, missing data, etc, etc)
	RemoveEmptyPointers( m_entities );
}

void CLayer::OnPostLoad()
{
	// Pass to base class
	TBaseClass::OnPostLoad();

	// Prevent entities to be duplicated in the entity array
	// This is a serious issue and we don't know yet why there are duplicated entities in the m_entites array
#ifndef RED_FINAL_BUILD
	for ( Uint32 i=0; i<m_entities.Size(); ++i )
	{
		CEntity* entity = m_entities[i];
		if ( entity )
		{
			// NULL the duplicates
			for ( Uint32 j=i+1; j<m_entities.Size(); ++j )
			{
				if ( m_entities[j] == entity )
					m_entities[j] = nullptr;
			}
		}
	}
#endif

	// Cleanup
	RemoveEmptyEntityPointers();

	// We need to update transform for all entities that were instanced from template
	// We are not yet ready for this fix...
	//if ( !IsCooked() )
	{
		for ( Uint32 i=0; i<m_entities.Size(); ++i )
		{
			CEntity* entity = m_entities[i];
			if ( entity->GetEntityTemplate() )
			{
				entity->ForceUpdateTransformNodeAndCommitChanges();
				entity->ForceUpdateBoundsNode();
			}
		}
	}
}

#ifndef NO_DATA_VALIDATION
void CLayer::OnCheckDataErrors() const
{
	// Pass to base class
	TBaseClass::OnCheckDataErrors();

	// No entities
	if ( !m_entities.Size() )
	{
		DATA_HALT( DES_Tiny, this, TXT("World"), TXT("Layer is empty and should be removed") );
	}

	// Process entities
	for ( Uint32 i=0; i<m_entities.Size(); ++i )
	{
		CEntity* entity = m_entities[i];
		if ( entity )
		{
			SEntityStreamingState state;

			entity->PrepareStreamingComponentsEnumeration( state, true, SWN_DoNotNotifyWorld );
			entity->ForceFinishAsyncResourceLoads();
			entity->ForceUpdateBoundsNode();
			entity->OnCheckDataErrors( false );
			entity->FinishStreamingComponentsEnumeration( state, SWN_DoNotNotifyWorld );
		}
	}
}
#endif

void CLayer::OnFinalize()
{
	// Unregister editor event listener
#ifndef NO_EDITOR_EVENT_SYSTEM
	SEvents::GetInstance().UnregisterListener( this );
#endif

	// Make sure sector data is deleted
	if ( m_sectorData )
	{
		delete m_sectorData;
		m_sectorData = nullptr;
		m_sectorDataId = 0;
	}

	// Pass to base class
	TBaseClass::OnFinalize();
}

void CLayer::SetLayerInfo( CLayerInfo* layerInfo ) 
{ 
	m_layerInfo = layerInfo; 
}

CLayerInfo* CLayer::GetLayerInfo() const
{ 
	//ASSERT( m_layerInfo );
	return m_layerInfo; 
}

Bool CLayer::PreventCollectingResource() const
{
	return false;
}

Bool CLayer::PreventFullUnloading() const
{
	return ShouldBeCached();
}

Bool CLayer::IsAttached() const
{
	return m_world != NULL;
}

void CLayer::LayerLoaded()
{
	for ( Uint32 i=0; i<m_entities.Size(); i++ )
	{
		CEntity* entity = m_entities[i];
		if ( entity )
		{
			entity->OnLoaded();
		}
	}
}

void CLayer::AttachToWorld( CWorld* world )
{
	PC_CHECKER_SCOPE( 0.001f, TXT("LAYERS"), TXT("Slow AttachToWorld") );

	// Make sure we are not attached, messing with those calls is a bad practice so be strict
	ASSERT( m_world == NULL );

	// Attach layer to world
	m_world = world;
	world->OnLayerAttached( this );

	// Add to list of layers attached to world
	VERIFY( m_world->m_attachedLayers.Insert( this ) );
	if ( !GetGUID().IsZero() )
	{
#ifndef NO_EDITOR_PATHLIB_SUPPORT
		CPathLibWorld* pathlib = m_world->GetPathLibWorld();
		if ( pathlib )
		{
			pathlib->GetGenerationManager()->CheckObsolateObstacles();
		}
#endif
	}
	
	// Inform all entities that were attached to the world 
	for ( Uint32 i = m_entities.Size(); i-- > 0; )
	{
		CEntity* entity = m_entities[ i ];
		if ( entity )
		{
			// Restore data from entity
			RestoreEntityState( entity );

			// Initialize
			ASSERT( !entity->IsInitialized() );
			entity->Initialize();
			ASSERT( entity->IsInitialized() );

			// Attach
			ASSERT( !entity->IsAttached() );
			entity->AttachToWorld( world );
			ASSERT( entity->IsAttached() );
		}
	}

	// Attach sector data in sector streaming system
	if ( m_sectorData )
	{
		// register sector data in world
		const Uint64 contentHash = Red::CalculateHash64( UNICODE_TO_ANSI( GetDepotPath().AsChar() ) );
		m_sectorDataId = world->RegisterSectorData( contentHash, m_sectorData, true );

		// delete source data
		delete m_sectorData;
		m_sectorData = nullptr;
	}

	// Toggle layer visibility
	if ( m_sectorDataId != 0 )
	{
		world->ToggleSectorDataVisibility( m_sectorDataId, true );
	}
}

void CLayer::DetachFromWorld()
{
	// Make sure we are attached, messing with those calls is a bad practice so be strict
	ASSERT( m_world != NULL );
	if ( m_world )
	{
		m_world->OnLayerDetached( this );
	}

	// Toggle layer visibility
	if ( m_sectorDataId != 0 )
	{
		m_world->ToggleSectorDataVisibility( m_sectorDataId, false );
	}

	// Inform all entities that we were dettached to the world 
	for ( Uint32 i=0; i<m_entities.Size(); i++ )
	{
		CEntity* entity = m_entities[i];
		if ( entity )
		{
			// Capture data from entity before detaching
			StoreEntityState( entity );

			// Detach entity
			ASSERT( entity->IsAttached() );
			entity->DetachFromWorld( m_world );
			ASSERT( !entity->IsAttached() );

			// Uninitialize entity
			ASSERT( entity->IsInitialized() );
			entity->Uninitialize();
			ASSERT( !entity->IsInitialized() );
		}
	}

	// Remove from list of attached layers
	VERIFY( m_world->m_attachedLayers.Erase( this ) );

	// Mark as not attached
	m_world = NULL;

	EDITOR_DISPATCH_EVENT( CNAME( Detached ), CreateEventData( this ) );
}

void CLayer::GetEntities( TDynArray< CEntity* >& entities ) const
{
	entities.Reserve( entities.Size() + m_entities.Size() );
	for ( Uint32 i=0; i<m_entities.Size(); i++ )
	{
		CEntity* entity = m_entities[i];
		if ( entity )
		{
			entities.PushBack( entity );
		}
	}
}

Bool CLayer::ValidateSpawnInfo( const EntitySpawnInfo& info ) const
{
	// Cannot spawn on non dynamic layer while in game
	// DM: active world may be null if entity is spawned during world initialization (check CEnvironmentManager constructor)
	if ( GGame->IsActive() && GGame->GetActiveWorld().Get() != nullptr )
	{
		if ( GetWorld() && this != GetWorld()->GetDynamicLayer() )
		{
			String txt = info.m_template ? info.m_template->GetFriendlyName() : TXT("Unknown");
			WARN_ENGINE( TXT("Cannot spawn entity '%ls' on non dynamic layer while in game"), txt.AsChar() );
			return false;
		}
	}

	// Make sure IdTag is specified is Dynamic
	if ( info.m_idTag.IsValid() && !info.m_idTag.IsDynamic() )
	{
		String txt = info.m_template ? info.m_template->GetFriendlyName() : TXT("Unknown");
		WARN_ENGINE( TXT("Cannot spawn entity '%ls' with static IdTag using CreateEntitySync"), txt.AsChar()	);
		return false;
	}

	// Managed entities should have valid IdTag
	if ( (info.m_entityFlags & EF_ManagedEntity ) && !info.m_idTag.IsValid() )
	{
		String txt = info.m_template ? info.m_template->GetFriendlyName() : TXT("Unknown");
		WARN_ENGINE( TXT("Cannot spawn managed entity '%ls' without valid IdTag"), txt.AsChar()	);
		return false;
	}

	// Managed entities need to have template specified
	if ( (info.m_entityFlags & EF_ManagedEntity ) && !info.m_template )
	{
		WARN_ENGINE( TXT("Cannot spawn managed entity without a template") );
		return false;
	}

	// Seems OK
	return true;
}

void CLayer::LinkEntityWithLayer( CEntity* entity, const EntitySpawnInfo& spawnInfo )
{
	PC_SCOPE( LinkEntityWithLayer );

	// Inform entity
	entity->OnCreated( this, spawnInfo );

	// Storage support
	CLayerStorage* storage = GetLayerStorage();
	if ( storage && GGame->IsActive() )
	{
		CPeristentEntity* pEntity = Cast< CPeristentEntity >( entity );
		if ( pEntity )
		{
			PC_SCOPE( EntityStateManagment );

			// Add to entity storage
			if ( entity->IsManaged() )
			{	
				storage->RegisterManagedEntity( pEntity );
			}

			// Restore data
			storage->RestoreEntityState( pEntity, this );
		}
	}

	// Register in the entity list
	m_entities.PushBack( entity );

	// Initialize the entity we are adding
	ASSERT( !entity->IsInitialized() );
	entity->Initialize();
	ASSERT( entity->IsInitialized() );

	// If we are already attached to world attach created entity
	if ( IsAttached() )
	{
		ASSERT( !entity->IsAttached() );
		entity->AttachToWorld( m_world );
		ASSERT( entity->IsAttached() );
	}

	// Since we have added an entity to this layer mark it as modified
	if ( GGame->IsActive() == false )
	{
		// Mark layer as modified
		MarkModified();
	}

	// inform
	OnEntityAdded( entity );
}

void CLayer::RestoreEntityState( CEntity* entity )
{
	// Storage support
	CLayerStorage* storage = GetLayerStorage();
	if ( storage && GGame->IsActive() )
	{
		CPeristentEntity* pEntity = Cast< CPeristentEntity >( entity );
		if ( pEntity )
		{
			// Restore data
			storage->RestoreEntityState( pEntity, this );
		}
	}
}

void CLayer::StoreEntityState( CEntity* entity )
{
	CLayerStorage* storage = GetLayerStorage();
	if ( storage && GGame->IsActive() )
	{
		CPeristentEntity* pEntity = Cast< CPeristentEntity >( entity );
		if ( pEntity )
		{
			if ( pEntity->ShouldSave() )
			{
				storage->SaveEntityState( pEntity );
			}
			else
			{
				storage->RemoveEntityState( pEntity );
			}
		}
	}
}

static CEntity* CreateEntityObject( const EntitySpawnInfo& info, CLayer* layer )
{
	CEntity* entity = NULL;

	// Get template
	const THandle< CEntityTemplate > entityTemplate = info.m_template;
	if ( !entityTemplate )
	{
		// Create empty entity shell
		if ( !info.m_entityClass )
		{
			// Default entity
			entity = CreateObject<CEntity>( layer );
		}
		else
		{
			// Create entity from given class
			entity = CreateObject<CEntity>( info.m_entityClass, layer );
		}
	}
	else
	{
		// Setup instancing mode
		EntityTemplateInstancingInfo instanceInfo;
		instanceInfo.m_detachFromTemplate = info.m_detachTemplate;
		instanceInfo.m_previewOnly = info.m_previewOnly;
		instanceInfo.m_entityClass = info.m_entityClass;

		// Use specified template to create entity
		entity = entityTemplate->CreateInstance( layer, instanceInfo );
		if ( !entity )
		{
			WARN_ENGINE( TXT("Unable to spawn '%ls', of template '%ls'"), info.m_name.AsChar(), info.m_template->GetFriendlyName().AsChar() );
		}
	}

	if( info.m_forceNonStreamed )
	{
		// We need the entity to not be attached so it wont fire streaming events and modify its internal state
		// If this assumption changes, the "nonstreamification" will need to change to make sure the streamed components
		// are converted to non-streamed components before the entity is attached to the world
		RED_ASSERT( !entity->IsAttached(), TXT("Trying to force an entity to be non-streamed while it is attached") );
		entity->CreateStreamedComponents( SWN_DoNotNotifyWorld, true, nullptr, false );
		entity->SetStreamed( false );
	}

	// Return created entity object
	return entity;
}

static Bool EditorOnlyEntityHacks( CEntity* entity, const EntitySpawnInfo& info, CLayer* layer )
{
	// If name not given generate one
	String entityName;
	if ( info.m_name.Empty() )
	{
		// Get base entity name to use
		String baseName;
		if ( info.m_resource && info.m_resource->GetFile() )
		{
			// Use resource name as the base entity name, this is usefull for meshes, particles etc
			CFilePath path( info.m_resource->GetDepotPath() );
			baseName = path.GetFileName();
		}
		else if ( info.m_template && info.m_template->GetFile() )
		{
			// Use template name
			CFilePath path( info.m_template->GetFile()->GetDepotPath() );
			baseName = path.GetFileName();
		}
		else
		{
			// Use default name
			baseName = TXT("CEntity");
		}

#ifndef NO_EDITOR_WORLD_SUPPORT
		// Generate automatic name
		entityName = layer->GenerateUniqueEntityName( baseName );
#endif
	}
	else
	{
		// Make sure such entity does not exist
		if ( layer->FindEntity( info.m_name ) )
		{
			WARN_ENGINE( TXT("Duplicated entity name '%ls'"), info.m_name.AsChar() );
			return false;
		}

		// Use the name given
		entityName = info.m_name;
	}

	// Change entity name
	entity->SetName( entityName );

	// Try to use given resource
	if ( info.m_resource )
	{ 
		entity->CreateStreamedComponents( SWN_NotifyWorld );
#ifndef NO_EDITOR
		entity->ForceFinishAsyncResourceLoads();
#endif
		const TDynArray< CComponent * > components = entity->GetComponents();
		TDynArray< CComponent * >::const_iterator currComp = components.Begin(), lastComp = components.End();
		for ( ; currComp != lastComp; ++currComp )
		{
			(*currComp)->TryUsingResource( info.m_resource );
		}

		// Setup streaming LOD
		if ( info.m_detachTemplate )
		{
			CMeshTypeComponent* meshComp = entity->FindComponent<CMeshTypeComponent>();
			if ( meshComp )
			{
				meshComp->SetStreamed( true );
			}
		}
#ifndef NO_EDITOR
		entity->UpdateStreamedComponentDataBuffers();
		entity->UpdateStreamingDistance();
#endif
		entity->DestroyStreamedComponents( SWN_NotifyWorld );
	}

	// Keep the entity
	return true;
}

CJobSpawnEntity* CLayer::CreateEntityAsync( EntitySpawnInfo&& info )
{
	// We can spawn only on dynamic layer
	if ( !GGame || !GGame->IsActive() || !GGame->GetActiveWorld() ) //|| (GGame->GetActiveWorld()->GetDynamicLayer() != this ) )
	{
		WARN_ENGINE( TXT("CreateEntityAsync can be used only on dynamic layer during the game") );
		return NULL;
	}

	// Validate
	if ( !ValidateSpawnInfo( info ) )
	{
		return NULL;
	}

	// Create and start job
	CJobSpawnEntity* job = new CJobSpawnEntity( this, Move( info ) );
	SJobManager::GetInstance().Issue( job );

	// Done
	return job;
}

CJobSpawnEntityList* CLayer::CreateEntityListAsync( TDynArray< EntitySpawnInfo >&& infos )
{
	// We can spawn only on dynamic layer
	if ( !GGame || !GGame->IsActive() || !GGame->GetActiveWorld() ) //|| (GGame->GetActiveWorld()->GetDynamicLayer() != this ) )
	{
		WARN_ENGINE( TXT("CreateEntityAsync can be used only on dynamic layer during the game") );
		return NULL;
	}

	for ( auto it = infos.Begin(), end = infos.End(); it != end; ++it )
	{
		if ( !ValidateSpawnInfo( (*it) ) )
		{
			return NULL;
		}
	}

	CJobSpawnEntityList* job = new CJobSpawnEntityList( this, Move( infos ) );
	SJobManager::GetInstance().Issue( job );

	return job;
}

CEntity* CLayer::CreateEntitySync( const EntitySpawnInfo& info )
{
	// Validate
	if ( !ValidateSpawnInfo( info ) )
	{
		return NULL;
	}

	// Create initial entity object
	CEntity* entity = CreateEntityObject( info, this );
	if ( !entity )
	{
		return NULL;
	}

	entity->OnCreatedAsync( info );
	//entity->ApplyMeshComponentColoring();

	// Some editor only entity hacks
	if ( !EditorOnlyEntityHacks( entity, info, this ) )
	{
		entity->Discard();
		return NULL;
	}

	// Link entity in the layer
	LinkEntityWithLayer( entity, info );

	info.OnPostAttach( entity );
	
	// Apply initial appearance
	CAppearanceComponent* appearanceComponent = CAppearanceComponent::GetAppearanceComponent( entity );
	if ( appearanceComponent )
	{
		appearanceComponent->ApplyInitialAppearance( info );
	}

	// Entity is spawned, added to layer and ready to use
	return entity;
}

void CLayer::DestroyAllEntities()
{
	// Move entities into temporary array (and empty m_entities - this should be okay)

	LayerEntitiesArray entities( Move( m_entities ) );

	// Destroy all entities

	for ( Uint32 i=0; i<entities.Size(); ++i )
	{
		CEntity* entity = entities[i];
		if ( entity )
		{
			entity->Destroy();
		}
	}
}

void CLayer::DestroyEntity( CEntity* entity )
{
	RED_ASSERT( SIsMainThread() );
	ASSERT( entity );
	ASSERT( entity->GetLayer() == this );

	// Do not destroy already destroyed entities
	if ( !entity->IsDestroyed() )
	{
		// Inform
		OnEntityRemoved( entity );

		// Mark entity as destroyed
		entity->OnDestroyed( this );
		RED_ASSERT( entity->IsDestroyed(), TXT("OnDestroyed() not propagated correctly for entity '%ls'"), entity->GetName().AsChar() );

		// Capture entity state just before it's removed, only for not managed entities
		if ( !entity->IsManaged() )
		{	
			StoreEntityState( entity );
		}

		// Cleanup entity mess
		entity->DestroyEntityInternals();

		// Destroy all components from entity
		entity->DestroyAllComponents();

		CWorld* world = entity->GetLayer()->GetWorld();

		// Detach
		if ( entity->IsAttached() )
		{
			ASSERT( entity->IsAttached() );
			entity->DetachFromWorld( world );
			ASSERT( !entity->IsAttached() );
		}

		// Uninitialize entity after it's detached
		ASSERT( entity->IsInitialized() );
		entity->Uninitialize();
		ASSERT( !entity->IsInitialized() );

		// Remove entity from layer list of entities
		m_entities.Remove( entity );

		// Remove managed entity
		if ( entity->IsManaged() )
		{	
			CLayerStorage* storage = GetLayerStorage();
			if ( storage && GGame->IsActive() )
			{
				CPeristentEntity* pEntity = Cast< CPeristentEntity >( entity );
				if ( pEntity )
				{
					storage->UnregisterManagedEntity( pEntity );
				}
			}
		}

		// Dispatch system wide event
		EDITOR_DISPATCH_EVENT( CNAME( Destroyed ), CreateEventData( entity ) );

		// Delete the entity object
		entity->Discard();
	}
}

void CLayer::AddEntity( CEntity* entity )
{
	ASSERT( entity->GetLayer() == NULL );

	// Update entity parent
	entity->SetParent( this );

	// Register in the entity list
	m_entities.PushBack( entity );

	// Initialize entity before attaching
	ASSERT( !entity->IsInitialized() );
	entity->Initialize();
	ASSERT( entity->IsInitialized() );

	// If we are already attached to world attach created entity
	ASSERT( !entity->IsAttached() );
	entity->AttachToWorld( m_world );
	ASSERT( entity->IsAttached() );

	// Since we have added an entity to this layer mark it as modified
	if ( GGame->IsActive() == false )
	{
		// Mark layer as modified
		MarkModified();
	}

	// inform
	OnEntityAdded( entity );
}
#ifndef NO_EDITOR
void CLayer::ForceAddEntity( CEntity* entity )
{
	// Update entity parent
	entity->SetParent( this );

	// Register in the entity list
	m_entities.PushBack( entity );
}
#endif // NO_EDITOR
void CLayer::RemoveEntity( CEntity* entity )
{
	ASSERT( entity );
	ASSERT( entity->GetLayer() == this );

	// Do not destroy already destroyed entities
	if ( !entity->IsDestroyed() )
	{
		// Inform
		OnEntityRemoved( entity );

		// Destroy internals
		entity->DestroyEntityInternals();

		CWorld* world = entity->GetLayer()->GetWorld();

		// Detach
		if ( entity->IsAttached() )
		{
			ASSERT( entity->IsAttached() );
			entity->DetachFromWorld( world );
			ASSERT( !entity->IsAttached() );
		}

		// Uninitialize entity after it was removed
		ASSERT( entity->IsInitialized() );
		entity->Uninitialize();
		ASSERT( !entity->IsInitialized() );

		// Remove entity from layer list of entities
		m_entities.Remove( entity );

		entity->SetParent( NULL );
	}
}

void CLayer::NotifyUnloading()
{
	for ( Uint32 i=0; i<m_entities.Size(); ++i )
	{
		CEntity* entity = m_entities[i];
		if ( entity )
		{
			entity->OnLayerUnloading();
		}
	}
}

CEntity* CLayer::FindEntity( const String& name ) const
{
	// Linear search by name
	LayerEntitiesArray::const_iterator	entCurr = m_entities.Begin();
	LayerEntitiesArray::const_iterator	entLast = m_entities.End();
	for ( ; entCurr != entLast; ++entCurr )
	{
		CEntity* entity = *entCurr;
		if ( entity && entity->GetName() == name )
		{
			return entity;
		}
	}

	// Not found
	return NULL;
}

CEntity* CLayer::FindEntity( const CGUID& guid ) const
{
	// Linear search by GUID
	LayerEntitiesArray::const_iterator	entCurr = m_entities.Begin();
	LayerEntitiesArray::const_iterator	entLast = m_entities.End();
	for ( ; entCurr != entLast; ++entCurr )
	{
		CEntity* entity = *entCurr;

		if ( entity && entity->GetGUID() == guid )
		{
			return entity;
		}
	}

	// Not found	
	return NULL;
}

Bool CLayer::MoveEntity( CEntity* entity, CLayer *layer )
{
	if( entity->GetParent() == this )
	{
		// Inform
		OnEntityRemoved( entity );

		// Remove from this layer
		m_entities.Remove( entity );

		// Add to new layer
		layer->m_entities.PushBack( entity );

		// Detach when hiding (new layer is hidden and old layer is visible)
		if ( m_layerInfo->IsVisible() && !layer->m_layerInfo->IsVisible() )
		{
			ASSERT( entity->IsAttached() );
			entity->OnDetached( m_world );
			ASSERT( !entity->IsAttached() );
		}

		// Update old layer attached components count
		m_numAttached -= entity->GetComponents().Size();
		ASSERT( m_numAttached >= 0 && TXT("Ups...") );

		// Reparent
		entity->SetParent( layer );

		// Attach when showing (old layer is hidden and new layer is visible)
		if ( !m_layerInfo->IsVisible() && layer->m_layerInfo->IsVisible() )
		{
			ASSERT( !entity->IsAttached() );
			entity->AttachToWorld( layer->m_world );
			ASSERT( entity->IsAttached() );
		}
		else // else as AttachToWorld() will update attached components count
		{
			// Update new layer attached components count
			layer->m_numAttached += entity->GetComponents().Size();
		}

		// Mark both layers as modified
		MarkModified();
		layer->MarkModified();

		// inform
		layer->OnEntityAdded( entity );
		
		// Update layers
		EDITOR_QUEUE_EVENT( CNAME( LayerChildrenChanged ), CreateEventData( m_layerInfo ) );
		EDITOR_QUEUE_EVENT( CNAME( LayerChildrenChanged ), CreateEventData( layer->m_layerInfo ) );

		return true;
	}

	// Not moved
	return false;
}

String CLayer::GenerateUniqueEntityName( const String& base )
{
	String pureName = base;
	Int32 initialIndex = -1;

	// If entity name ends with a number extract it and use as initial increment
	Int32 firstNumberChar = -1;
	for ( Int32 i=pureName.GetLength()-1; i>=0; i-- )
	{
		if ( pureName[i] >= '0' && pureName[i] <= '9' )
		{
			firstNumberChar = i;
		}
		else
		{
			break;
		}
	}

	// Numbered name
	if ( firstNumberChar != -1 )
	{
		pureName = base.LeftString( firstNumberChar );
		initialIndex = atoi( UNICODE_TO_ANSI( &base[ firstNumberChar ] ) );
		m_nameCount = initialIndex + 1; 
	}

	// TODO: Optimize !
	for ( ; ; m_nameCount++ )
	{
		// Assemble name
		Char name[ 256 ];
		Red::System::SNPrintF( name, ARRAY_COUNT(name), TXT("%s%i"), pureName.AsChar(), m_nameCount );

		// If not used take it
		if ( !FindEntity( name ) )
		{
			return name;
		}
	}
}

//FIXME>>>>> WTF?
void CLayer::GenerateUniqueEntityNameCheap( CEntity* entity, const String& base, String& outStr )
{
#ifndef RED_PLATFORM_ORBIS
	Char addressStr[9] = { 0 };	
	_ultow_s( static_cast< Uint32 >( reinterpret_cast< uintptr_t >( entity ) ), addressStr, ARRAY_COUNT( addressStr ), 16 );

	outStr = base;
	outStr.Append( addressStr, Red::System::StringLength( addressStr ) );
#endif
}

Bool CLayer::PasteSerializedEntities( const TDynArray< Uint8 >& data, LayerEntitiesArray& pastedEntities, Bool relativeSpawn, const Vector& spawnPosition, const EulerAngles& spawnRotation )
{
	// Deserialize
	TDynArray< CObject* > objects;
	CMemoryFileReader reader( data, 0 );
	CDependencyLoader loader( reader, NULL );
	DependencyLoadingContext loadingContext;
	loadingContext.m_parent = this;
	if ( !loader.LoadObjects( loadingContext ) )
	{
		// No objects loaded
		return false;
	}

	// Get spawned entities
	LayerEntitiesArray entities;
	for ( Uint32 i=0; i<loadingContext.m_loadedRootObjects.Size(); i++ )
	{
		CEntity* object = Cast< CEntity >( loadingContext.m_loadedRootObjects[i] );
		if ( object )
		{
			entities.PushBack( object );
		}
	}

	// Call post load of spawned objects
	loader.PostLoad();

	// Offset root components by specified spawn point
	if ( relativeSpawn )
	{
		// No explicit pivot, try using root transform component
		CEntity* localPivot = entities[0];
		ASSERT( localPivot );
		Vector localPivotPos = localPivot->GetWorldPosition();

		// Transform components
		for ( Uint32 i=0; i<entities.Size(); i++ )
		{
			// Set initial position
			CEntity* entity = entities[i];
			entity->SetPosition( spawnPosition + entity->GetWorldPosition() - localPivotPos );

			// Force transform update
			entity->ForceUpdateTransformNodeAndCommitChanges();
			entity->ForceUpdateBoundsNode();
		}
	}

	// Register in the entity list
	m_entities.PushBack( entities );

	// Generate new entities names and bind entities and components to new layer
	for ( Uint32 i=0; i<entities.Size(); i++ )
	{
		CEntity* entity = entities[i];
		entity->SetName( GenerateUniqueEntityName( entity->GetName() ) );
	}

	// Inform entities that they have been pasted
	for ( Uint32 i=0; i<entities.Size(); i++ )
	{
		entities[i]->OnPasted( this );
	}

	// If we are already attached to world attach created entity
	if ( m_world )
	{
		for ( Uint32 i=0; i<entities.Size(); i++ )
		{
			// Initialize entity
			ASSERT( !entities[i]->IsInitialized() );
			entities[i]->Initialize();
			ASSERT( entities[i]->IsInitialized() );

			// Attach to world
			ASSERT( !entities[i]->IsAttached() );
			entities[i]->AttachToWorld( m_world );
			ASSERT( entities[i]->IsAttached() );
		}
	}

	pastedEntities = entities;

	// Inform
	for ( Uint32 i=0; i<entities.Size(); i++ )
	{
		OnEntityAdded( entities[ i ] );
	}

	// Done
	return true;
}

#ifndef NO_FILE_SOURCE_CONTROL_SUPPORT

Bool CLayer::Reload( Bool confirm )
{
	if ( GGame && m_world == GGame->GetActiveWorld() && GetReloadPriority() )
	{
		if ( confirm )
		{
			EDITOR_QUEUE_EVENT( CNAME( FileReloadConfirm ), CreateEventData( this ) );
			return true;
		}
		else
		{
			// Mark as unmodified
			GetFile()->Unmodify();

			// Reload
			Bool ret = true;
			CLayerInfo* info = m_layerInfo;
			ret &= info->SyncUnload();
			LayerLoadingContext context;
			ret &= info->SyncLoad( context );
			return ret;
		}
	}

	return false;
}

#endif

struct EntityRecreateData
{
	CEntity*								m_entity;
	EntitySpawnInfo							m_spawnInfo;
	CScriptSnapshot::ScriptableSnapshot*	m_snapshot;
};

#ifndef NO_EDITOR_EVENT_SYSTEM
void CLayer::DispatchEditorEvent( const CName& name, IEdEventData* data )
{
	if ( name == CNAME( EntityTemplateChanged ) && GGame && m_world == GGame->GetActiveWorld() )
	{
		CEntityTemplate* entTemplate = GetEventData< CEntityTemplate* >( data );
		RecreateEntieties( entTemplate );
	}
}
#endif

void CLayer::RecreateEntieties( CEntityTemplate* entTemplate )
{
	const Uint32 numberOfEntieties = m_entities.Size();

	TDynArray< CObject* > entitiesToRecreate;
	for ( Uint32 i=0; i<numberOfEntieties; i++ )
	{
		CEntity* entity = m_entities[i];
		if ( entity->GetEntityTemplate() == entTemplate )
		{	
			entitiesToRecreate.PushBack( entity );
		}
	}

	// no entities to recreate
	if ( entitiesToRecreate.Empty() )
	{
		return;
	}

	// Serialize to memory package
	TDynArray< Uint8 > buffer;
	CMemoryFileWriter writer( buffer );

	// Serialize
	DependencySavingContext context( entitiesToRecreate );
	CDependencySaver saver( writer, NULL );
	if ( !saver.SaveObjects( context ) )
	{
		WARN_ENGINE( TXT("Unable to save objects to memory") );
		return;
	}
	
	// Destroy entities
	for ( Uint32 i=0; i<entitiesToRecreate.Size(); i++ )
	{
		CEntity* entity = Cast< CEntity >( entitiesToRecreate[i] );
		entity->Destroy();
	}

	// Deserialize
	TDynArray< CObject* > objects;
	CMemoryFileReader reader( buffer, 0 );
	CDependencyLoader loader( reader, NULL );
	DependencyLoadingContext loadingContext;
	loadingContext.m_parent = this;
	if ( !loader.LoadObjects( loadingContext ) )
	{
		// No objects loaded
		return;
	}

	// Get spawned entities
	LayerEntitiesArray entities;
	for ( Uint32 i=0; i<loadingContext.m_loadedRootObjects.Size(); i++ )
	{
		CEntity* object = Cast< CEntity >( loadingContext.m_loadedRootObjects[i] );
		if ( object )
		{
			entities.PushBack( object );
		}
	}

	// Register in the entity list
	m_entities.PushBack( entities );

	// If we are already attached to world attach created entity
	if ( m_world )
	{
		for ( Uint32 i=0; i<entities.Size(); i++ )
		{
			ASSERT( !entities[i]->IsInitialized() );
			entities[i]->Initialize();
			ASSERT( entities[i]->IsInitialized() );

			ASSERT( !entities[i]->IsAttached() );
			entities[i]->AttachToWorld( m_world );
			ASSERT( entities[i]->IsAttached() );
		}

		// Update streaming on next frame
		m_world->RequestStreamingUpdate();
	}

	// I don't know how, I don't know why....
	// But it fixes the problem of disappearing entities!
	for( Uint32 i = 0; i < entities.Size(); ++i )
	{
		entities[ i ]->ForceUpdateTransformNodeAndCommitChanges();

		// Inform
		OnEntityAdded( entities[ i ] );
	}

	ASSERT( numberOfEntieties == m_entities.Size() );
}

void CLayer::AddClonedEntity( CEntity *entity )
{
	if ( entity )
	{
		// Add to list
		ASSERT( !m_entities.Exist( entity ) );
		m_entities.PushBack( entity );

		// Make sure parent is valid
		ASSERT( entity->GetParent() == this );

		// attach entity
		ASSERT( !entity->IsAttached() );
		entity->AttachToWorld( m_world );
		ASSERT( entity->IsAttached() );

		// inform
		OnEntityAdded( entity );
	}
}

void CLayer::RemoveShadowsFromLayer()
{
	if ( CanModify() )
	{
		Bool anyModified = false;
		for ( Uint32 i=0; i<m_entities.Size(); i++ )
		{
			CEntity* entity = m_entities[i];
			if ( entity && entity->GetTemplate()==NULL )
			{
				for ( ComponentIterator< CMeshTypeComponent > it( entity ); it; ++it )
				{
					CMeshTypeComponent* mc = *it;
					if ( mc->IsCastingShadows() )
					{
						mc->SetCastingShadows( false );
						anyModified = true;
					}
				}

				// so why destruction component is not a meshtype component??
				for ( ComponentIterator< CDestructionSystemComponent > it( entity ); it; ++it )
				{
					CDestructionSystemComponent* dc = *it;					
					CDrawableComponent* dcmp = Cast< CDestructionSystemComponent >( dc );

					if( dcmp && !dcmp->IsCastingShadows() )
					{
						dcmp->SetCastingShadows( false );
						anyModified = true;
					}		
				}
			}
		}

		if ( anyModified )
		{
			MarkModified();
		}
	}
}

void CLayer::AddShadowsToLayer()
{	
	if ( CanModify() )
	{
		Bool anyModified = false;
		for ( Uint32 i=0; i<m_entities.Size(); i++ )
		{
			CEntity* entity = m_entities[i];
			if ( entity && entity->GetTemplate()==NULL )
			{
				for ( ComponentIterator< CMeshTypeComponent > it( entity ); it; ++it )
				{
					CMeshTypeComponent* mc = *it;
					if ( !mc->IsCastingShadows() )
					{
						mc->SetCastingShadows( true );
						anyModified = true;
					}
				}

				// so why destruction component is not a meshtype component??
				for ( ComponentIterator< CDestructionSystemComponent > it( entity ); it; ++it )
				{
					CDestructionSystemComponent* dc = *it;					
					CDrawableComponent* dcmp = Cast< CDestructionSystemComponent >( dc );

					if( dcmp && !dcmp->IsCastingShadows() )
					{
						dcmp->SetCastingShadows( true );
						anyModified = true;
					}		
				}
			}
		}

		if ( anyModified )
		{
			MarkModified();
		}
	}

}

void CLayer::AddShadowsFromLocalLightsToLayer()
{
	if ( CanModify() )
	{
		Bool anyModified = false;
		for ( Uint32 i=0; i<m_entities.Size(); i++ )
		{
			CEntity* entity = m_entities[i];
			if ( entity && entity->GetTemplate()==NULL )
			{					
				for ( ComponentIterator< CMeshTypeComponent > it( entity ); it; ++it )
				{
					CMeshTypeComponent* mc = *it;
					if( !mc->IsCastingShadowsFromLocalLightsOnly() )
					{
						mc->SetCastingShadows( false, true );
						anyModified = true;
					}					
				}

				// so why destruction component is not a meshtype component??
				for ( ComponentIterator< CDestructionSystemComponent > it( entity ); it; ++it )
				{
					CDestructionSystemComponent* dc = *it;					
					CDrawableComponent* dcmp = Cast< CDestructionSystemComponent >( dc );

					if( dcmp && !dcmp->IsCastingShadowsFromLocalLightsOnly() )
					{
						dcmp->SetCastingShadows( false, true );
						anyModified = true;
					}					
				}
			}
		}

		if ( anyModified )
		{
			MarkModified();
		}
	}
}
	

const CGUID& CLayer::GetGUID() const
{
	if ( m_layerInfo )
	{
		return m_layerInfo->GetGUID();
	}
	else
	{
		return CGUID::ZERO;
	}
}

CBrushCompiledData* CLayer::CreateBrushCompiledData()
{
#if 0
	// Use existing if we have one
	if ( m_brushCompiledData )
	{
		return m_brushCompiledData;
	}

	// We should be able to modify this layer
	if ( CanModify() )
	{
		// Create the brush list
		m_brushCompiledData = ::CreateObject< CBrushCompiledData >( this );

		// Attach to world if layer is attached
		if ( IsAttached() )
		{
			ASSERT( m_world );
			m_brushCompiledData->AttachedToWorld( m_world );
		}

		// Return created data
		MarkModified();
		return m_brushCompiledData;
	}
#endif
	// Not created
	return NULL;
}

CLayerStorage* CLayer::GetLayerStorage()
{
	// Use the layer storage from the layer info for static layers
	if ( m_layerInfo )
	{
		return m_layerInfo->GetLayerStorage();
	}

	// No implicit layer storage
	return NULL;
}

void CLayer::CollectResourceUsage( class IResourceUsageCollector& collector ) const
{
	if ( m_layerInfo )
	{
		collector.PushLayer( m_layerInfo->GetDepotPath() );

		switch ( m_layerInfo->GetLayerType() )
		{
			case LT_NonStatic: collector.ReportLayerFlag( CName( TXT("nonstatic" ) ), true ); break;
			case LT_AutoStatic: collector.ReportLayerFlag( CName( TXT("autostatic" ) ), true ); break;
		}

		switch ( m_layerInfo->GetLayerBuildTag() )
		{
			case LBT_EnvOutdoor: collector.ReportLayerFlag( CName( TXT("outdoor" ) ), true ); break;
			case LBT_EnvIndoor: collector.ReportLayerFlag( CName( TXT("indoor" ) ), true ); break;
			case LBT_Quest: collector.ReportLayerFlag( CName( TXT("quest" ) ), true ); break;
			case LBT_Communities: collector.ReportLayerFlag( CName( TXT("community" ) ), true ); break;
		}

		for ( Uint32 i=0; i<m_entities.Size(); ++i )
		{
			CEntity* entity = m_entities[i];
			if ( entity )
			{
				collector.PushEntity( entity->GetName(), entity->GetClass()->GetName(), entity->GetWorldPositionRef() );
				entity->CollectResourceUsage( collector );
				collector.PopEntity();
			}
		}

		collector.PopLayer();
	}
}

Uint32 CLayer::CalcDataSize() const
{
	return CObjectMemoryAnalizer::CalcObjectSize( const_cast< CLayer* >( this ) );
}

void CLayer::CalcMemSnapshot( SLayerMemorySnapshot& snapshot ) const
{
	snapshot.m_name = m_layerInfo ? m_layerInfo->GetShortName() : GetFriendlyName();

	snapshot.m_entitiesNum = m_entities.Size();
	snapshot.m_attachedCompNum = m_numAttached;

	snapshot.m_memStatic = GetClass()->GetSize();
	snapshot.m_memSerialize = CObjectMemoryAnalizer::CalcObjectSize( const_cast< CLayer* >( this ) );
	snapshot.m_memDynamic = CalcObjectDynamicDataSize();
}

#ifndef NO_EDITOR

void CLayer::ConvertToStreamed(bool templatesOnly)
{
	RED_HALT( "You should not call this" );

#if 0
	CWorld* world = GGame->GetActiveWorld();

	if ( !world )
	{
		return;
	}

	const LayerEntitiesArray& entities = m_entities;
	for ( Uint32 i = 0; i < entities.Size(); ++i )
	{
		Bool hasVisuals;
		Bool hasLogic;
		CEntity* ent = entities[i];

		if ( !ent )
		{
			continue;
		}

		ent->CheckEntityStreamingCategory( hasVisuals, hasLogic );

		if ( hasVisuals )
		{
			CEntityTemplate *templ = ent->GetEntityTemplate();
			if ( templ )
			{
				EntityTemplateInstancingInfo eInfo;
				eInfo.m_async = false;
				eInfo.m_detachFromTemplate = false;
				eInfo.m_previewOnly = false;

				CEntity* newEnt = templ->CreateInstance( NULL, eInfo );
				newEnt->IncludeAllStreamedComponents();

				CMeshComponent* meshComp = NULL;
				for ( ComponentIterator< CMeshComponent > it( newEnt ); it; ++it )
				{
					meshComp = static_cast<CMeshComponent*>( *it );
					Int8 lod = meshComp->GetMeshNow() ? CWorld::SuggestStreamingLODFromBox( meshComp->GetMeshNow()->GetBoundingBox() ) : -1;
					meshComp->SetStreamingLOD( lod );
				}
				
				newEnt->DetachTemplate();
				templ->CaptureData( newEnt );

				if ( templ->MarkModified() )
				{
					templ->Save();
				}
				ent->CreateAllStreamedComponents();
			}
			else
			{
				ent->CreateAllStreamedComponents();
				CMeshComponent* meshComp = ent->FindComponent<CMeshComponent>();
				if ( meshComp )
				{
					CMesh* mesh = meshComp->GetMeshNow();
					if ( mesh )
					{
						Int8 lod = CWorld::SuggestStreamingLODFromBox( mesh->GetBoundingBox() );
						meshComp->SetStreamingLOD( lod );
					}
				}
			}
		}
	}
#endif
}

Bool CLayer::IsCommunityLayer()
{
	return GetLayerInfo() && GetLayerInfo()->GetLayerBuildTag() == LBT_Communities;
}
#endif

void CLayer::OnEntityAdded( const CEntity* entity )
{
}

void CLayer::OnEntityRemoved( const CEntity* entity )
{
}

