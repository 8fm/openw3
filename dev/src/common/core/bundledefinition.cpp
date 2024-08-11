#include "build.h"

#ifndef NO_EDITOR

#include "bundledefinition.h"
#include "filePath.h"
#include "../../../external/rapidjson/include/rapidjson/prettywriter.h"
#include "../../../external/rapidjson/include/rapidjson/filestream.h"
#include "../../../external/rapidjson/include/rapidjson/document.h"


#define BUNDLE_DEBUG_LOG_VERBOSE 0
#if BUNDLE_DEBUG_LOG_VERBOSE
	#define BUNDLE_LOG( message, ... ) RED_LOG( RED_LOG_CHANNEL(BundleBuilding), message, ##__VA_ARGS__ )
#else
	#define BUNDLE_LOG( message, ... ) {}
#endif

namespace Red { namespace Core { namespace BundleDefinition {

typedef CJSONFileReader< rapidjson::UTF8<Red::System::AnsiChar>, RED_JSON_MEMORY_GET_ALLOCATOR(MemoryPool_Default, MC_ResourceBuffer) > CBundleJSONFileReader;
typedef CJSONValueRef< rapidjson::UTF8<Red::System::AnsiChar>, RED_JSON_MEMORY_GET_ALLOCATOR(MemoryPool_Default, MC_ResourceBuffer) >   CBundleJSONValueRef;
typedef CJSONObjectRef< rapidjson::UTF8<Red::System::AnsiChar>, RED_JSON_MEMORY_GET_ALLOCATOR(MemoryPool_Default, MC_ResourceBuffer) >  CBundleJSONObjectRef;

using ResourceManagement::CResourceId;

//////////////////////////////////////////////////////////////////////////
// Constant strings used for comparisons when reading bundle definitions
//////////////////////////////////////////////////////////////////////////
const Red::System::AnsiChar* SBundleFileDesc::FOURCC_STR				= "FourCC";
const Red::System::AnsiChar* SBundleFileDesc::RESOURCEID_STR			= "ID";
const Red::System::AnsiChar* SBundleFileDesc::PATH_STR					= "Path";
const Red::System::AnsiChar* SBundleFileDesc::COMPRESSIONTYPE_STR		= "Compression";

//////////////////////////////////////////////////////////////////////////
// CBundleDataContainer
//////////////////////////////////////////////////////////////////////////
CBundleDataContainer::CBundleDataContainer( const StringAnsi& bundleName )
	: m_bundleName( bundleName )
{
}

//////////////////////////////////////////////////////////////////////////
CBundleDataContainer::CBundleDataContainer( const CBundleDataContainer& other )
	: m_bundleName( other.m_bundleName )
	, m_bundleFiles( other.m_bundleFiles )
{

}

//////////////////////////////////////////////////////////////////////////
CBundleDataContainer::~CBundleDataContainer()
{
	// Give back the memory which was allocated by adding files to the bundle.
	m_bundleFiles.ClearPtr();
}


//////////////////////////////////////////////////////////////////////////
Bool CBundleDataContainer::Write( CBundleJSONWriter& writer ) const
{
	const Red::System::AnsiChar* bundleName = m_bundleName.AsChar();
	writer.WriteString(bundleName, m_bundleName.GetLength() );
	const Uint32 fileCount = m_bundleFiles.Size();
	writer.StartArray();
	{
		for( Uint32 i = 0; i < fileCount; ++i )
		{
			writer.StartObject();
			{
				writer.WriteString( SBundleFileDesc::FOURCC_STR );
				writer.WriteUint32( m_bundleFiles[i]->m_fourCC );
				writer.WriteString( SBundleFileDesc::RESOURCEID_STR );
				WriteResourceId( writer, m_bundleFiles[i]->m_resourceId );
				writer.WriteString( SBundleFileDesc::PATH_STR );
				writer.WriteString( m_bundleFiles[i]->m_resourcePath.AsChar() );
				writer.WriteString( SBundleFileDesc::COMPRESSIONTYPE_STR );
				writer.WriteUint32( m_bundleFiles[i]->m_compressionType );
			}
			writer.EndObject();
		}
	}
	writer.EndArray();
	return true;
}

void CBundleDataContainer::WriteResourceId( CBundleJSONWriter& writer, const CResourceId& resourceId ) const
{
	writer.StartArray();
	{
		for( Uint32 i = 0; i < CResourceId::NUM_PARTS_64; ++i )
		{
			writer.WriteUint64( resourceId[ i ] );
		}
	}
	writer.EndArray();
}

//////////////////////////////////////////////////////////////////////////
void CBundleDataContainer::AddBundleFileDesc( const SBundleFileDesc& bundleFileDesc )
{
	if ( m_existingEntries.Exist( bundleFileDesc.m_resourceId ) )
		return;

	SBundleFileDesc* fileDescCopy = new SBundleFileDesc( bundleFileDesc );
	m_bundleFiles.PushBack( fileDescCopy );
	m_existingEntries.Insert( bundleFileDesc.m_resourceId );
}

//////////////////////////////////////////////////////////////////////////
// IBundleDefinition
//////////////////////////////////////////////////////////////////////////
CBundleDataContainer* IBundleDefinition::Find( const StringAnsi& name )
{
	CBundleDataContainer** ptr = m_data.m_bundleData.FindPtr( name );

	if( ptr )
	{
		return *ptr;
	}

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////
Bool IBundleDefinition::AddBundle( const StringAnsi& name )
{
	BUNDLE_LOG( TXT( "Add Bundle: %hs" ), name.AsChar() );

	CBundleDataContainer* container = new CBundleDataContainer( name );
	m_data.m_bundleData.Insert( name, container );

	return true;
}

//////////////////////////////////////////////////////////////////////////
Bool IBundleDefinition::AddBundleFileDesc( const StringAnsi& name, const SBundleFileDesc& bundleFileDesc )
{
	CBundleDataContainer* container = Find( name );

	if( container )
	{
		container->AddBundleFileDesc( bundleFileDesc );

		return true;
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////
Bool IBundleDefinition::RemoveBundle( const StringAnsi& name )
{
	BUNDLE_LOG( TXT( "Remove Bundle: %hs" ), name.AsChar() );

	return m_data.m_bundleData.Erase( name );
}

//////////////////////////////////////////////////////////////////////////
const TBundleDataContainers& IBundleDefinition::GetBundles() const 
{
	return m_data.m_bundleData;
}

//////////////////////////////////////////////////////////////////////////
// CBundleDefinitionWriter
//////////////////////////////////////////////////////////////////////////
CBundleDefinitionWriter::CBundleDefinitionWriter( const AnsiChar* jsonFile )
	: IBundleDefinition( jsonFile )
{
}

//////////////////////////////////////////////////////////////////////////
CBundleDefinitionWriter::~CBundleDefinitionWriter()
{
	m_data.m_bundleData.ClearPtr();
}

//////////////////////////////////////////////////////////////////////////
Bool CBundleDefinitionWriter::Write() const
{
	Bool isSuccessful = false;
	
	// Create the file.
	IFile* pFile = GFileManager->CreateFileWriter( String( ANSI_TO_UNICODE( m_data.m_jsonFilename.AsChar() ) ), FOF_AbsolutePath );
	if( !pFile )
	{
		return isSuccessful;
	}
	
	CBundleJSONWriter writer( *pFile );

	writer.StartObject();
	{
		writer.WriteString( "bundles" );
		writer.StartArray();
		{
			for( auto iter = m_data.m_bundleData.Begin(); iter != m_data.m_bundleData.End(); ++iter )
			{
				iter->m_second->Write( writer );
			}
		}
		writer.EndArray();
	}
	writer.EndObject();

	writer.Flush();

	delete pFile;

	isSuccessful = true;
	
	return isSuccessful;
}

//////////////////////////////////////////////////////////////////////////
// CBundleDefinitionReader
//////////////////////////////////////////////////////////////////////////
CBundleDefinitionReader::CBundleDefinitionReader( const AnsiChar* jsonFile )
	: IBundleDefinition( jsonFile )
{
}

//////////////////////////////////////////////////////////////////////////
CBundleDefinitionReader::~CBundleDefinitionReader()
{

}

//////////////////////////////////////////////////////////////////////////
CBundleDefinitionReader::EErrorCode CBundleDefinitionReader::Read()
{
	// Read the file.
	IFile* pFile = GFileManager->CreateFileReader( String( ANSI_TO_UNICODE( m_data.m_jsonFilename.AsChar() ) ), FOF_AbsolutePath );
	if( !pFile )
	{
		return EC_FileNotFound;
	}

	CBundleJSONFileReader reader( pFile ); //! pFile - will be destroyed

	if( reader.HasParseError() )
	{
		return EC_InvalidJSON;
	}

	if( !reader.GetDocument().HasMember("bundles") )
	{
		return EC_InvalidBundle;
	}

	CBundleJSONBasicRef jsonRef = reader.GetDocument().GetMember( "bundles" );
	if( jsonRef.GetType() != JSON_Array )
	{
		return EC_InvalidBundle;
	}

	CBundleJSONArrayRef bundlesArray( jsonRef );

	BUNDLE_LOG( TXT("Bundles ArraySize: %d" ), bundlesArray.Size() );

	StringAnsi currentContainer;
	const MemSize BundlesArraySize = bundlesArray.Size();
	for( rapidjson::SizeType i = 0; i < BundlesArraySize; ++i )
	{
		CBundleJSONBasicRef jsonRef( bundlesArray.GetMemberAt( i ) );

		// If it's a string at this level, we have a new container
		if( jsonRef.GetType() == JSON_String )
		{
			CBundleJSONValueRef value( jsonRef );
			currentContainer = value.GetString();
			AddBundle( currentContainer );
		}

		// Contents for the current container
		else if( jsonRef.GetType() == JSON_Array )
		{
			CBundleJSONArrayRef containerArray( jsonRef );
			for( MemSize iContainerElement = 0; iContainerElement < containerArray.Size(); ++iContainerElement )
			{
				CreateBundleFileDesc( currentContainer, containerArray.GetMemberAt( iContainerElement ) );
			}
		}
	}

	return EC_Success;
}

void* CBundleDefinitionReader::ReadFile( const AnsiChar* filename ) const
{
	FILE* pFile = fopen( filename, "r" );

	if( !pFile )
	{
		// File doesn't exist
		return nullptr;
	}

	// Calculate file size
	fseek( pFile, 0, SEEK_END );
	Red::System::MemSize size = ftell( pFile );

	// Create buffer to read into
	void* jsonFileData = RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_ResourceBuffer, size );

	// Read file into buffer
	fseek( pFile, 0, SEEK_SET );
	fread( jsonFileData, 1, size, pFile );

	// Close file handle
	fclose( pFile );

	return jsonFileData;
}

//////////////////////////////////////////////////////////////////////////
Bool CBundleDefinitionReader::CreateBundleFileDesc( const StringAnsi& name, const CBundleJSONBasicRef& jsonRef )
{
	SBundleFileDesc bundleFileDesc;
	Bool isValid = true;
	if( jsonRef.GetType() != JSON_Object )
	{
		return !isValid;
	}

	CBundleJSONObjectRef jsonObjectRef( jsonRef );

	THashMap< TString<CBundleJSONBasicRef::CharType>, CBundleJSONBasicRef > members;
	jsonObjectRef.GetMembers( members );

	THashMap< TString<CBundleJSONBasicRef::CharType>, CBundleJSONBasicRef >::const_iterator end = members.End();
	for( THashMap< TString<CBundleJSONBasicRef::CharType>, CBundleJSONBasicRef >::const_iterator  it = members.Begin(); it != end; ++it  )
	{
		const CBundleJSONBasicRef::CharType* currentMemberName = it->m_first.AsChar();
		const CBundleJSONBasicRef& currentMember =  it->m_second;
		
		if( currentMemberName == NULL )
		{
			isValid = false;
			RED_ASSERT( currentMemberName != NULL , TXT( "Member names MUST be a string." ) );
			break;
		}
		
		if( currentMember.GetType() == JSON_Number )
		{
			CBundleJSONValueRef jsonValRef( currentMember );
			if( jsonValRef.IsUint32() )
			{
				// Read FourCC.
				if( Red::System::StringCompare( currentMemberName, SBundleFileDesc::FOURCC_STR ) == 0 )
				{
					bundleFileDesc.m_fourCC = jsonValRef.GetUint32();
					BUNDLE_LOG
					(
						TXT( "FourCC: %u - %c%c%c%c" ),
						bundleFileDesc.m_fourCC, 
						RESOURCE_FOURCC_GETELEMENTA( bundleFileDesc.m_fourCC ), 
						RESOURCE_FOURCC_GETELEMENTB( bundleFileDesc.m_fourCC ), 
						RESOURCE_FOURCC_GETELEMENTC( bundleFileDesc.m_fourCC ), 
						RESOURCE_FOURCC_GETELEMENTD( bundleFileDesc.m_fourCC )
					);
				}
				// Read CompressionType.
				else if( Red::System::StringCompare( currentMemberName, SBundleFileDesc::COMPRESSIONTYPE_STR ) == 0 )
				{
					bundleFileDesc.m_compressionType = static_cast< Bundle::ECompressionType >( jsonValRef.GetUint32() );
					BUNDLE_LOG( TXT( "CompressionType: %u" ), bundleFileDesc.m_compressionType );
				}
			}
		}
		else if( currentMember.GetType() == JSON_String )
		{
			CBundleJSONValueRef jsonValRef( currentMember );

			// Read Origin Path.
			if( Red::System::StringCompare( currentMemberName, SBundleFileDesc::PATH_STR ) == 0 )
			{
				bundleFileDesc.m_resourcePath = jsonValRef.GetString();
				BUNDLE_LOG( TXT( "OriginPath: %hs" ), bundleFileDesc.m_resourcePath.AsChar() );
			}
		}
		else if( currentMember.GetType() == JSON_Array )
		{
			CBundleJSONArrayRef jsonArrayRef( currentMember );

			// Read ResourceID.
			if( Red::System::StringCompare( currentMemberName, SBundleFileDesc::RESOURCEID_STR ) == 0 )
			{
				ReadResourceId( bundleFileDesc.m_resourceId, jsonArrayRef );
				BUNDLE_LOG( TXT( "ResourceID: %lu:%lu" ), bundleFileDesc.m_resourceId[ 0 ], bundleFileDesc.m_resourceId[ 1 ] );
			}
		}
		else
		{
			// UNKNOWN TYPE.
			isValid = false;
			BUNDLE_LOG( TXT("Tried to parse an unknown type.") );
			break;
		}		
	}	

	if( isValid )
	{
		isValid = AddBundleFileDesc( name, bundleFileDesc );
	}

	return isValid;
}

void CBundleDefinitionReader::ReadResourceId( CResourceId& resourceId, const CBundleJSONArrayRef& valArray )
{
	RED_ASSERT( valArray.GetType() == JSON_Array, TXT( "CResourceId data expects an array of two unsigned 64 bit integers" ) );
	RED_ASSERT( valArray.Size() == CResourceId::NUM_PARTS_64, TXT( "CResourceId data expects an array of two unsigned 64 bit integers, but this array has %u" ), valArray.Size() );

	for( Uint32 iPart = 0; iPart < CResourceId::NUM_PARTS_64; ++iPart )
	{
		resourceId[ iPart ] = CBundleJSONValueRef( valArray.GetMemberAt( iPart ) ).GetUint64();
	}
}

//////////////////////////////////////////////////////////////////////////
Bool CBundleDefinitionReader::ReadType( const CBundleJSONBasicRef& jsonRef )
{
	EJSONType currentType = jsonRef.GetType();
	switch( currentType )
	{
	case JSON_Null: //!< null
		BUNDLE_LOG( TXT("Null Type") );
		break;

	case JSON_False: //!< false
		BUNDLE_LOG( TXT("False Type") );
		break;

	case JSON_True: //!< true
		BUNDLE_LOG( TXT("True Type") );
		break;

	case JSON_Object: //!< object
		BUNDLE_LOG( TXT("Object Type") );
		{
			CBundleJSONObjectRef jsonObjectRef( jsonRef );

			THashMap< TString<CBundleJSONBasicRef::CharType>, CBundleJSONBasicRef > members;
			jsonObjectRef.GetMembers( members );

			THashMap< TString<CBundleJSONBasicRef::CharType>, CBundleJSONBasicRef >::const_iterator end = members.End();
			for( THashMap< TString<CBundleJSONBasicRef::CharType>, CBundleJSONBasicRef >::const_iterator  it = members.Begin(); it != end; ++it  )
			{
				LOG_CORE( TXT("STRING VALUE: %hs "), it->m_first.AsChar() );
				ReadType(  it->m_second );
			}
		}
		break;

	case JSON_Array: //!< array 
		BUNDLE_LOG( TXT("Array Type") );		
		{
			CBundleJSONArrayRef jsonArrayRef( jsonRef );
			for( MemSize i = 0; i < jsonArrayRef.Size(); ++i )
			{
				ReadType( jsonArrayRef.GetMemberAt( i ) );
			}
		}
		break;

	case JSON_String: //!< string
		BUNDLE_LOG( TXT("String Type") );		
		{
			CBundleJSONValueRef jsonValueRef( jsonRef );
			LOG_CORE( TXT("STRING VALUE: %hs "), jsonValueRef.GetString() );
		}
		break;

	case JSON_Number: //!< number
		BUNDLE_LOG( TXT("Number Type") );
		{
			CBundleJSONValueRef jsonValueRef( jsonRef );
			if( jsonValueRef.IsUint32() || jsonValueRef.IsInt32() )
			{
				LOG_CORE( TXT( "NUMBER VALUE: %u" ), jsonValueRef.GetUint32() );
			}
		}		
		break;

	default:
		BUNDLE_LOG( TXT("UNKNOWN!") );	
		return false;
		break;
	}
	
	return true;
}

// This thing only works on Windows since we shouldn't be using Core stuff
CBundleDefinitionFilePathResolver::CBundleDefinitionFilePathResolver( IBundleDefinition&& sourceData, const AnsiChar* baseDirectory )
	: IBundleDefinition( std::move( sourceData ) )
{
	// Go through each bundle
	for( auto bundleIt = m_data.m_bundleData.Begin(); bundleIt != m_data.m_bundleData.End(); ++bundleIt )
	{
		CBundleDataContainer* bundleContainer = bundleIt.Value();
		Uint32 entryCount = bundleContainer->GetFileCount();
		for( Uint32 entryIndex=0; entryIndex < entryCount; ++entryIndex )
		{
			SBundleFileDesc* fileDescriptor = bundleContainer->GetBundleFileDescription( entryIndex );
			fileDescriptor->m_resolvedResourcePath = BuildAbsolutePathToFile( fileDescriptor->m_resourcePath, baseDirectory );
		}
	}
}

StringAnsi CBundleDefinitionFilePathResolver::BuildAbsolutePathToFile( const StringAnsi& srcFile, const AnsiChar* baseDirectory )
{
	return StringAnsi( baseDirectory ) + srcFile;
}

CBundleDefinitionFilePathResolver::~CBundleDefinitionFilePathResolver()
{

}

////////////////////////////////////////////////////////////////////////////////
void SBundleFileDesc::Populate( const StringAnsi& path,Bundle::ECompressionType compression )
{
	RED_ASSERT( !path.Empty(), TXT( "No file name passed. Bundle file desc will be invalid" ) );

	m_resourceId = ResourceManagement::CResourceId( path );
	m_resourcePath = path.AsChar();
	m_compressionType = compression;
	m_fourCC = 0;
}

void SBundleFileDesc::Populate( const String& path, Bundle::ECompressionType compression )
{
	const StringAnsi convertedName = UNICODE_TO_ANSI( path.AsChar() );
	Populate( convertedName, compression );
}

} } } // namespace Red { namespace Core { namespace BundleDefinition {

#endif // ! NO_EDITOR
