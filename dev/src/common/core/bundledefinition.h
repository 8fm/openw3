/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#ifndef NO_EDITOR
#ifndef _RED_BUNDLE_DEFINITION_H_
#define _RED_BUNDLE_DEFINITION_H_

#include "bundleHeader.h"
#include "string.h"
#include "hashmap.h"
#include "hashset.h"
#include "resourceid.h"
#include "jsonFilePrettyWriter.h"
#include "jsonFileReader.h"
#include "jsonArray.h"

class CFilePath;

#define DEFINE_BUNDLE_NEWDELETE_OPERATORS											\
	static void* operator new( size_t size )										\
	{																				\
		return RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_ResourceBuffer, size );	\
	}																				\
																					\
	static void operator delete( void* ptr )										\
	{																				\
		RED_MEMORY_FREE( MemoryPool_Default, MC_ResourceBuffer, ptr );				\
	}

namespace Red
{
	namespace Core
	{
		namespace BundleDefinition
		{

			RED_JSON_MEMORY_DEFINE_ALLOCATOR(MemoryPool_Default, MC_ResourceBuffer)
			typedef CJSONFilePrettyWriter< rapidjson::UTF8<Red::System::AnsiChar>, RED_JSON_MEMORY_GET_ALLOCATOR(MemoryPool_Default, MC_ResourceBuffer) > CBundleJSONWriter;
			typedef CJSONBasicRef< rapidjson::UTF8<Red::System::AnsiChar>, RED_JSON_MEMORY_GET_ALLOCATOR(MemoryPool_Default, MC_ResourceBuffer) > CBundleJSONBasicRef;
			typedef CJSONArrayRef< rapidjson::UTF8<Red::System::AnsiChar>, RED_JSON_MEMORY_GET_ALLOCATOR(MemoryPool_Default, MC_ResourceBuffer) > CBundleJSONArrayRef;

			//////////////////////////////////////////////////////////////////////////
			// SBundleFileDesc
			// - Data containment for each file in the bundle desc.
			//////////////////////////////////////////////////////////////////////////
			struct SBundleFileDesc
			{
				static const Uint32 RESOURCE_DEPENDENCY_DATA_SIZE = ( sizeof( ResourceManagement::CResourceId ) * Bundle::SBundleHeaderItem::MAX_DEPENDENCY_ITEMS );
				
				//////////////////////////////////////////////////////////////////////////
				// Constructors
				//////////////////////////////////////////////////////////////////////////
				SBundleFileDesc()
					: m_resourcePath( StringAnsi::EMPTY )
					, m_compressionType( Bundle::CT_Default )
					, m_fourCC( Uint32( -1 ) )
				{
				}
				
				SBundleFileDesc( const String& path, Bundle::ECompressionType compression = Bundle::CT_Default )
				{
					Populate( path, compression );
				}

				SBundleFileDesc( const StringAnsi& path, Bundle::ECompressionType compression = Bundle::CT_Default )
				{
					Populate( path, compression );
				}

				SBundleFileDesc( const SBundleFileDesc& other)
					: m_resourceId( other.m_resourceId )
					, m_resourcePath( other.m_resourcePath )
					, m_resolvedResourcePath( other.m_resolvedResourcePath )
					, m_compressionType( other.m_compressionType )
					, m_fourCC( other.m_fourCC )
				{
				}

				//////////////////////////////////////////////////////////////////////////
				// Equality Operator
				//////////////////////////////////////////////////////////////////////////
				Bool operator==( const SBundleFileDesc& other ) const
				{
					return
					(
						m_resourcePath == other.m_resourcePath &&
						m_compressionType == other.m_compressionType &&
						m_fourCC == other.m_fourCC && 
						m_resourceId == other.m_resourceId
					);
				}

				Bool operator==( SBundleFileDesc& other ) const
				{
					return
					(
						m_resourcePath == other.m_resourcePath &&
						m_compressionType == other.m_compressionType &&
						m_fourCC == other.m_fourCC && 
						m_resourceId == other.m_resourceId
					);
				}

				//////////////////////////////////////////////////////////////////////////
				// Helper functions
				void Populate( const String& path, Bundle::ECompressionType compression = Bundle::CT_Auto );
				void Populate( const StringAnsi& path, Bundle::ECompressionType compression = Bundle::CT_Auto );

				//////////////////////////////////////////////////////////////////////////
				// New / Delete operators
				//////////////////////////////////////////////////////////////////////////
				DEFINE_BUNDLE_NEWDELETE_OPERATORS;

				//////////////////////////////////////////////////////////////////////////
				// Static Data
				// Used to aid reading
				static const Red::System::AnsiChar*			FOURCC_STR;
				static const Red::System::AnsiChar*			RESOURCEID_STR;
				static const Red::System::AnsiChar*			PATH_STR;
				static const Red::System::AnsiChar*			COMPRESSIONTYPE_STR;

				//////////////////////////////////////////////////////////////////////////
				// Data
				ResourceManagement::CResourceId				m_resourceId;
				StringAnsi									m_resourcePath;
				StringAnsi									m_resolvedResourcePath;
				Bundle::ECompressionType					m_compressionType;
				Uint32										m_fourCC;
			};

			

			//////////////////////////////////////////////////////////////////////////
			// CBundleDataContainer
			// - Stores the bundle name will be.
			// - Store SBundleFileDesc's, which directly relate to files we wish
			// to bundle.
			// - Deals with writing JSON data for the bundle definition.
			//////////////////////////////////////////////////////////////////////////
			class CBundleDataContainer
			{
			public:
				//////////////////////////////////////////////////////////////////////////
				// Constructors
				CBundleDataContainer( const StringAnsi& bundleName );
				CBundleDataContainer( const CBundleDataContainer& other );
				
				//////////////////////////////////////////////////////////////////////////
				// Destructor
				~CBundleDataContainer();

				//////////////////////////////////////////////////////////////////////////
				// Public Methods

				// Add a SBundleFileDesc to the bundle.
				// NOTE: We make a copy, so feel free to make your objects
				// on the stack.
				void AddBundleFileDesc( const SBundleFileDesc& bundleFileDesc );

				// Returns bundles name.
				const AnsiChar* GetBundleName() const
				{
					return m_bundleName.AsChar();
				}

				// Returns the number of files which have been added to this bundle.
				Uint32 GetFileCount() const
				{
					return m_bundleFiles.Size();
				}

				//
				const SBundleFileDesc* GetBundleFileDescription( const Uint32 index ) const
				{
					return m_bundleFiles[index];
				}

				SBundleFileDesc* GetBundleFileDescription( const Uint32 index ) 
				{
					return m_bundleFiles[ index ];
				}

				Bool Write( CBundleJSONWriter& writer ) const;
				void WriteResourceId( CBundleJSONWriter& writer, const ResourceManagement::CResourceId& resourceId ) const;

				DEFINE_BUNDLE_NEWDELETE_OPERATORS;

			private:
				CBundleDataContainer& operator=( const CBundleDataContainer& other )
				{
					RED_UNUSED( other );
					return *this;
				}

				const StringAnsi m_bundleName;
				TDynArray< SBundleFileDesc*, MC_ResourceBuffer > m_bundleFiles;
				THashSet< Red::Core::ResourceManagement::CResourceId > m_existingEntries;
			};

			//////////////////////////////////////////////////////////////////////////
			// IBundleDefinition
			// - Data consistency between reader and writer.
			//////////////////////////////////////////////////////////////////////////
			
			typedef THashMap< StringAnsi, CBundleDataContainer*, DefaultHashFunc< StringAnsi >, DefaultEqualFunc< StringAnsi >, MC_ResourceBuffer > TBundleDataContainers;

			class IBundleDefinition
			{
			protected:
				struct Data
				{
					StringAnsi						m_jsonFilename;
					TBundleDataContainers			m_bundleData;
				};
				Data m_data;

			protected:
				IBundleDefinition( const AnsiChar* jsonFile )
				{
					m_data.m_jsonFilename = jsonFile;
				}

				virtual ~IBundleDefinition()
				{
				}

			public:
				IBundleDefinition( IBundleDefinition&& other )
				{
					m_data.m_jsonFilename = std::move( other.m_data.m_jsonFilename );
					m_data.m_bundleData = std::move( other.m_data.m_bundleData );
				}

				Bool AddBundle( const StringAnsi& name );
				Bool AddBundleFileDesc( const StringAnsi& name, const SBundleFileDesc& fileDesc );

				Bool RemoveBundle( const StringAnsi& name );

				CBundleDataContainer* Find( const StringAnsi& name );

				Uint32 GetBundleCount() const
				{
					return m_data.m_bundleData.Size();
				}

				void GetBundleNames( TDynArray< StringAnsi >& bundleNames ) const
				{
					return m_data.m_bundleData.GetKeys( bundleNames );
				}

				const TBundleDataContainers& GetBundles() const;
			};

			//////////////////////////////////////////////////////////////////////////
			// CBundleDefinitionWriter
			// - Writes JSON string data to a file describing how the bundle(s)
			// should look.
			//////////////////////////////////////////////////////////////////////////
			class CBundleDefinitionWriter : public IBundleDefinition
			{
			public:
				CBundleDefinitionWriter( const AnsiChar* jsonFile );
				CBundleDefinitionWriter( IBundleDefinition&& other )
					:	IBundleDefinition( std::move( other ) )
				{

				}

				virtual ~CBundleDefinitionWriter();

				Bool Write() const;
			};

			//////////////////////////////////////////////////////////////////////////
			// CBundleDefinitionReader
			// - Reads data from a JSON file, and create the correct storage
			// structure.
			//////////////////////////////////////////////////////////////////////////
			class CBundleDefinitionReader : public IBundleDefinition
			{

			public:
				enum EErrorCode
				{
					EC_Success = 0,
					EC_FileNotFound,
					EC_InvalidJSON,
					EC_InvalidBundle
				};

			public:
				CBundleDefinitionReader( const AnsiChar* jsonFile );
				CBundleDefinitionReader( IBundleDefinition&& other )
				:	IBundleDefinition( std::move( other ) )
				{

				}
				virtual ~CBundleDefinitionReader();
				
				EErrorCode Read();

			private:

				void* ReadFile( const AnsiChar* filename ) const;

				Bool ReadType( const CBundleJSONBasicRef& value );
				Bool CreateBundleFileDesc( const StringAnsi& name, const CBundleJSONBasicRef& value );
				void ReadResourceId( ResourceManagement::CResourceId& resourceId, const CBundleJSONArrayRef& valArray );
			};

			//////////////////////////////////////////////////////////////////////////
			// CBundleDefinitionFilePathResolver
			// - Passed an existing bundle definition, searches for the cooked data files
			//	and converts paths to absolute
			//////////////////////////////////////////////////////////////////////////
			class CBundleDefinitionFilePathResolver : public IBundleDefinition
			{
			public:
				CBundleDefinitionFilePathResolver( IBundleDefinition&& sourceData, const AnsiChar* baseDirectory );
				virtual ~CBundleDefinitionFilePathResolver();

			private:
				StringAnsi BuildAbsolutePathToFile( const StringAnsi& srcFile, const AnsiChar* directoryList );
			};
		}
	}
}

#endif //_RED_BUNDLE_DEFINITION_H_

#endif // ! NO_EDITOR
