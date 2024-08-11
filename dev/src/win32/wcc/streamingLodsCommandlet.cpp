#include "build.h"

#include "../../common/core/commandlet.h"
#include "../../common/core/dependencyLinker.h"
#include "../../common/core/depot.h"
#include "../../common/core/garbageCollector.h"
#include "../../common/engine/layerGroup.h"
#include "../../common/engine/layerInfo.h"
#include "../../common/engine/entity.h"
#include "../../common/engine/entityTemplate.h"

//////////////////////////////////////////////////////////////////////////

#if 0
class CStreamingLodsCommandlet : public ICommandlet
{
	DECLARE_ENGINE_CLASS( CStreamingLodsCommandlet, ICommandlet, 0 );

public:
	CStreamingLodsCommandlet();

	virtual bool Execute( const CommandletOptions& options );
	virtual const Char* GetOneLiner() const	{ return TXT("List streaming LODs for objects (entities & templates)"); 	}
	virtual void PrintHelp() const {}

private:
	void ConditionalGC();

	struct TemplateInfo
	{
		String		m_path;
		Int32		m_maxLod;
	};

	struct EntityInfo
	{
		String		m_path;
		String		m_name;
		Int32		m_maxLod;
	};

	THashMap< String, TemplateInfo >	m_templates;
	TDynArray< EntityInfo >				m_entities;

	void ReportTemplate( const String& path, const Int32 maxLod );
	void ReportEntity( const String& path, const String& name, const Int32 maxLod );
	void DumpReport( const String& absolutePath );
};

DEFINE_SIMPLE_RTTI_CLASS( CStreamingLodsCommandlet, ICommandlet );
IMPLEMENT_ENGINE_CLASS( CStreamingLodsCommandlet );

CStreamingLodsCommandlet::CStreamingLodsCommandlet()
{
	m_commandletName = CName( TXT("streaminglods") );
}

bool CStreamingLodsCommandlet::Execute( const CommandletOptions& options )
{
	// output file
	String outputAbsolutePath;
	if ( !options.GetSingleOptionValue( TXT("out"), outputAbsolutePath ) )
	{
		ERR_WCC( TXT("No output path specified") );
		return false;
	}

	// enumerate all files
	TDynArray< CDiskFile* > allFiles;
	{
		LOG_WCC( TXT("Enumerating depot..." ) );
		GDepot->CollectFiles( allFiles, String::EMPTY, true, false );
		LOG_WCC( TXT("Found %d depot files" ), allFiles.Size() );
	}

	// gather entity templates
	TDynArray< CDiskFile* > entityTemplates;
	for ( Uint32 i=0; i<allFiles.Size(); ++i )
	{
		CDiskFile* file = allFiles[i];

		// is entity template ?
		if ( StringHelpers::GetFileExtension( file->GetFileName() ) == ResourceExtension< CEntityTemplate >() )
		{
			const CClass* resourceClass = file->GetResourceClass();
			if ( resourceClass->IsA( ClassID< CEntityTemplate >() ) )
			{
				entityTemplates.PushBack( file );
			}
		}
	}

	// analyze entity templates
	LOG_WCC( TXT("Found %d entity templates"), entityTemplates.Size() );
	for ( Uint32 i=0; i<entityTemplates.Size(); ++i )
	{
		CDiskFile* file = entityTemplates[i];
		LOG_WCC( TXT("Status: [%d/%d] Loading '%ls'..."), i, entityTemplates.Size(), file->GetDepotPath().AsChar() );

		// load 
		THandle< CEntityTemplate > entityTemplate = Cast< CEntityTemplate >( file->Load() );
		if ( !entityTemplate )
			continue;

		// get entity
		CEntity* entity = entityTemplate->GetEntityObject();
		if ( !entity )
		{
			ERR_WCC( TXT("Entity template '%ls' contain no entity"), file->GetDepotPath().AsChar() );
			continue;
		}

		// get the data buffer sizes
		const void* dataPtr = nullptr;
		Uint32 dataSize = 0;
		Int32 lod = -1;
		Int32 lodCount = 0;
		const Bool hasLod0 = entity->GetLocalStreamedComponentDataBuffer( 0, dataPtr, dataSize );
		const Bool hasLod1 = entity->GetLocalStreamedComponentDataBuffer( 1, dataPtr, dataSize );
		const Bool hasLod2 = entity->GetLocalStreamedComponentDataBuffer( 2, dataPtr, dataSize );
		if ( hasLod0 ) { lod = 0; lodCount += 1; }
		if ( hasLod1 ) { lod = 1; lodCount += 1; }
		if ( hasLod2 ) { lod = 2; lodCount += 1; }

		if ( lodCount > 1 )
		{
			ReportTemplate( file->GetDepotPath(), lod );
		}

		// GC
		ConditionalGC();
	}

	// gather layers
	TDynArray< CDiskFile* > layers;
	for ( Uint32 i=0; i<allFiles.Size(); ++i )
	{
		CDiskFile* file = allFiles[i];

		// is entity template ?
		if ( StringHelpers::GetFileExtension( file->GetFileName() ) == ResourceExtension< CLayer >() )
		{
			const CClass* resourceClass = file->GetResourceClass();
			if ( resourceClass->IsA( ClassID< CLayer >() ) )
			{
				layers.PushBack( file );
			}
		}
	}

	// analyze layers
	LOG_WCC( TXT("Found %d layers"), layers.Size() );
	for ( Uint32 i=0; i<layers.Size(); ++i )
	{
		CDiskFile* file = layers[i];
		LOG_WCC( TXT("Status: [%d/%d] Loading '%ls'..."), i, layers.Size(), file->GetDepotPath().AsChar() );

		// load 
		THandle< CLayer > layer = Cast< CLayer >( file->Load() );
		if ( !layer)
			continue;

		// process entities, only the detached ones
		const Uint32 numEntities = layer->GetEntities().Size();
		for ( Uint32 i=0; i<numEntities; ++i )
		{
			// get entity
			CEntity* entity = layer->GetEntities()[i];
			if ( !entity )
				continue;

			// skip entities attached to their templates
			if ( entity->GetEntityTemplate() )
				continue;

			// get the data buffer sizes
			const void* dataPtr = nullptr;
			Uint32 dataSize = 0;
			Int32 lod = -1;
			Int32 lodCount = 0;
			const Bool hasLod0 = entity->GetLocalStreamedComponentDataBuffer( 0, dataPtr, dataSize );
			const Bool hasLod1 = entity->GetLocalStreamedComponentDataBuffer( 1, dataPtr, dataSize );
			const Bool hasLod2 = entity->GetLocalStreamedComponentDataBuffer( 2, dataPtr, dataSize );
			if ( hasLod0 ) { lod = 0; lodCount += 1; }
			if ( hasLod1 ) { lod = 1; lodCount += 1; }
			if ( hasLod2 ) { lod = 2; lodCount += 1; }

			// entity has streaming, report it
			if ( lodCount > 1 )
			{
				ReportEntity( file->GetDepotPath(), entity->GetName(), lod );
			}
		}

		// GC
		ConditionalGC();
	}

	// save the report
	DumpReport( outputAbsolutePath );
	return true;
}

void CStreamingLodsCommandlet::ConditionalGC()
{
	const Uint32 totalAllocateCPU_MB = (Uint32)(SRedMemory::GetInstance().GetMetricsCollector().GetTotalBytesAllocated() >> 20);
	if ( totalAllocateCPU_MB > 1024 )
	{
		SGarbageCollector::GetInstance().CollectNow();
		SGarbageCollector::GetInstance().CollectNow();
	}
}

void CStreamingLodsCommandlet::ReportTemplate( const String& path, const Int32 maxLod )
{
	TemplateInfo info;
	if ( m_templates.Find( path, info ) )
	{
		if ( maxLod > info.m_maxLod )
			info.m_maxLod = maxLod;
	}
	else
	{
		info.m_maxLod = maxLod;
		info.m_path = path;
	}
	m_templates.Set( path, info );
}

void CStreamingLodsCommandlet::ReportEntity( const String& path, const String& name, const Int32 maxLod )
{
	EntityInfo* info = new ( m_entities ) EntityInfo();
	info->m_path = path;
	info->m_name = name;
	info->m_maxLod = maxLod;	
}

void CStreamingLodsCommandlet::DumpReport( const String& absolutePath )
{
	GFileManager->CreatePath( absolutePath );

	FILE* f = nullptr;
	_wfopen_s( &f, absolutePath.AsChar(), L"w" );
	if ( !f )  return;

	// dump templates
	if ( !m_templates.Empty() )
	{
		TDynArray< TemplateInfo > templates;
		for ( auto it = m_templates.Begin(); it != m_templates.End(); ++it )
		{
			templates.PushBack( (*it).m_second );
		}

		Sort( templates.Begin(), templates.End(), []( const TemplateInfo& a, const TemplateInfo& b ) { return a.m_path < b.m_path; } );
		fwprintf( f, L"%d templates\n", m_templates.Size() );
		for ( const TemplateInfo& info : templates )
		{
			fwprintf( f, L" %d: %s\n", info.m_maxLod, info.m_path.AsChar() );
		}
	}

	// dump entities
	if ( !m_entities.Empty() )
	{
		Sort( m_entities.Begin(), m_entities.End(), []( const EntityInfo& a, const EntityInfo& b )
		{
			if ( a.m_path < b.m_path ) return true;
			if ( a.m_path > b.m_path ) return false;
			return a.m_name < b.m_name;
		});

		fwprintf( f, L"%d entities from layers\n", m_entities.Size() );
		for ( const EntityInfo& info : m_entities )
		{
			fwprintf( f, L" %d: %s in %s\n", info.m_maxLod, info.m_name.AsChar(), info.m_path.AsChar() );
		}
	}

	// done
	fclose(f);
}

//////////////////////////////////////////////////////////////////////////
#endif