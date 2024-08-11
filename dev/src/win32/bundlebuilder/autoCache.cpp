/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "autoCache.h"

#include "bundleBuilderMemory.h"
#include "../../common/core/bundledefinition.h"

#include "../../../external/rapidjson/include/rapidjson/prettywriter.h"
#include "../../../external/rapidjson/include/rapidjson/filestream.h"
#include "../../../external/rapidjson/include/rapidjson/document.h"

namespace rapidjson
{
	template < typename T > struct UTF8;
	template < typename BaseAllocator > class MemoryPoolAllocator;
	template < typename Stream, typename Encoding, typename Allocator > class PrettyWriter;
	template < typename Encoding, typename Allocator > class GenericDocument;
	template < typename Encoding, typename Allocator > class GenericValue;
	class FileStream;
};

namespace Bundler { namespace AutoCache {

Bundle::Bundle()
{

}

Bundle::~Bundle()
{

}

void Bundle::Initialize( const StringAnsi& bundleName )
{
	m_bundleName = bundleName;
}

void Bundle::AddResult( const StringAnsi& id, CompressionType compression )
{
	m_results.Set( id, compression );
}

Bool Bundle::GetResult( const StringAnsi& id, CompressionType& compression )
{
	return m_results.Find( id, compression );
}

//////////////////////////////////////////////////////////////////////////

class Definition::Allocator
{
public:
	RED_INLINE void* Malloc( MemSize size )
	{
		return BUNDLER_MEMORY_ALLOCATE( MC_BundlerAutoCache, size );
	}

	RED_INLINE void* Realloc( void* originalPtr, MemSize, MemSize newSize )
	{
		return BUNDLER_MEMORY_REALLOCATE( originalPtr, MC_BundlerAutoCache, newSize );
	}

	static void Free( void* ptr )
	{
		BUNDLER_MEMORY_FREE( MC_BundlerAutoCache, ptr );
	}
};

//////////////////////////////////////////////////////////////////////////

const AnsiChar* Definition::HEADER = "auto compression cache";

Definition::Definition()
:	m_firstFreeIndex( 0 )
{

}

Definition::~Definition()
{

}

void Definition::Initialize( Uint32 numBundles )
{
	m_results.Resize( numBundles );
}

Bundle& Definition::MapResults( const StringAnsi& bundleName )
{
	RED_FATAL_ASSERT( m_firstFreeIndex < m_results.Size(), "No more free slots" );

	Bundle& result = m_results[ m_firstFreeIndex ];
	result.Initialize( bundleName );
	m_nameToBundleIndex.Set( bundleName, m_firstFreeIndex );
	++m_firstFreeIndex;
	
	return result;
}

Bundle& Definition::GetResults( const StringAnsi& bundleName )
{
	Uint32 index;
	if( m_nameToBundleIndex.Find( bundleName, index ) )
	{
		return m_results[ index ];
	}

	return MapResults( bundleName );
}

void* Definition::Open( const AnsiChar* filepath )
{
	FILE* file = fopen( filepath, "r" );

	if( file )
	{
		// Calculate file size
		fseek( file, 0, SEEK_END );
		Red::System::MemSize size = ftell( file );

		// Create buffer to read into
		void* jsonFileData = BUNDLER_MEMORY_ALLOCATE( MC_BundlerAutoCache, size );

		// Read file into buffer
		fseek( file, 0, SEEK_SET );
		fread( jsonFileData, 1, size, file );

		// Close file handle
		fclose( file );

		return jsonFileData;
	}

	return nullptr;
}

void Definition::Load( const AnsiChar* filepath, const TDynArray< StringAnsi >& bundleNames )
{
	void* jsonData = Open( filepath );

	if( !jsonData )
	{
		return;
	}

	typedef rapidjson::GenericDocument< rapidjson::UTF8< AnsiChar >, rapidjson::MemoryPoolAllocator< Definition::Allocator > > JSONDocument;
	typedef rapidjson::GenericValue< rapidjson::UTF8< AnsiChar >, rapidjson::MemoryPoolAllocator< Definition::Allocator > > JSONValue;

	rapidjson::MemoryPoolAllocator< Definition::Allocator > allocator;
	JSONDocument jsonDocument( &allocator );
	jsonDocument.Parse< 0 >( static_cast< AnsiChar* >( jsonData ) );

	if( jsonDocument.HasParseError() )
	{
		return;
	}

	if( !jsonDocument.HasMember( HEADER ) )
	{
		return;
	}

	JSONValue& bundlesArray = jsonDocument[ HEADER ];
	if( !bundlesArray.IsArray() )
	{
		return;
	}

	const Uint32 BundlesArraySize = bundlesArray.Size();
	RED_FATAL_ASSERT( BundlesArraySize % 2 == 0, "Pair mismatch, should be 1 bundle name to 1 array of data!" );

	Uint32 iBundleElement = 0;

	while( iBundleElement < BundlesArraySize )
	{
		const JSONValue& nameParam = bundlesArray[ iBundleElement ];
		RED_FATAL_ASSERT( nameParam.IsString(), "Expected Bundle name" );

		StringAnsi name = nameParam.GetString();
		if ( !bundleNames.Exist( name ) ) {
			iBundleElement+= 2;
			continue;
		}

		++iBundleElement;

		RED_FATAL_ASSERT( !m_nameToBundleIndex.KeyExist( name ), "Same bundle encounted multiple times!" );

		Bundle& bundle = MapResults( name );

		const JSONValue& dataParam = bundlesArray[ iBundleElement ];
		RED_FATAL_ASSERT( dataParam.IsArray(), "Expected data for bundle: %hs", name.AsChar() );

		const Uint32 numResults = dataParam.Size();
		RED_FATAL_ASSERT( numResults % 2 == 0, "Pair mismatch, should be 1 id to 1 compression type!" );

		Uint32 iResultElement = 0;
		while( iResultElement < numResults )
		{
			const JSONValue& idParam = dataParam[ iResultElement ];

			StringAnsi id = idParam.GetString();

			++iResultElement;

			const JSONValue& compressionTypeParam = dataParam[ iResultElement ];

			Int32 compressionType = compressionTypeParam.GetInt();

			++iResultElement;

			bundle.AddResult( id, static_cast< CompressionType >( compressionType ) );
		}

		++iBundleElement;
	}

	BUNDLER_MEMORY_FREE( MC_BundlerAutoCache, jsonData );
}

void Definition::Save( const AnsiChar* filepath ) const
{
	RED_VERIFY( GFileManager->SetFileReadOnly( ANSI_TO_UNICODE( filepath ), false ) );
	FILE* file = fopen( filepath, "w" );

	if( file )
	{
		typedef rapidjson::PrettyWriter< rapidjson::FileStream, rapidjson::UTF8< AnsiChar >, rapidjson::MemoryPoolAllocator< Definition::Allocator > > RapidJSONWriter;

		rapidjson::FileStream stream( file );
		rapidjson::MemoryPoolAllocator< Definition::Allocator > allocator;

		RapidJSONWriter writer( stream, &allocator );

		writer.StartObject();
		{
			writer.String( HEADER );

			writer.StartArray();
			for( Uint32 i = 0; i < m_results.Size(); ++i )
			{
				const Bundle& bundle = m_results[ i ];

				writer.String( bundle.GetBundleName().AsChar() );

				writer.StartArray();
				for( auto result = bundle.Begin(); result != bundle.End(); ++result )
				{
					writer.String( result->m_first.AsChar() );
					writer.Int( result->m_second );
				}
				writer.EndArray();
			}
			writer.EndArray();
		}
		writer.EndObject();

		// Force null termination at the end of the file.
		// The JSON parser gets upset if it doesn't find this at the end of the stream.
		stream.Put( '\0' );

		fclose( file );
	}
}

void Definition::Normalize()
{
	THashMap< StringAnsi, CompressionType > visitedIds;
	
	const Uint32 numResults = m_results.Size();
	for( Uint32 i = 0; i < numResults; ++i )
	{
		Bundle& bundle = m_results[ i ];
		for( THashMap< StringAnsi, CompressionType >::iterator iter = bundle.Begin(); iter != bundle.End(); ++iter )
		{
			CompressionType* compressionType = visitedIds.FindPtr( iter->m_first );

			if( compressionType )
			{
				iter->m_second = *compressionType;
			}
			else
			{
				visitedIds.Insert( iter->m_first, iter->m_second );
			}
		}
	}
}

} } // namespace Bundler {  namespace AutoCache {
