/**
* Copyright (c) 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "../../common/core/depot.h"
#include "../../common/core/bundledefinition.h"
#include "../../common/core/garbageCollector.h"
#include "../../common/core/objectDiscardList.h"
#include "../../common/engine/layerInfo.h"
#include "../../common/engine/layerGroup.h"
#include "../../common/engine/layer.h"
#include "../../common/engine/world.h"
#include "../../common/engine/mesh.h"
#include "resourceUsageCommandlet.h"
#include "resourceUsageDataBase.h"

IMPLEMENT_ENGINE_CLASS( CResourceUsageCommandlet )

///---

CResourceUsageCommandlet::Settings::Settings()
{
}

bool CResourceUsageCommandlet::Settings::Parse( const CommandletOptions& options )
{
	// parse world path
	if ( !options.GetSingleOptionValue( TXT("world"), m_worldPath ) )
	{
		ERR_WCC( TXT("World path must be speicifed") );
		return false;
	}

	// parse output path
	if ( !options.GetSingleOptionValue( TXT("out"), m_outPath ) )
	{
		ERR_WCC( TXT("Output data base path must be speicifed") );
		return false;
	}

	return true;
}

///---

class CResourceUsageDataBaseCollector : public IResourceUsageCollector
{
public:
	CResourceUsageDataBaseCollector( CResourceUsageDataBase* db )
		: m_db( db )
	{
		Reset();
	}

	void Reset();

	// interface
	virtual void PushLayer( const String& layerName ) override;
	virtual void PopLayer() override;
	virtual void PushEntity( const String& entityName, const CName entityClass, const Vector& position ) override;
	virtual void PopEntity() override;
	virtual void PushComponent( const String& componentName, const CName componentClass, const Vector& position ) override;
	virtual void PopComponent() override;
	virtual void ReportTemplate( const String& templatePath ) override;
	virtual void ReportLayerFlag( const CName flagName, const Bool flagValue ) override;
	virtual void ReportEntityFlag( const CName flagName, const Bool flagValue ) override;
	virtual void ReportComponentFlag( const CName flagName, const Bool flagValue ) override;
	virtual void ReportBoundingBox( const Box& box ) override;
	virtual void ReportVisibilityDistance( const Float minRange=0.0f, const Float maxRange=FLT_MAX ) override;
	virtual void ReportResourceUsage( class CResource* res ) override;
	virtual void ReportResourceUsage( const String& depotPath ) override;

private:
	typedef TDynArray< CName > TFlags;
	typedef THashMap< String, Bool > TValidResources;

	CResourceUsageDataBase*		m_db;
	TValidResources				m_resources;	

	CName						m_layerName;
	TFlags						m_layerFlags;

	CName						m_entityName;
	CName						m_entityClass;
	TFlags						m_entityFlags;
	Box							m_entityBox;
	Vector						m_entityPosition;
	Bool						m_entityHasBox;
	Float						m_entityVisDistanceMin;
	Float						m_entityVisDistanceMax;
	Bool						m_entityHasVisDistance;
	StringAnsi					m_entityTemplate;

	CName						m_componentName;
	CName						m_componentClass;
	Vector						m_componentPosition;
	TFlags						m_componentFlags;
	Box							m_componentBox;
	Bool						m_componentHasBox;
	Float						m_componentVisDistanceMin;
	Float						m_componentVisDistanceMax;
	Bool						m_componentHasVisDistance;


	Bool GrabResourceData( const String& depotPath );
};

void CResourceUsageDataBaseCollector::Reset()
{
	m_layerName = CName::NONE;
	m_layerFlags.ClearFast();

	m_entityName = CName::NONE;
	m_entityClass = CName::NONE;
	m_entityFlags.ClearFast();
	m_entityBox = Box::EMPTY;
	m_entityHasBox = false;
	m_entityVisDistanceMin = 0.0f;
	m_entityVisDistanceMax = FLT_MAX;
	m_entityHasVisDistance = false;

	m_componentName = CName::NONE;
	m_componentClass = CName::NONE;
	m_componentFlags.ClearFast();
	m_componentBox = Box::EMPTY;
	m_componentHasBox = false;
	m_componentVisDistanceMin = 0.0f;
	m_componentVisDistanceMax = FLT_MAX;
	m_componentHasVisDistance = false;	
}

void CResourceUsageDataBaseCollector::PushLayer( const String& layerName )
{
	RED_ASSERT( m_layerName.Empty() );
	m_layerName = CName( layerName.AsChar() );
	m_layerFlags.ClearFast();
}

void CResourceUsageDataBaseCollector::PopLayer()
{
	RED_ASSERT( !m_layerName.Empty() );
	m_layerName = CName::NONE;
}

void CResourceUsageDataBaseCollector::PushEntity( const String& entityName, const CName entityClass, const Vector& position )
{
	RED_ASSERT( m_entityName.Empty() );
	m_entityName = entityName.Empty() ? CName( TXT("UnknownEntity") ): CName( entityName.AsChar() );
	m_entityClass = entityClass;
	m_entityPosition = position;
	m_entityFlags.ClearFast();
	m_entityBox = Box::EMPTY;
	m_entityHasBox = false;
	m_entityVisDistanceMin = 0.0f;
	m_entityVisDistanceMax = FLT_MAX;
	m_entityHasVisDistance = false;
	m_entityTemplate = StringAnsi::EMPTY;
}

void CResourceUsageDataBaseCollector::PopEntity()
{
	RED_ASSERT( !m_entityName.Empty() );
	m_entityName = CName::NONE;
	m_entityClass = CName::NONE;
	m_entityFlags.ClearFast();
	m_entityBox = Box::EMPTY;
	m_entityHasBox = false;
	m_entityVisDistanceMin = 0.0f;
	m_entityVisDistanceMax = FLT_MAX;
	m_entityHasVisDistance = false;
	m_entityTemplate = StringAnsi::EMPTY;
}

void CResourceUsageDataBaseCollector::PushComponent( const String& componentName, const CName componentClass, const Vector& position )
{
	RED_ASSERT( m_componentName.Empty() );
	m_componentName = componentName.Empty() ? CName( TXT("UnknownComponent") ) : CName( componentName.AsChar() );
	m_componentClass = componentClass;
	m_componentPosition = position;
	m_componentFlags.ClearFast();
	m_componentBox = Box::EMPTY;
	m_componentHasBox = false;
	m_componentVisDistanceMin = 0.0f;
	m_componentVisDistanceMax = FLT_MAX;
	m_componentHasVisDistance = false;	
}

void CResourceUsageDataBaseCollector::PopComponent()
{
	RED_ASSERT( !m_componentName.Empty() );
	m_componentName = CName::NONE;
	m_componentClass = CName::NONE;
	m_componentPosition = Vector::ZEROS;
	m_componentFlags.ClearFast();
	m_componentBox = Box::EMPTY;
	m_componentHasBox = false;
	m_componentVisDistanceMin = 0.0f;
	m_componentVisDistanceMax = FLT_MAX;
	m_componentHasVisDistance = false;	
}

void CResourceUsageDataBaseCollector::ReportLayerFlag( const CName flagName, const Bool flagValue )
{
	if ( flagValue )
	{
		m_layerFlags.PushBackUnique( flagName );
	}
}

void CResourceUsageDataBaseCollector::ReportEntityFlag( const CName flagName, const Bool flagValue )
{
	if ( flagValue )
	{
		m_entityFlags.PushBackUnique( flagName );
	}
}

void CResourceUsageDataBaseCollector::ReportComponentFlag( const CName flagName, const Bool flagValue )
{
	if ( flagValue )
	{
		m_componentFlags.PushBackUnique( flagName );
	}
}

void CResourceUsageDataBaseCollector::ReportBoundingBox( const Box& box )
{
	if ( !box.IsEmpty() )
	{
		if ( !m_componentName.Empty() )
		{
			m_componentBox = box;
			m_componentHasBox = true;
		}
		else if ( !m_entityName.Empty() )
		{
			m_entityBox = box;
			m_entityHasBox = true;
		}
	}
}

void CResourceUsageDataBaseCollector::ReportTemplate( const String& templatePath )
{
	m_entityTemplate = UNICODE_TO_ANSI( templatePath.AsChar() );
}

void CResourceUsageDataBaseCollector::ReportVisibilityDistance( const Float minRange/*=0.0f*/, const Float maxRange/*=FLT_MAX*/ )
{
	if ( !m_componentName.Empty() )
	{
		m_componentHasVisDistance = true;
		m_componentVisDistanceMin = minRange;
		m_componentVisDistanceMax = maxRange;
	}
	else if ( !m_entityName.Empty() )
	{
		m_entityHasVisDistance = true;
		m_entityVisDistanceMin = minRange;
		m_entityVisDistanceMax = maxRange;
	}
}

void CResourceUsageDataBaseCollector::ReportResourceUsage( class CResource* res )
{
	if ( res )
		ReportResourceUsage( res->GetDepotPath() );
}

namespace
{
	CResourceUsageEntry::TFlags CopyFlags( const TDynArray< CName >& flags )
	{
		CResourceUsageEntry::TFlags ret;

		for ( CName name : flags )
		{
			if ( ret.Size() < ret.Capacity() )
				ret.PushBack( name );
		}

		return ret;
	}
}

Bool CResourceUsageDataBaseCollector::GrabResourceData( const String& depotPath )
{
	// load resource
	THandle< CResource > res = GDepot->LoadResource( depotPath );
	if ( !res )
		return false;

	// mesh ?
	if ( res->IsA< CMesh >() )
	{
		const CMesh* mesh = static_cast< CMesh* >( res.Get() );

		// auto hide distance
		const Float autoHideDistance = mesh->GetAutoHideDistance();

		// process the LODs
		const Uint32 numLODs = mesh->GetNumLODLevels();
		Float startDistnace = 0.0f;

		// process LODs
		for ( Uint32 i=0; i<numLODs; ++i )
		{
			const Float lodDistance = mesh->GetLODLevel(i).GetDistance();
			const Float nextLodDistance = (i+1 < numLODs) ? mesh->GetLODLevel(i+1).GetDistance() : autoHideDistance;

			// gather useful cost related information from this LOD
			CResourceUsageCost cost;
			cost.m_name = CName( String::Printf( TXT("LOD%d"), i ).AsChar() );

			// Data size
			cost.m_memorySizeCPU = mesh->EstimateMemoryUsageCPU(i);
			cost.m_memorySizeGPU = mesh->EstimateMemoryUsageGPU(i);
			cost.m_triangleCount = mesh->CountLODTriangles(i);
			cost.m_vertexCount = mesh->CountLODVertices(i);

			// LOD visibility distances
			cost.m_visibilityRangeMin = lodDistance;
			cost.m_visibilityRangeMax = nextLodDistance;

			// add to db
			m_db->AddCost( depotPath, autoHideDistance, cost );
		}

		return true;
	}

	// not added - nothing known
	return false;
}

void CResourceUsageDataBaseCollector::ReportResourceUsage( const String& unsafeDepotPath )
{
	if ( unsafeDepotPath.Empty() )
		return;

	// no component/no entity
	if ( !m_componentName && !m_entityName )
		return; // ignore such information - not useful

	// conform the path
	String tempString;
	const String& depotPath = CFilePath::ConformPath( unsafeDepotPath, tempString );

	// grab resource cost information
	if ( !m_resources.KeyExist( depotPath ) )
	{
		const Bool valid = GrabResourceData( depotPath );
		m_resources.Set( depotPath, valid );
	}

	// build resource usage description
	CResourceUsageEntry entry;
	entry.m_layerName = m_layerName;
	entry.m_layerFlags = CopyFlags( m_layerFlags );
	entry.m_entityName = m_entityName;
	entry.m_entityClass = m_entityClass;
	entry.m_entityFlags = CopyFlags( m_entityFlags );
	entry.m_componentName = m_componentName;
	entry.m_componentClass = m_componentClass;
	entry.m_componentFlags = CopyFlags( m_componentFlags );
	entry.m_template = m_entityTemplate;

	// position
	if ( m_componentName )
		entry.m_position = m_componentPosition;
	else if ( m_entityName )
		entry.m_position = m_entityPosition;

	// box
	if ( m_componentName && m_componentHasBox )
		entry.m_box = m_componentBox;
	else if ( m_entityName && m_entityHasBox )
		entry.m_box = m_entityBox;

	// visibility distances
	Float visRangeMin = 0.0f;
	Float visRangeMax = FLT_MAX;
	if ( m_componentName && m_componentHasVisDistance )
	{
		visRangeMin = Red::Math::NumericalUtils::Max< Float >( visRangeMin, m_componentVisDistanceMin );
		visRangeMax = Red::Math::NumericalUtils::Min< Float >( visRangeMax, m_componentVisDistanceMax );
	}
	if ( m_entityName && m_entityHasVisDistance )
	{
		visRangeMin = Red::Math::NumericalUtils::Max< Float >( visRangeMin, m_entityVisDistanceMin );
		visRangeMax = Red::Math::NumericalUtils::Min< Float >( visRangeMax, m_entityVisDistanceMax );
	}
	entry.m_visibilityRangeMin = visRangeMin;
	entry.m_visibilityRangeMax = visRangeMax;

	// add to DB
	m_db->AddUsage( depotPath, entry );
}

///---

CResourceUsageCommandlet::CResourceUsageCommandlet()
	: m_usageDataBase( nullptr )
{
	m_commandletName = CName( TXT("resourceusage") );
}

CResourceUsageCommandlet::~CResourceUsageCommandlet()
{
	if ( m_usageDataBase )
	{
		delete m_usageDataBase;
		m_usageDataBase = nullptr;
	}
}

Bool CResourceUsageCommandlet::Execute( const CommandletOptions& options )
{
	// parse options
	if ( !m_settings.Parse( options ) )
		return false;

	// load the world
	WorldLoadingContext loadingContext;
	THandle< CWorld > world = CWorld::LoadWorld( m_settings.m_worldPath, loadingContext );
	if ( !world )
	{
		ERR_WCC( TXT("Could not load world '%ls'"), m_settings.m_worldPath.AsChar() );
		return false;
	}

	// disable streaming for now
	world->EnableStreaming( false, false );
	world->AddToRootSet();

	// create data base
	m_usageDataBase = new CResourceUsageDataBase();

	// test all the layers
	TDynArray< CLayerInfo* > allLayers;
	world->GetWorldLayers()->GetLayers( allLayers, false, true );

	// process layers
	for ( Uint32 i=0; i<allLayers.Size(); ++i )
	{
		CLayerInfo* layerInfo = allLayers[i];

		LOG_WCC( TXT("Status: [%d/%d] Processing '%ls'..."), 
			i, allLayers.Size(), layerInfo->GetDepotPath().AsChar() );

		// load the layer
		LayerLoadingContext loadingContext;
		loadingContext.m_loadHidden = true;
		if ( !layerInfo->SyncLoad( loadingContext ) )
		{
			ERR_WCC( TXT("Failed to load layer '%ls' it won't be processed"),
				layerInfo->GetDepotPath().AsChar() );
			continue;
		}

		// scan resource usage
		CLayer* layer = layerInfo->GetLayer();
		if ( layer )
		{
			CResourceUsageDataBaseCollector collector( m_usageDataBase );
			layer->CollectResourceUsage( collector );
		}

		// unload the layer
		{
			layerInfo->SyncUnload();
			world->UpdateLoadingState();

			SGarbageCollector::GetInstance().CollectNow();
			GObjectsDiscardList->ProcessList( true );
		}
	}

	// save db
	if ( !m_usageDataBase->SaveToFile( m_settings.m_outPath ))
	{
		ERR_WCC( TXT("Failed to save resource usage DB to '%ls'"),
			m_settings.m_outPath.AsChar() );
	}

	// delete any way
	delete m_usageDataBase;
	m_usageDataBase = nullptr;

	// unload world
	world->RemoveFromRootSet();
	CWorld::UnloadWorld( world );
	
	// done
	return true;
}

void CResourceUsageCommandlet::PrintHelp() const
{
	LOG_WCC( TXT("Usage:") );
	LOG_WCC( TXT("  resourceusage -world=<worldfilepath> -out=<outfile> [options]") );
	LOG_WCC( TXT("") );
	LOG_WCC( TXT("Options:") );
	LOG_WCC( TXT("  -world=<path>	   - Specifies path to the world file (required)") );
	LOG_WCC( TXT("  -out=<path>        - Sepcifies path to the output file (required)") );
	LOG_WCC( TXT("  -noerrors          - Allow cooking errors") );
	LOG_WCC( TXT("  -noasserts         - Allow asserts") );
	LOG_WCC( TXT("  -silent            - No output") );
}
