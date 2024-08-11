/**
* Copyright (c) 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "../../common/core/depot.h"
#include "../../common/core/algorithms.h"
#include "../../common/core/xmlFileReader.h"
#include "../../common/engine/layerInfo.h"
#include "cookDataBase.h"

//---

CCookerResourceUsageEntry::CCookerResourceUsageEntry()
	: m_position(0,0,0)
	, m_visibilityDistance(-1.0f)
{
}

const Uint64 CCookerResourceUsageEntry::CalcRuntimeHash() const
{
	return ACalcBufferHash64Merge( this, sizeof(CCookerResourceUsageEntry), HASH64_BASE ); 
}

void CCookerResourceUsageEntry::Setup(const class CComponent* component)
{
	if (!component)
		return;

	m_componentName = CName( component->GetName().AsChar() );
	m_componentClass = component->GetClass()->GetName();

	Setup( component->GetEntity() );

	// refine position
	m_position = component->GetWorldPosition();
}

void CCookerResourceUsageEntry::Setup(const class CEntity* entity)
{
	if (!entity)
		return;

	m_entityName = CName( entity->GetName().AsChar() );
	m_entityClass = entity->GetClass()->GetName();

	Setup( entity->GetLayer() );
}

void CCookerResourceUsageEntry::Setup(const class CLayer* layer)
{
	if (!layer || !layer->GetLayerInfo())
		return;

	const CFilePath layerDepotPath( layer->GetLayerInfo()->GetDepotPath() );
	m_layerName = CName( layerDepotPath.GetFileName().AsChar() );

	Setup( layer->GetLayerInfo()->GetWorld() );
}

void CCookerResourceUsageEntry::Setup(const class CWorld* world)
{
	if (!world)
		return;

	const CFilePath worldDepotPath( world->GetDepotPath() );
	m_layerName = CName( worldDepotPath.GetDirectories().Back().AsChar() );
}

//---

CCookerResourceEntry::CCookerResourceEntry()
{
}

//---

CCookerDataBase::UsageEntry::UsageEntry()
{
}


CCookerDataBase::UsageEntry::UsageEntry( const CCookerResourceUsageEntry& entry )
	: CCookerResourceUsageEntry(entry)
{
}

//---

CCookerDataBase::FileEntry::FileEntry( const StringAnsi& filePath )
	: m_memoryCPU( 0 )
	, m_memoryGPU( 0 )
	, m_numImportedAsSoftDependency( 0 )
	, m_numImportedAsHardDependency( 0 )
	, m_numImportedAsInplaceDependency( 0 )
	, m_dependencyLevel( 0 )
	, m_flagIsSeedFile( 0 )
	, m_flagIsBufferFile( 0 )
	, m_flagIsConsumed( 0 )
	, m_flagIsAdditional( 0 )
	, m_diskSize( 0 )
{
	m_groupIndex = 0;
	CFilePath::GetConformedPath( filePath, m_filePath );
	m_fileId = TResourceID( m_filePath );
}

void CCookerDataBase::FileEntry::ResolveUnresolvedDependencies(const TEntriesMap& depList)
{
	for ( TResourceID id : m_unresolvedSoftDependencies )
	{
		FileEntry* dep = nullptr;
		if ( depList.Find( id, dep ) )
		{
			m_softDependencies.PushBack( dep );
			dep->m_numImportedAsSoftDependency += 1;
		}
		else
		{
			AnsiChar tempName[ 128 ]; // TODO: TResourceID::MAX_STRING
			id.ToString(tempName, ARRAY_COUNT(tempName));

			WARN_WCC( TXT("Unresolved soft depedency '%ls' in '%ls'"), 
				ANSI_TO_UNICODE(tempName), ANSI_TO_UNICODE(GetFilePath().AsChar()) );
		}
	}

	for ( TResourceID id : m_unresolvedHardDependencies )
	{
		FileEntry* dep = nullptr;
		if ( depList.Find( id, dep ) )
		{
			m_hardDependencies.PushBack( dep );
			dep->m_numImportedAsHardDependency += 1;
		}
		else
		{
			AnsiChar tempName[ 128 ]; // TODO: TResourceID::MAX_STRING
			id.ToString(tempName, ARRAY_COUNT(tempName));

			WARN_WCC( TXT("Unresolved hard depedency '%ls' in '%ls'"), 
				ANSI_TO_UNICODE(tempName), ANSI_TO_UNICODE(GetFilePath().AsChar()) );
		}
	}

	for ( TResourceID id : m_unresolvedInplaceDependencies )
	{
		FileEntry* dep = nullptr;
		if ( depList.Find( id, dep ) )
		{
			m_inplaceDependencies.PushBack( dep );
			dep->m_numImportedAsInplaceDependency += 1;
		}
		else
		{
			AnsiChar tempName[ 128 ]; // TODO: TResourceID::MAX_STRING
			id.ToString(tempName, ARRAY_COUNT(tempName));

			WARN_WCC( TXT("Unresolved inplace depedency '%ls' in '%ls'"), 
				ANSI_TO_UNICODE(tempName), ANSI_TO_UNICODE(GetFilePath().AsChar()) );
		}
	}

	m_unresolvedSoftDependencies.Clear();
	m_unresolvedHardDependencies.Clear();
	m_unresolvedInplaceDependencies.Clear();
}

void CCookerDataBase::FileEntry::ResolveDepenencyOrder(const Int32 level)
{
	if( level > 100 )
	{
		ERR_WCC( TXT( "The dependency level for the file: '%ls' is too high! Circular dependency most likely..." ),
				ANSI_TO_UNICODE( GetFilePath().AsChar() ));
		return;
	}
	if (level > m_dependencyLevel)
	{
		m_dependencyLevel = level;

		for (Uint32 i=0; i<m_hardDependencies.Size(); ++i)
		{
			FileEntry* dep = m_hardDependencies[i];
			dep->ResolveDepenencyOrder( level+1 );
		}
	}
}

//---

CCookerDataBase::CCookerDataBase()
{
}

CCookerDataBase::~CCookerDataBase()
{
	Reset();
}

void CCookerDataBase::Reset()
{
	m_entries.ClearPtr();
	m_entriesMap.Clear();
}

void CCookerDataBase::GetFileEntries( TDynArray<CCookerResourceEntry>& outEntries ) const
{
	// copy ordered list
	outEntries.Resize( m_entries.Size() );
	for ( Uint32 i=0; i<m_entries.Size(); ++i )
	{
		outEntries[i] = *m_entries[i];
	}
}

CCookerDataBase::TCookerDataBaseID CCookerDataBase::GetFileEntry( const String& depotPath ) const
{
	const Red::Core::ResourceManagement::CResourceId resId(depotPath);

	FileEntry* fileEntry = NULL;
	if ( !m_entriesMap.Find( resId, fileEntry ) )
		return NO_ENTRY;

	return ToID( fileEntry );
}

CCookerDataBase::TCookerDataBaseID CCookerDataBase::GetFileEntry( const StringAnsi& depotPath ) const
{
	const Red::Core::ResourceManagement::CResourceId resId(depotPath);

	FileEntry* fileEntry = NULL;
	if ( !m_entriesMap.Find( resId, fileEntry ) )
		return NO_ENTRY;

	return ToID( fileEntry );
}

CCookerDataBase::TCookerDataBaseID CCookerDataBase::GetFileEntry( const Red::Core::ResourceManagement::CResourceId& resId ) const
{
	FileEntry* fileEntry = NULL;
	if ( !m_entriesMap.Find( resId, fileEntry ) )
		return NO_ENTRY;

	return ToID( fileEntry );
}

CCookerDataBase::TCookerDataBaseID CCookerDataBase::AddFileEntry( const String& depotPath )
{
	const Red::Core::ResourceManagement::CResourceId resId(depotPath);

	FileEntry* fileEntry = NULL;
	if ( !m_entriesMap.Find( resId, fileEntry ) )
	{
		fileEntry = new FileEntry( UNICODE_TO_ANSI( depotPath.AsChar() ) );
		m_entriesMap.Insert( resId, fileEntry );
		m_entries.PushBack( fileEntry );
	}

	return ToID( fileEntry );
}

void CCookerDataBase::AddFileHardDependency( TCookerDataBaseID baseFile, TCookerDataBaseID dependencyFile )
{
	FileEntry* baseFileEntry = FromID( baseFile );
	FileEntry* depFileEntry = FromID( dependencyFile );

	if ( !baseFileEntry || !depFileEntry )
		return;

	if ( baseFileEntry->m_hardDependencies.PushBackUnique(depFileEntry) )
	{
		depFileEntry->m_numImportedAsHardDependency += 1;
	}
}

void CCookerDataBase::AddFileSoftDependency( TCookerDataBaseID baseFile, TCookerDataBaseID dependencyFile )
{
	FileEntry* baseFileEntry = FromID( baseFile );
	FileEntry* depFileEntry = FromID( dependencyFile );

	if ( !baseFileEntry || !depFileEntry )
		return;

	if ( baseFileEntry->m_softDependencies.PushBackUnique(depFileEntry) )
	{
		depFileEntry->m_numImportedAsSoftDependency += 1;
	}
}

void CCookerDataBase::RemoveInplaceDependencies( TCookerDataBaseID baseFile )
{
	FileEntry* baseFileEntry = FromID( baseFile );
	if ( !baseFileEntry )
		return;

	for ( FileEntry* entry : baseFileEntry->m_inplaceDependencies )
	{
		RED_FATAL_ASSERT( entry->m_numImportedAsInplaceDependency > 0, "Invalid inplace dependencies counter - 'cook.db' file is corrupted." );
		entry->m_numImportedAsInplaceDependency -= 1;
	}

	baseFileEntry->m_inplaceDependencies.ClearFast();
}

void CCookerDataBase::AddFileInplaceDependency( TCookerDataBaseID baseFile, TCookerDataBaseID dependencyFile )
{
	FileEntry* baseFileEntry = FromID( baseFile );
	FileEntry* depFileEntry = FromID( dependencyFile );

	if ( !baseFileEntry || !depFileEntry )
		return;

	baseFileEntry->m_inplaceDependencies.PushBackUnique(depFileEntry);
	depFileEntry->m_numImportedAsInplaceDependency += 1;
}

void CCookerDataBase::SetFileMemoryUsageCPU( TCookerDataBaseID baseFile, const Uint32 estimatedMemoryUsage )
{
	FileEntry* baseFileEntry = FromID( baseFile );

	if ( !baseFileEntry )
		return;

	baseFileEntry->m_memoryCPU = estimatedMemoryUsage;
}

void CCookerDataBase::SetFileMemoryUsageGPU( TCookerDataBaseID baseFile, const Uint32 estimatedMemoryUsage )
{
	FileEntry* baseFileEntry = FromID( baseFile );

	if ( !baseFileEntry )
		return;

	baseFileEntry->m_memoryGPU = estimatedMemoryUsage;
}

void CCookerDataBase::SetFileDiskSize(TCookerDataBaseID baseFile, const Uint32 fileDiskSize)
{
	FileEntry* baseFileEntry = FromID( baseFile );

	if ( !baseFileEntry )
		return;

	baseFileEntry->m_diskSize = fileDiskSize;
}

void CCookerDataBase::SetFileResourceClass(TCookerDataBaseID baseFile, const CName& className)
{
	FileEntry* baseFileEntry = FromID( baseFile );

	if ( !baseFileEntry )
		return;

	baseFileEntry->m_className = className;
}

void CCookerDataBase::SetFileBufferFlag(TCookerDataBaseID baseFile, const Bool isBuffer)
{
	FileEntry* baseFileEntry = FromID( baseFile );

	if ( !baseFileEntry )
		return;

	baseFileEntry->m_flagIsBufferFile = isBuffer ? 1 : 0;
}

void CCookerDataBase::SetFileConsumedFlag(TCookerDataBaseID baseFile, const Bool isBuffer)
{
	FileEntry* baseFileEntry = FromID( baseFile );

	if ( !baseFileEntry )
		return;

	baseFileEntry->m_flagIsConsumed = isBuffer ? 1 : 0;
}

void CCookerDataBase::SetFileAdditionalFlag(TCookerDataBaseID baseFile, const Bool isBuffer)
{
	FileEntry* baseFileEntry = FromID( baseFile );

	if ( !baseFileEntry )
		return;

	baseFileEntry->m_flagIsAdditional = isBuffer ? 1 : 0;
}

void CCookerDataBase::SetFileSeedFlag(TCookerDataBaseID baseFile, const Bool isBuffer)
{
	FileEntry* baseFileEntry = FromID( baseFile );

	if ( !baseFileEntry )
		return;

	baseFileEntry->m_flagIsSeedFile = isBuffer ? 1 : 0;
}

void CCookerDataBase::AddFileUsage( TCookerDataBaseID baseFile, const CCookerResourceUsageEntry& entry )
{
	FileEntry* baseFileEntry = FromID( baseFile );

	if ( !baseFileEntry )
		return;

	// prevent adding the same usage information twice
	const Uint64 runtimeHash = entry.CalcRuntimeHash();
	UsageEntry* usageEntry = NULL;
	if ( !baseFileEntry->m_usage.Find( runtimeHash, usageEntry ) )
	{
		usageEntry = new UsageEntry( entry );
		baseFileEntry->m_usage.Insert( runtimeHash, usageEntry );
	}
}

Bool CCookerDataBase::GetFileEntry( TCookerDataBaseID baseFile, CCookerResourceEntry& outEntry ) const
{
	FileEntry* baseFileEntry = FromID( baseFile );

	if ( !baseFileEntry )
		return false;

	outEntry = *baseFileEntry;
	return true;
}

Bool CCookerDataBase::GetFileDependencies( TCookerDataBaseID baseFile, TDynArray<CCookerResourceEntry>& outHardDependencies, TDynArray<CCookerResourceEntry>& outSoftDependencies, TDynArray<CCookerResourceEntry>& outInplaceDependencies ) const
{
	FileEntry* baseFileEntry = FromID( baseFile );

	if ( !baseFileEntry )
		return false;

	// copy hard dependencies (only file name and ID)
	outHardDependencies.Resize( baseFileEntry->m_hardDependencies.Size() );
	for ( Uint32 i=0; i<baseFileEntry->m_hardDependencies.Size(); ++i )
	{
		const FileEntry* dep = baseFileEntry->m_hardDependencies[i];
		outHardDependencies[i] = *dep;
	}

	// copy soft dependencies (only file name and ID)
	outSoftDependencies.Resize( baseFileEntry->m_softDependencies.Size() );
	for ( Uint32 i=0; i<baseFileEntry->m_softDependencies.Size(); ++i )
	{
		const FileEntry* dep = baseFileEntry->m_softDependencies[i];
		outSoftDependencies[i] = *dep;
	}

	// copy inplace dependencies (only file name and ID)
	outInplaceDependencies.Resize( baseFileEntry->m_inplaceDependencies.Size() );
	for ( Uint32 i=0; i<baseFileEntry->m_inplaceDependencies.Size(); ++i )
	{
		const FileEntry* dep = baseFileEntry->m_inplaceDependencies[i];
		outInplaceDependencies[i] = *dep;
	}

	return true;
}

Bool CCookerDataBase::GetFileResourceClass( TCookerDataBaseID baseFile, CName& outClassName ) const
{
	FileEntry* baseFileEntry = FromID( baseFile );

	if ( !baseFileEntry )
		return false;

	outClassName = baseFileEntry->m_className;
	return true;
}

Bool CCookerDataBase::GetFileDiskSize( TCookerDataBaseID baseFile, Uint32& outDiskSize ) const
{
	FileEntry* baseFileEntry = FromID( baseFile );

	if ( !baseFileEntry )
		return false;

	outDiskSize = baseFileEntry->m_diskSize;
	return true;
}

Bool CCookerDataBase::GetFileMemoryUsageCPU( TCookerDataBaseID baseFile, Uint32& outMemoryUsage ) const
{
	FileEntry* baseFileEntry = FromID( baseFile );

	if ( !baseFileEntry )
		return false;

	outMemoryUsage = baseFileEntry->m_memoryCPU;
	return true;
}

Bool CCookerDataBase::GetFileMemoryUsageGPU( TCookerDataBaseID baseFile, Uint32& outMemoryUsage ) const
{
	FileEntry* baseFileEntry = FromID( baseFile );

	if ( !baseFileEntry )
		return false;

	outMemoryUsage = baseFileEntry->m_memoryGPU;
	return true;
}

Bool CCookerDataBase::GetFileBufferFlag( TCookerDataBaseID baseFile, Bool& outBufferFlag ) const
{
	FileEntry* baseFileEntry = FromID( baseFile );

	if ( !baseFileEntry )
		return false;

	outBufferFlag = baseFileEntry->m_flagIsBufferFile;
	return true;
}

Bool CCookerDataBase::GetFileSeedFlag( TCookerDataBaseID baseFile, Bool& outSeedFlag ) const
{
	FileEntry* baseFileEntry = FromID( baseFile );

	if ( !baseFileEntry )
		return false;

	outSeedFlag = baseFileEntry->m_flagIsSeedFile;
	return true;
}

Bool CCookerDataBase::GetFileConsumedFlag( TCookerDataBaseID baseFile, Bool& outConsumedFlag ) const
{
	FileEntry* baseFileEntry = FromID( baseFile );

	if ( !baseFileEntry )
		return false;

	outConsumedFlag = baseFileEntry->m_flagIsConsumed;
	return true;
}

