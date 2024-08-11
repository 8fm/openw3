/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#ifndef _RED_RESOURCE_DATABASE_H_
#define _RED_RESOURCE_DATABASE_H_

// This Define determines whether the dependency graphs can be edited
#ifndef NO_EDITOR
	#define ENABLE_RESOURCE_DATABASE_EDITING
#endif

#include "resourcedatabaserecord.h"
#include "resourcedatabaseshareddata.h"
#include "dependencygraph.h"
#include "sortedmap.h"

namespace Red
{
	namespace Core
	{
		namespace ResourceManagement
		{
			// This represents a graph of data about a set of resources / object dependencies
			// No direct pointer access is allowed - this is to ensure that the data can be easily rebuilt / serialised with minimal overhead
			class CResourceBuildDatabase
			{
				DECLARE_CLASS_MEMORY_ALLOCATOR( MC_ResourceDatabase );

			public:
				CResourceBuildDatabase();
				~CResourceBuildDatabase();

				// Resource index is used to map to the entries in the database. No direct pointer access allowed
				typedef System::Uint32 ResourceIndex;
				static const ResourceIndex c_InvalidIndex = (ResourceIndex)-1;

#ifdef ENABLE_RESOURCE_DATABASE_EDITING

				// Adding resources...
				ResourceIndex AddNewResource( const System::AnsiChar* typeName, const CResourceId& resourceId );

				// Adding resource metadata
				void SetResourceName( ResourceIndex theResource, const System::AnsiChar* name );
				void SetResourcePath( ResourceIndex theResource, const System::AnsiChar* sourcePath );
				void SetResourceGUID( ResourceIndex theResource, const CGUID& theGuid );

				// Adding dependencies between records...
				void AddDependency( ResourceIndex dependantResource, ResourceIndex dependency );

#endif	//ENABLE_RESOURCE_DATABASE_EDITING

				// Finding existing resources
				ResourceIndex FindResourceById( const CResourceId& resourceId ) const;

				// Polling resource metadata
				const System::AnsiChar* GetResourceTypeName( ResourceIndex theResource ) const;
				System::Uint32 GetResourceTypeHash( ResourceIndex theResource ) const;
				const System::AnsiChar* GetResourceName( ResourceIndex theResource ) const;
				const System::AnsiChar* GetResourceSourcePath( ResourceIndex theResource ) const;
				const CGUID* GetResourceGUID( ResourceIndex theResource ) const;
				CResourceId GetResourceId( ResourceIndex theResource ) const;

				// Polling dependencies
				void GetImmediateDependencies( ResourceIndex theResource, TSortedSet<ResourceIndex>& dependencies );
				void GetAllDependencies( ResourceIndex theResource, TSortedSet<ResourceIndex>& dependencies );
				System::Uint32 GetImmediateDependencyCount( ResourceIndex theResource );

				// High level metrics 
				System::Uint32 GetResourceCount() const;

				// Serialisation
				void Serialise( IFile& theFile );

			private:

				// This is a flat array of metadata records for each resource in the system.
				// These are mapped 1:1 with the dependency graph indices to allow fast lookups
				TDynArray< CResourceDatabaseRecord, MC_ResourceDatabase > m_metadataRecords;

				// This is the dependency graph between resources
				CResourceGraph m_dependencyGraph;

				// This data is unique and required per record
				CPooledResourceData< CResourceId > m_resourceIdPool;

				// This is pooled data that is unique per record (but optional)
				CPooledResourceData< System::AnsiChar > m_resourceNamePool;
				CPooledResourceData< CGUID > m_guidPool;
			
				// These shared data objects are used to store pools of data that is not unique to each record
				TSharedResourceData< System::AnsiChar, System::Uint32 > m_typenamePool;
				TSharedResourceData< System::AnsiChar, System::Uint64 > m_pathPool;

				// These are the lookup tables for quickly finding resources based on various keys
				THashMap< CResourceId, ResourceIndex, DefaultHashFunc< CResourceId >, DefaultEqualFunc< CResourceId >, MC_ResourceDatabase > m_resourceIdLookup;
			};
		}
	}
}

#endif
