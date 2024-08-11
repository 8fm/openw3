/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "resourcedatabase.h"

#include "../redSystem/hash.h"

namespace Red {	namespace Core {
namespace ResourceManagement {

//////////////////////////////////////////////////////////////////////////
CResourceBuildDatabase::CResourceBuildDatabase()
{
}

//////////////////////////////////////////////////////////////////////////
CResourceBuildDatabase::~CResourceBuildDatabase()
{
}

#ifdef ENABLE_RESOURCE_DATABASE_EDITING

//////////////////////////////////////////////////////////////////////////
CResourceBuildDatabase::ResourceIndex CResourceBuildDatabase::AddNewResource( const System::AnsiChar* typeName, const CResourceId& resourceId )
{
	// Make sure the type is in the shared type data
	System::Uint32 typeHash = System::CalculateHash32( typeName );
	m_typenamePool.Add( typeHash, typeName );

	// Add a new resource record, and pass the data to it
	ResourceIndex newIndex = m_metadataRecords.Size();
	m_metadataRecords.Grow( 1 );
	RED_ASSERT( newIndex < m_metadataRecords.Size(), TXT( "Failed to add a new metadata record!" ) );

	CResourceDatabaseRecord& newRecord = m_metadataRecords[newIndex];
	newRecord.SetTypeHash( typeHash );

	// Make sure we add a corresponding dependency graph root node
	System::Uint32 dependencyIndex = m_dependencyGraph.AddNewNode();
	RED_UNUSED( dependencyIndex );
	RED_ASSERT( dependencyIndex == newIndex, TXT( "The dependency graph is out of sync with the resource database. Prepare for wierdness!" ) );

	// Store the resource id
	RED_ASSERT( !m_resourceIdLookup.KeyExist( resourceId ), TXT( "Hash already exists in this database!" ) );
	m_resourceIdLookup.Insert( resourceId, newIndex );

	CResourceDatabasePoolEntry entry = m_resourceIdPool.Add( resourceId );
	m_metadataRecords[ newIndex ].SetEntry( CResourceDatabaseRecord::ET_Id, entry );

	return newIndex;
}

//////////////////////////////////////////////////////////////////////////
void CResourceBuildDatabase::SetResourceName( ResourceIndex theResource, const System::AnsiChar* name )
{
	RED_ASSERT( theResource != c_InvalidIndex, TXT( "Bad resource index" ) );
			
	// Always set the name on the record, regardless of it being a duplicate
	CResourceDatabasePoolEntry nameEntry = m_resourceNamePool.Add( name );
	m_metadataRecords[ theResource ].SetEntry( CResourceDatabaseRecord::ET_Name, nameEntry );
}

//////////////////////////////////////////////////////////////////////////
void CResourceBuildDatabase::SetResourcePath( ResourceIndex theResource, const System::AnsiChar* sourcePath )
{
	RED_ASSERT( theResource != c_InvalidIndex, TXT( "Bad resource index" ) );

	// If the path already exists, use that one
	System::Uint64 pathHash = System::CalculateHash64( sourcePath );
	CResourceDatabasePoolEntry pathEntry = m_pathPool.Add( pathHash, sourcePath );
	m_metadataRecords[ theResource ].SetEntry( CResourceDatabaseRecord::ET_Path, pathEntry );
}

//////////////////////////////////////////////////////////////////////////
void CResourceBuildDatabase::SetResourceGUID( ResourceIndex theResource, const CGUID& theGuid )
{
	RED_ASSERT( theResource != c_InvalidIndex, TXT( "Bad resource index" ) );

	CResourceDatabasePoolEntry guidEntry = m_guidPool.Add( theGuid );
	m_metadataRecords[ theResource ].SetEntry( CResourceDatabaseRecord::ET_GUID, guidEntry );
}

//////////////////////////////////////////////////////////////////////////
void CResourceBuildDatabase::AddDependency( ResourceIndex dependantResource, ResourceIndex dependency )
{
#ifdef ENABLE_DEPENDENCY_GRAPH_EDITING
	RED_ASSERT( dependantResource != c_InvalidIndex, TXT( "Bad resource index" ) );
	RED_ASSERT( dependency != c_InvalidIndex, TXT( "Bad resource index" ) );

	m_dependencyGraph.AddLink( dependantResource, dependency );
#else
	RED_UNUSED( dependantResource );
	RED_UNUSED( dependency );
	RED_HALT( TXT( "Dependency graph editing is disabled!" ) );
#endif	//ENABLE_DEPENDENCY_GRAPH_EDITING
}


#endif	//ENABLE_RESOURCE_DATABASE_EDITING

//////////////////////////////////////////////////////////////////////////
CResourceBuildDatabase::ResourceIndex CResourceBuildDatabase::FindResourceById( const ResourceManagement::CResourceId& resourceId ) const
{
	ResourceIndex resultIndex = c_InvalidIndex;
	m_resourceIdLookup.Find( resourceId, resultIndex );
	return resultIndex;
}

//////////////////////////////////////////////////////////////////////////
const System::AnsiChar* CResourceBuildDatabase::GetResourceTypeName( ResourceIndex theResource ) const
{
	RED_ASSERT( theResource != c_InvalidIndex, TXT( "Bad resource index" ) );
	System::Uint32 typeHash = m_metadataRecords[ theResource ].GetTypeHash();
	return m_typenamePool.Get( typeHash );
}

//////////////////////////////////////////////////////////////////////////
System::Uint32 CResourceBuildDatabase::GetResourceTypeHash( ResourceIndex theResource ) const
{
	RED_ASSERT( theResource != c_InvalidIndex, TXT( "Bad resource index" ) );
	return m_metadataRecords[ theResource ].GetTypeHash();
}

//////////////////////////////////////////////////////////////////////////
const System::AnsiChar* CResourceBuildDatabase::GetResourceSourcePath( ResourceIndex theResource ) const
{
	RED_ASSERT( theResource != c_InvalidIndex, TXT( "Bad resource index" ) );
	CResourceDatabasePoolEntry entry = m_metadataRecords[ theResource ].GetEntry( CResourceDatabaseRecord::ET_Path );
	return m_pathPool.Get( entry );
}

//////////////////////////////////////////////////////////////////////////
const CGUID* CResourceBuildDatabase::GetResourceGUID( ResourceIndex theResource ) const
{
	RED_ASSERT( theResource != c_InvalidIndex, TXT( "Bad resource index" ) );
	CResourceDatabasePoolEntry entry = m_metadataRecords[ theResource ].GetEntry( CResourceDatabaseRecord::ET_GUID );
	return m_guidPool.Get( entry );
}

ResourceManagement::CResourceId CResourceBuildDatabase::GetResourceId( ResourceIndex theResource ) const
{
	RED_ASSERT( theResource != c_InvalidIndex, TXT( "Bad resource index" ) );
	CResourceDatabasePoolEntry entry = m_metadataRecords[ theResource ].GetEntry( CResourceDatabaseRecord::ET_Id );
	return *m_resourceIdPool.Get( entry );
}

//////////////////////////////////////////////////////////////////////////
const System::AnsiChar* CResourceBuildDatabase::GetResourceName( ResourceIndex theResource ) const
{
	RED_ASSERT( theResource != c_InvalidIndex, TXT( "Bad resource index" ) );
	CResourceDatabasePoolEntry entry = m_metadataRecords[ theResource ].GetEntry( CResourceDatabaseRecord::ET_Name );
	return m_resourceNamePool.Get( entry );
}

//////////////////////////////////////////////////////////////////////////
System::Uint32 CResourceBuildDatabase::GetImmediateDependencyCount( ResourceIndex theResource )
{
	return m_dependencyGraph.GetChildCount( theResource );
}

//////////////////////////////////////////////////////////////////////////
void CResourceBuildDatabase::GetImmediateDependencies( ResourceIndex theResource, TSortedSet<ResourceIndex>& dependencies )
{
	RED_ASSERT( theResource != c_InvalidIndex, TXT( "Bad resource index" ) );
	m_dependencyGraph.GetChildren( theResource, dependencies );
}

//////////////////////////////////////////////////////////////////////////
void CResourceBuildDatabase::GetAllDependencies( ResourceIndex theResource, TSortedSet<ResourceIndex>& dependencies )
{
	RED_ASSERT( theResource != c_InvalidIndex, TXT( "Bad resource index" ) );
	m_dependencyGraph.GetDescendents( theResource, dependencies );
}

//////////////////////////////////////////////////////////////////////////
System::Uint32 CResourceBuildDatabase::GetResourceCount() const
{
	return m_metadataRecords.Size();
}

//////////////////////////////////////////////////////////////////////////
// Serialisation
void CResourceBuildDatabase::Serialise( IFile& theFile )
{
	// Serialise the records
	m_metadataRecords.SerializeBulk( theFile );

	// Save the dependency graph
	m_dependencyGraph.Serialise( theFile );

	// Save pooled data
	m_resourceNamePool.Serialise( theFile );
	m_typenamePool.Serialise( theFile );
	m_pathPool.Serialise( theFile );
	m_guidPool.Serialise( theFile );
	m_resourceIdPool.Serialise( theFile );

	// Save lookup tables
	theFile << m_resourceIdLookup;
}

}	// namespace ResourceManagement {
} } // namespace Red {	namespace Core {
