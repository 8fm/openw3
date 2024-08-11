/**
* Copyright c 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "../../common/core/depot.h"
#include "../../common/core/objectGC.h"
#include "../../common/core/objectDiscardList.h"
#include "../../common/core/commandlet.h"
#include "../../common/core/dependencyLinkerFactory.h"
#include "../../common/core/dependencyFileTables.h"
#include "../../common/core/dependencyLoader.h"
#include "../../common/core/memoryFileReader.h"
#include "../../common/core/fileSystemProfiler.h"
#include "../../common/core/fileSystemProfilerWrapper.h"

#include "../../common/engine/sectorData.h"

#include "reportWriter.h"

/// File dumper - loads the file, deserializes it and dumps the loaded objects
class CFileDumpCommandlet : public ICommandlet
{
	DECLARE_ENGINE_CLASS( CFileDumpCommandlet, ICommandlet, 0 );

public:
	CFileDumpCommandlet();
	~CFileDumpCommandlet();

	// ICommandlet interface
	virtual const Char* GetOneLiner() const { return TXT("Dump file content (objects)"); }
	virtual bool Execute( const CommandletOptions& options );
	virtual void PrintHelp() const;

private:
	String	m_outPath;
	Bool m_directLoad;
	THashSet< String > m_excludedExtensions;

	void ProcessFile( const String& basePath, const String& filePath );

	void MapObjects( const void* objectData, const CClass* objectClass, THashMap< const void*, Int32 >& visitedObjects, const THashSet< const CObject* >& allowedRoots );
	void MapObjectData( const void* objectData, const CClass* objectClass, THashMap< const void*, Int32 >& visitedObjects, const THashSet< const CObject* >& allowedRoots  );
	void MapProperty( const void* propData, const IRTTIType* propType, THashMap< const void*, Int32 >& visitedObjects, const THashSet< const CObject* >& allowedRoots  );
	void DumpObject( CHTMLNode& node, const void* objectData, const CClass* objectClass, const THashMap< const void*, Int32 >& visitedObjects );
	void DumpObjectData( CHTMLNode& node, const void* objectData, const CClass* objectClass, const THashMap< const void*, Int32 >& visitedObjects );
	void DumpProperty( CHTMLNode& node, const void* propData, const IRTTIType* propType, const THashMap< const void*, Int32 >& visitedObjects );

	void DumpTables( CHTMLNode& node, const CDependencyFileData& fileData );
};

BEGIN_CLASS_RTTI( CFileDumpCommandlet )
	PARENT_CLASS( ICommandlet )
END_CLASS_RTTI()

IMPLEMENT_ENGINE_CLASS( CFileDumpCommandlet );

CFileDumpCommandlet::CFileDumpCommandlet()
{
	m_commandletName = CName( TXT("dumpfile") );
	m_directLoad = false;
}

CFileDumpCommandlet::~CFileDumpCommandlet()
{
}

bool CFileDumpCommandlet::Execute( const CommandletOptions& options )
{
	// get out path
	if ( !options.GetSingleOptionValue( TXT("out"), m_outPath ) )
	{
		ERR_WCC( TXT("Missing output file path") );
		return false;
	}

	// excluded extensions
	m_excludedExtensions.Insert( TXT(".buffer") );
	if ( options.HasOption( TXT("exclude") ) )
	{
		auto opt = options.GetOptionValues( TXT("exclude") );
		for ( auto it = opt.Begin(); it != opt.End(); ++it )
		{
			m_excludedExtensions.Insert( *it );
		}
	}

	// direct load
	m_directLoad = options.HasOption( TXT("direct") );

	// get file path
	String inputPath;
	if ( options.GetSingleOptionValue( TXT("file"), inputPath ) )
	{
		ProcessFile( String::EMPTY, inputPath );
		return true;
	}
	else if ( options.GetSingleOptionValue( TXT("dir"), inputPath ) )
	{
		TDynArray< String > files;
		GFileManager->FindFiles( inputPath, TXT("*.*"), files, true );
		LOG_WCC( TXT("Found %d files"), files.Size() );

		for ( Uint32 i=0; i<files.Size(); ++i )
		{
			LOG_WCC( TXT("Status: [%d/%d] Processing '%ls'..."), 
				i, files.Size(), files[i].AsChar() );

			ProcessFile( inputPath, files[i] );
		}
		return true;
	}
	else
	{
		ERR_WCC( TXT("Expecting -file or -dir") );
		return false;
	}
}

void CFileDumpCommandlet::ProcessFile( const String& basePath, const String& filePath )
{
	// Buffers
	const auto ext = StringHelpers::GetFileExtension( filePath );
	if ( m_excludedExtensions.Exist( ext ) )
		return;

	// Write a detailed object dump
	const String relativePath = filePath.StringAfter( basePath );
	const String outputPath = m_outPath + relativePath + TXT(".xml");

	// file already exists
	if ( GFileManager->FileExist( outputPath ) )
		return;

	// Setup loader context
	DependencyLoadingContext loadingContext;
	loadingContext.m_parent = NULL; // resources from files are never parented
	loadingContext.m_validateHeader = true; // do header validation - at this stage we can safely handle missing assets
	loadingContext.m_getAllLoadedObjects = true;

	CDependencyFileData fileData;

	// direct load ?
	//if ( m_directLoad )
	{
		// reader
		IFile* reader = GFileManager->CreateFileReader( filePath, FOF_AbsolutePath | FOF_Buffered );
		if ( !reader )
			return;

		// load file content into memory buffer
		TDynArray< Uint8 > fileDataInMemory;
		const Uint32 fileSize = (Uint32) reader->GetSize();
		fileDataInMemory.Resize( fileSize );
		reader->Serialize( fileDataInMemory.Data(), fileSize );

		// read from the memory
		delete reader;
		reader = new CMemoryFileReader( fileDataInMemory.TypedData(), fileSize, 0 );
		reader->m_flags &= ~FF_MemoryBased;
		reader->m_flags |= FF_FileBased;

		// load the file data
		Uint32 fileVersion = 0;
		fileData.Load( *reader, fileVersion );
		reader->Seek(0);

		// Hacky disk file
		CDiskFile* diskFile = new CDiskFile( GDepot, TXT("fake") );

		// stats
	#ifdef RED_PROFILE_FILE_SYSTEM
		RedIOProfiler::ProfileDiskFileLoadResourceStart( filePath.AsChar() );
	#endif

		// Load data from file
		CDependencyLoader loader( *reader, diskFile );
		if ( !loader.LoadObjects( loadingContext ) )
		{
			ERR_WCC( TXT("Could not load content of file '%ls'"), filePath.AsChar() );
			delete reader;
			return;
		}

		// stats
	#ifdef RED_PROFILE_FILE_SYSTEM
		RedIOProfiler::ProfileDiskFileLoadResourceEnd( filePath.AsChar() );
		GFileManagerProfiler.Flush();
	#endif

		// close
		delete reader;
	}

	// create writer
	Red::TScopedPtr< CHTMLReportWriter > writer( CHTMLReportWriter::Create( outputPath ) );
	if ( !writer )
	{
		ERR_WCC( TXT("Could not open output file '%ls'"), outputPath.AsChar() );
	}			
	else
	{
		CHTMLNode docRoot( writer.Get(), "dump" );

		// tables
		{
			CHTMLNode doc( docRoot, "tables" );
			DumpTables( doc, fileData );
		}

		// objects
		{
			CHTMLNode doc( docRoot, "objects" );
			doc->Attrf( "count", "%d", loadingContext.m_loadedObjects.Size() );

			// collect allowed root object
			THashSet< const CObject* > allowedRoots;
			for ( Uint32 i=0; i<loadingContext.m_loadedObjects.Size(); ++i )
			{
				const CPointer obj = loadingContext.m_loadedObjects[i];
				const CObject* trueObject = obj.GetObjectPtr();
				if ( trueObject )
				{
					allowedRoots.Insert( trueObject );
				}
			}

			// map all objects to IDs
			THashMap< const void*, Int32 > visitedObjects;
			for ( Uint32 i=0; i<loadingContext.m_loadedObjects.Size(); ++i )
			{
				const CPointer obj = loadingContext.m_loadedObjects[i];
				MapObjects( obj.GetPointer(), obj.GetRuntimeClass(), visitedObjects, allowedRoots );
			}

			THashSet< const CObject* > dumpedObjects;

			// dump objects into 
			for ( Uint32 i=0; i<loadingContext.m_loadedObjects.Size(); ++i )
			{
				const CPointer obj = loadingContext.m_loadedObjects[i];

				// skip already dumped child objects
				const CObject* trueObject = obj.GetObjectPtr();
				if ( trueObject )
				{
					dumpedObjects.Insert( trueObject );

					if ( trueObject->GetParent() && dumpedObjects.Exist( trueObject->GetParent() ) )
						continue;
				}

				// dump object data
				DumpObject( doc, obj.GetPointer(), obj.GetRuntimeClass(), visitedObjects );
			}
		}
	}

	// discard loaded objects
	if ( !loadingContext.m_loadedObjects.Empty() )
	{
		const CPointer obj = loadingContext.m_loadedObjects[0];
		CObject* trueObject = obj.GetObjectPtr();

		if ( trueObject )
		{
			trueObject->Discard();
			GObjectsDiscardList->ProcessList(true);
		}
	}

}

void CFileDumpCommandlet::MapProperty( const void* propData, const IRTTIType* propType, THashMap< const void*, Int32 >& visitedObjects, const THashSet< const CObject* >& allowedRoots )
{
	if ( propType->GetType() == RT_Array || propType->GetType() == RT_NativeArray || propType->GetType() == RT_StaticArray )
	{
		const IRTTIBaseArrayType* arrayType = static_cast< const IRTTIBaseArrayType* >( propType );
		const Uint32 arrayCount = arrayType->ArrayGetArraySize( propData );
		for ( Uint32 i=0; i<arrayCount; ++i )
		{
			const void* itemData = arrayType->ArrayGetArrayElement( propData, i );
			MapProperty( itemData, arrayType->ArrayGetInnerType(), visitedObjects, allowedRoots );
		}
	}
	else if ( propType->GetType() == RT_Pointer || propType->GetType() == RT_Handle )
	{
		const IRTTIPointerTypeBase* pointerType = static_cast< const IRTTIPointerTypeBase* >( propType );

		const CPointer pointedData = pointerType->GetPointer(propData);
		if ( pointedData.GetPointer() )
		{
			MapObjects( pointedData.GetPointer(), pointedData.GetClass(), visitedObjects, allowedRoots );
		}
	}
	else if ( propType->GetType() == RT_Class )
	{
		const CClass* classType = static_cast< const CClass* >( propType );
		MapObjectData( propData, classType, visitedObjects, allowedRoots );
	}
}

void CFileDumpCommandlet::MapObjects( const void* objectData, const CClass* objectClass, THashMap< const void*, Int32 >& visitedObjects, const THashSet< const CObject* >& allowedRoots )
{
	// already visited ?
	if ( visitedObjects.KeyExist( objectData ) )
		return;

	// is it from the allowed root set ?
	if ( objectClass->IsSerializable() )
	{
		const ISerializable* serializable = static_cast< const ISerializable* >( objectData );
		if ( serializable->IsObject() )
		{
			const CObject* trueObject = static_cast< const CObject* >( objectData );

			bool isUnderAllowedRoots = false;
			for ( auto it = allowedRoots.Begin(); it != allowedRoots.End(); ++it )
			{
				CObject* rootObject = (CObject*) *it;
				if ( trueObject->IsContained( rootObject ) )
				{
					isUnderAllowedRoots = true;
					break;
				}
			}

			// not from this object tree
			if ( !isUnderAllowedRoots )
			{
				visitedObjects.Insert( objectData, -1 );
				return;
			}
		}
	}

	// allocate ID and insert
	const Uint32 objectID = visitedObjects.Size();
	visitedObjects.Insert( objectData, objectID );

	// map the data
	MapObjectData( objectData, objectClass, visitedObjects, allowedRoots );
}

void CFileDumpCommandlet::MapObjectData( const void* objectData, const CClass* objectClass, THashMap< const void*, Int32 >& visitedObjects, const THashSet< const CObject* >& allowedRoots )
{
	// get runtime class
	const CClass* runtimeClass = objectClass;
	if ( objectClass->IsSerializable() )
	{
		const ISerializable* serializable = static_cast< const ISerializable* >( objectData );
		runtimeClass = serializable->GetClass();
	}

	// child objects
	if ( runtimeClass->IsObject() )
	{
		const CObject* object = static_cast< const CObject* >( objectData );

		// child objects
		TDynArray< CObject* > children;
		object->GetChildren( children );

		for ( CObject* child : children )
		{
			MapObjects( child, child->GetClass(), visitedObjects, allowedRoots );
		}
	}

	// map properties
	const auto& properties = runtimeClass->GetCachedProperties();
	if ( !properties.Empty() )
	{
		for ( const CProperty* prop : properties )
		{
			const void* propData = prop->GetOffsetPtr( objectData );
			MapProperty( propData, prop->GetType(), visitedObjects, allowedRoots );
		}
	}
}

void CFileDumpCommandlet::DumpTables( CHTMLNode& node, const CDependencyFileData& fileData )
{
	CHTMLNode doc2( node, "tables" );

	// strings
	{
		CHTMLNode doc3( doc2, "strings" );
		doc3->Attrf( "count", "%d", fileData.m_strings.Size() );

		const AnsiChar* base = (const AnsiChar*) &fileData.m_strings[0];
		const AnsiChar* cur = base;
		const AnsiChar* end = base + fileData.m_strings.Size();
		while ( cur < end )
		{
			const Uint32 length = (const Uint32) Red::StringLength( cur );
			CHTMLNode doc4( doc3, "string" );
			doc4->Attrf( "length", "%d", length );
			doc4->Attrf( "offset", "%d", (Uint32)(cur - base) );
			doc4->Write( cur );

			cur += (length+1); // next string
		}
	}

	// names
	{
		CHTMLNode doc3( doc2, "names" );
		doc3->Attrf( "count", "%d", fileData.m_names.Size() );

		Uint32 index = 0;
		for ( const auto& it : fileData.m_names )
		{
			CHTMLNode doc4( doc3, "name" );
			doc4->Attrf( "name", "%d", index++ );
			doc4->Attrf( "string", "%d", it.m_string );
			doc4->Write( &fileData.m_strings[ it.m_string ] );
		}
	}

	// exports
	{
		CHTMLNode doc3( doc2, "exports" );
		doc3->Attrf( "count", "%d", fileData.m_exports.Size() );

		Uint32 index = 0;
		for ( const auto& it : fileData.m_exports )
		{
			CHTMLNode doc4( doc3, "export" );
			doc4->Attrf( "index", "%d", index++ );
			doc4->Attrf( "className", "%d", it.m_className );
			doc4->Attrf( "objectFlags", "0x%08X", it.m_objectFlags );
			doc4->Attrf( "parent", "%d", it.m_parent );
			doc4->Attrf( "dataSize", "%d", it.m_dataSize );
			doc4->Attrf( "dataOffset", "%d (0x%08X)", it.m_dataOffset, it.m_dataOffset );
			doc4->Attrf( "template", "%d", it.m_template );
			doc4->Attrf( "crc", "0x%08X", it.m_crc );
		}
	}

	// imports
	{
		CHTMLNode doc3( doc2, "imports" );
		doc3->Attrf( "count", "%d", fileData.m_imports.Size() );

		Uint32 index = 0;
		for ( const auto& it : fileData.m_imports )
		{
			CHTMLNode doc4( doc3, "imports" );
			doc4->Attrf( "index", "%d", index++ );
			doc4->Attrf( "path", "%d", it.m_path );
			doc4->Attrf( "className", "%d", it.m_className );
			doc4->Attrf( "flags", "0x%08X", it.m_flags );
		}
	}

	// buffers
	{
		CHTMLNode doc3( doc2, "buffers" );
		doc3->Attrf( "count", "%d", fileData.m_buffers.Size() );

		Uint32 index = 0;
		for ( const auto& it : fileData.m_buffers )
		{
			CHTMLNode doc4( doc3, "buffer" );
			doc4->Attrf( "index", "%d", index++ );
			doc4->Attrf( "name", "%d", it.m_name );
			doc4->Attrf( "flags", "0x%04X", it.m_flags );
			doc4->Attrf( "index", "%d", it.m_index );
			doc4->Attrf( "dataOffset", "%d", it.m_dataOffset );
			doc4->Attrf( "dataSizeOnDisk", "%d", it.m_dataSizeOnDisk );
			doc4->Attrf( "dataSizeInMemory", "%d", it.m_dataSizeInMemory );
			doc4->Attrf( "crc", "0x%08X", it.m_crc );
		}
	}
}

void CFileDumpCommandlet::DumpProperty( CHTMLNode& node, const void* propData, const IRTTIType* propType, const THashMap< const void*, Int32 >& visitedObjects )
{
	if ( propType->GetType() == RT_Array || propType->GetType() == RT_NativeArray || propType->GetType() == RT_StaticArray )
	{
		const IRTTIBaseArrayType* arrayType = static_cast< const IRTTIBaseArrayType* >( propType );
		const Uint32 arrayCount = arrayType->ArrayGetArraySize( propData );

		CHTMLNode doc2( node, "array" );
		doc2->Attrf( "count", "%d", arrayCount );

		for ( Uint32 i=0; i<arrayCount; ++i )
		{
			CHTMLNode doc3( doc2, "element" );
			doc3->Attrf( "index", "%d", i );

			const void* itemData = arrayType->ArrayGetArrayElement( propData, i );
			DumpProperty( doc3, itemData, arrayType->ArrayGetInnerType(), visitedObjects );
		}
	}
	else if ( propType->GetType() == RT_Pointer || propType->GetType() == RT_Handle )
	{
		const IRTTIPointerTypeBase* pointerType = static_cast< const IRTTIPointerTypeBase* >( propType );

		const CPointer pointedData = pointerType->GetPointer(propData);
		if ( pointedData.GetPointer() )
		{
			Int32 objectId = -1;
			visitedObjects.Find( pointedData.GetPointer(), objectId );

			if ( objectId != -1 )
			{
				CHTMLNode doc3( node, "reference" );
				doc3->Attr( "class", pointedData.GetRuntimeClass()->GetName().AsAnsiChar() );
				doc3->Attrf( "id", "%d", objectId );
			}
			else
			{
				const CClass* runtimeClass = pointedData.GetRuntimeClass();
				if ( runtimeClass->IsA< CResource >() )
				{
					const CResource* res = static_cast< const CResource* >( pointedData.GetPointer() );

					CHTMLNode doc3( node, "resource" );
					doc3->Attr( "class", pointedData.GetRuntimeClass()->GetName().AsAnsiChar() );
					doc3->Attrf( "path", UNICODE_TO_ANSI( res->GetDepotPath().AsChar() ) );
				}
				else
				{
					CHTMLNode doc3( node, "external" );
					doc3->Attr( "class", pointedData.GetRuntimeClass()->GetName().AsAnsiChar() );
				}
			}
		}
		else
		{
			node->Write( "NULL" );
		}
	}
	else if ( propType->GetType() == RT_Class )
	{
		const CClass* classType = static_cast< const CClass* >( propType );
		DumpObjectData( node, propData, classType, visitedObjects );
	}
	else
	{
		String value;
		propType->ToString( propData, value );
		node->Write( UNICODE_TO_ANSI( value.AsChar() ) );
	}
}

void CFileDumpCommandlet::DumpObject( CHTMLNode& node, const void* objectData, const CClass* objectClass, const THashMap< const void*, Int32 >& visitedObjects )
{
	// not defined
	Int32 objectID = -1;
	visitedObjects.Find( objectData, objectID );
	if ( objectID == -1 )
		return;

	// dump object data
	DumpObjectData( node, objectData, objectClass, visitedObjects );

	// ultra hack
	if ( objectClass->IsA< CSectorData >() )
	{
		CHTMLNode doc( node, "sectorData" );
		const CSectorData* data = static_cast<const CSectorData*>( objectData );

		// resources
		{
			CHTMLNode doc2( doc, "resources" );

			Uint32 index = 0;
			for ( const auto& it : data->m_resources )
			{
				CHTMLNode doc3( doc2, "resource" );
				doc3->Attrf( "index", "%d", index++ );
				doc3->Attrf( "box0", "%f", it.m_box[0] );
				doc3->Attrf( "box1", "%f", it.m_box[1] );
				doc3->Attrf( "box2", "%f", it.m_box[2] );
				doc3->Attrf( "box3", "%f", it.m_box[3] );
				doc3->Attrf( "box4", "%f", it.m_box[4] );
				doc3->Attrf( "box5", "%f", it.m_box[5] );
				doc3->Attrf( "patchHash", "0x%016llX", (Uint64)it.m_pathHash );
			}
		}
		
		// objects
		{
			CHTMLNode doc2( doc, "objects" );

			for ( Uint32 index=0; index<data->m_objects.Size(); ++index )
			{
				const auto& it = data->m_objects[index];

				CHTMLNode doc3( doc2, "object" );
				doc3->Attrf( "index", "%d", index );
				doc3->Attrf( "type", "%d", it.m_type );
				doc3->Attrf( "flags", "0x%08X", it.m_flags );
				doc3->Attrf( "radius", "%d", it.m_radius );
				doc3->Attrf( "dataOffset", "%d (0x%08X)", it.m_offset, it.m_offset );
				doc3->Attrf( "sectorID", "%d", it.m_sectorID );
				doc3->Attrf( "posX", "%f", it.m_pos.X );
				doc3->Attrf( "posY", "%f", it.m_pos.Y );
				doc3->Attrf( "posZ", "%f", it.m_pos.Z );

				Uint32 nextObjectOffset = ((index+1) < data->m_objects.Size())
					? data->m_objects[index+1].m_offset 
					: data->m_dataStream.Size();

				{
					CHTMLNode doc4( doc3, "dataBlock" );
					for ( Uint32 j=it.m_offset; j<nextObjectOffset; ++j )
					{
						CHTMLNode doc5( doc4, "data" );
						doc5->Attrf( "relOffset", "%d (0x%08X)", j-it.m_offset, j-it.m_offset );
						doc5->Attrf( "absOffset", "%d (0x%08X)", j, j );
						doc5->Writef( "%d", data->m_dataStream[j] );
					}
				}
			}
		}
	}
}

void CFileDumpCommandlet::DumpObjectData( CHTMLNode& node, const void* objectData, const CClass* objectClass, const THashMap< const void*, Int32 >& visitedObjects )
{
	// not defined
	Int32 objectID = -1;
	visitedObjects.Find( objectData, objectID );

	// get runtime class
	const ISerializable* serializable = nullptr;
	const CClass* runtimeClass = objectClass;
	if ( objectClass->IsSerializable() )
	{
		serializable = static_cast< const ISerializable* >( objectData );
		runtimeClass = serializable->GetClass();
	}

	// object stuff
	CHTMLNode doc( node, "object" );
	doc->Attr( "class", runtimeClass->GetName().AsAnsiChar() );

	// ID - only properly visited objects
	if ( objectID != -1 ) 
		doc->Attrf( "id", "%d", objectID );

	// flags
	if ( runtimeClass->IsObject() )
	{
		const CObject* object = static_cast<const  CObject* >( objectData );
		doc->Attrf( "flags", "0x%08X", object->GetFlags() );

		// child objects
		TDynArray< CObject* > children;
		object->GetChildren( children );
		if ( !children.Empty() )
		{
			CHTMLNode doc2( doc, "children" );
			for ( CObject* child : children )
			{
				DumpObject( doc2, child, child->GetClass(), visitedObjects );
			}
		}
	}

	// process properties
	auto properties = runtimeClass->GetCachedProperties();
	if ( !properties.Empty() )
	{
		CHTMLNode doc2( doc, "properties" );

		::Sort( properties.Begin(), properties.End(), []( const CProperty* a, const CProperty* b ) { return Red::StringCompare( a->GetName().AsChar(), b->GetName().AsChar() ) < 0; } );
		for ( const CProperty* prop : properties )
		{
			CHTMLNode doc3( doc2, "prop" );
			doc3->Attr( "name", prop->GetName().AsAnsiChar() );
			doc3->Attr( "type", prop->GetType()->GetName().AsAnsiChar() );
			doc3->Attrf( "offset", "%d", prop->GetDataOffset() );

			if ( prop->IsScripted() )
			{
				doc3->Attr( "scripted", "1" );
			}

			// write property value
			const void* propData = prop->GetOffsetPtr( objectData );
			DumpProperty( doc3, propData, prop->GetType(), visitedObjects );
		}
	}	

}

void CFileDumpCommandlet::PrintHelp() const
{
	LOG_WCC( TXT("Usage:") );
	LOG_WCC( TXT("  dumpfile -file=<path> -dir=<path> -out=<output dir or file>") );
	LOG_WCC( TXT("Parameters:") );
	LOG_WCC( TXT("  -dir=<dir>     - dump the whole directory (recursive)") );
	LOG_WCC( TXT("  -file=<file>   - depot path to the file") );
	LOG_WCC( TXT("  -out=<file>    - absolute path to the output file") );
	LOG_WCC( TXT("  -exclude=<ext> - exclude given file extensions") );
}

