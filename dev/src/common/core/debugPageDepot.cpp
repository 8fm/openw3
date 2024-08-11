/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "filePath.h"
#include "hashSet.h"
#include "depot.h"
#include "debugPageHandler.h"
#include "debugPageHTMLDoc.h"
#include "httpResponseData.h"

#include "objectRootSet.h"
#include "objectMap.h"

#include "dependencyLoader.h"
#include "dependencyFileTables.h"

#ifndef NO_DEBUG_PAGES

class CDebugPageDepot : public IDebugPageHandlerHTML
{
public:
	CDebugPageDepot()
		: IDebugPageHandlerHTML( "/depot/" )
	{}

private:
	// IDebugPageHandler interface
	virtual StringAnsi GetTitle() const override { return "Depot browser"; }
	virtual StringAnsi GetCategory() const override { return "Core"; }
	virtual Bool HasIndexPage() const override { return true; }

	//! Process command, commands are always passed via URLs, return NULL if not handled
	virtual bool OnFillPage( const class CBasicURL& fullURL, const class CBasicURL& relativeURL, class CDebugPageHTMLDocument& doc ) override
	{
		// get conformed path
		StringAnsi temp;
		const StringAnsi& safeDepotPath = CFilePath::ConformPath( relativeURL.GetPath(), temp );

		// get the dir
		CDirectory* dir = safeDepotPath.Empty() ? GDepot : GDepot->FindPath( ANSI_TO_UNICODE( safeDepotPath.AsChar() ) );
		if ( !dir )
			return false;

		// display content of directory
		StringAnsi dirPath( UNICODE_TO_ANSI( dir->GetDepotPath().AsChar() ) );
		doc << "<p>Content of '";
		doc << dirPath;
		doc << "'</p>";

		// relative path base
		doc.AddReplacementKey( "dir", relativeURL.GetPath() );

		// print directories
		if ( dir->GetDirectories().Size() > 0 )
		{
			// collect directories
			TDynArray< CDirectory* > dirs;
			for ( auto dir : dir->GetDirectories() )
			{
				dirs.PushBack( dir );
			}

			// sort by name
			::Sort( dirs.Begin(), dirs.End(), []( CDirectory* a, CDirectory* b ) { return a->GetName() < b->GetName(); } );

			// print
			doc << "<p>Directories:<ul>";
			for ( auto dir : dirs )
			{
				const StringAnsi name( UNICODE_TO_ANSI( dir->GetName().AsChar() ) );
				doc.Open("li").Doc().Link("/depot/#<dir>#%hs/", name.AsChar() ).Write( name.AsChar() );
			}
			doc << "</ul></p>";
		}

		// print files
		if ( dir->GetFiles().Size() > 0 )
		{
			// collect files
			TDynArray< CDiskFile* > files;
			for ( auto file : dir->GetFiles() )
			{
				files.PushBack( file );
			}

			// sort by name
			::Sort( files.Begin(), files.End(), []( CDiskFile* a, CDiskFile* b ) { return a->GetFileName() < b->GetFileName(); } );

			// print
			doc << "<p>Files:<ul>";
			for ( auto file : files )
			{
				const StringAnsi name( UNICODE_TO_ANSI( file->GetFileName().AsChar() ) );
				doc.Open("li").Doc().Link("/file/#<dir>#%hs", name.AsChar() ).Write( name.AsChar() );
			}
			doc << "</ul></p>";
		}

		return true;
	}
};

namespace Helper
{
	class CSoftHandleDepFinder
	{
	public:
		struct PropInfo
		{
			const void*			m_data;
			const CClass*		m_class;
			const CProperty*	m_property;

			PropInfo( const void* data, const CClass* theClass, const CProperty* prop )
				: m_data( data )
				, m_class( theClass )
				, m_property( prop )
			{}
		};

		struct Dependency
		{
			const CObject*				m_baseObject;
			TDynArray< PropInfo >		m_properties;
		};

		// Find the uses of given disk file as soft handle
		void Scan( const CDiskFile* file, TDynArray< Dependency >& outDeps, Uint32& outNumVisitedObjects )
		{
			m_file = file;
			m_deps = &outDeps;
			m_visited.Clear();

			// get conformed path
			String temp;
			m_path = CFilePath::ConformPath( file->GetDepotPath(), temp );

			// scan root set objects ONLY
			GObjectsMap->VisitAllObjectsNoFilter(
				[this]( CObject* object, const Uint32 ) { VisitObject( object ); return true; }
			);			

			outNumVisitedObjects = m_visited.Size();
		}

	private:
		void VisitObject( const CObject* object )
		{
			TDynArray< PropInfo > propStack;
			VisitPtr( object, object, object->GetClass(), propStack );
		}

		void VisitPtr( const CObject* owner, const void* object, const CClass* theClass, TDynArray< PropInfo >& propStack )
		{
			// already visited
			if ( !object || !m_visited.Insert( object ) )
				return;

			VisitPtrRaw( owner, object, theClass, propStack );
		}

		void VisitPtrRaw( const CObject* owner, const void* object, const CClass* theClass, TDynArray< PropInfo >& propStack )
		{

			// process class properties
			const auto& props = theClass->GetCachedProperties();
			for ( const CProperty* prop : props )
			{
				propStack.PushBack( PropInfo( object, theClass, prop ) );
				const void* propData = prop->GetOffsetPtr( object );
				VisitTypeData( owner, propData, prop->GetType(), propStack );
				propStack.PopBack();
			}
		}

		static Bool IsWorthVisiting( const IRTTIType* type )
		{
			switch ( type->GetType() )
			{
				case RT_Enum:
				case RT_BitField:
				case RT_Simple:
				case RT_Fundamental:
					return false;
			}

			return true;
		}

		void VisitTypeData( const CObject* owner, const void* propData, const IRTTIType* propType, TDynArray< PropInfo >& propStack )
		{
			const auto metaType = propType->GetType();
			if ( metaType == RT_Array || metaType == RT_NativeArray || metaType == RT_StaticArray )
			{
				const IRTTIBaseArrayType* arrayType = static_cast< const IRTTIBaseArrayType* >( propType );
				const IRTTIType* innerType = arrayType->ArrayGetInnerType();
				
				if ( IsWorthVisiting( innerType ) )
				{
					const Uint32 count = arrayType->ArrayGetArraySize( propData );
					for ( Uint32 i=0; i<count; ++i )
					{
						const void* itemData = arrayType->ArrayGetArrayElement( propData, i );
						VisitTypeData( owner, itemData, arrayType->ArrayGetInnerType(), propStack );
					}
				}
			}
			else if ( propType->GetType() == RT_SoftHandle )
			{
				const BaseSoftHandle& softHandle = *(const BaseSoftHandle*) propData;

				String temp;
				const String& conformedPath = CFilePath::ConformPath(softHandle.GetPath(), temp);

				if ( conformedPath == m_path )
				{
					Dependency* depInfo = new ( *m_deps ) Dependency();
					depInfo->m_baseObject = owner;
					depInfo->m_properties = propStack;
				}
			}
			else if ( propType->GetType() == RT_Handle || propType->GetType() == RT_Pointer )
			{
				const CRTTIPointerType* pointerType = static_cast< const CRTTIPointerType* >( propType );
				const CPointer pointedObject = pointerType->GetPointer( propData );
				if ( !pointedObject.IsNull() )
				{
					if ( pointedObject.IsObject() )
					{
						const CObject* object = pointedObject.GetObjectPtr();
						VisitObject( object );
					}
					else
					{
						const CClass* runtimeClass = pointedObject.GetRuntimeClass();
						const void* runtimeData = pointedObject.GetPointer();
						VisitPtr( owner, runtimeData, runtimeClass, propStack );
					}
				}
			}
			else if ( propType->GetType() == RT_Class )
			{
				const CClass* pointedClass = static_cast< const CClass* >( propType );
				VisitPtrRaw( owner, propData, pointedClass, propStack );
			}
		}

		THashSet< const void* >		m_visited;
		TDynArray< Dependency >*	m_deps;
		String						m_path;
		const CDiskFile*			m_file;
	};
}

namespace Helper
{
	// information about string
	class StringInfo : public CDebugPageHTMLTable::IRow
	{
	public:
		Uint32			m_id;
		Uint32			m_offset;
		StringAnsi		m_text;
		Uint32			m_lenght;

	public:
		StringInfo( const Uint32 id, const Uint32 offset, const StringAnsi& str )
			: m_id( id )
			, m_offset( offset )
			, m_text( str )
		{
			m_lenght = m_text.GetLength();
		}

		virtual Bool OnCompare( const IRow* other, const Int32 columnID ) override
		{
			const auto* b = ((const StringInfo*)other);

			switch ( columnID )
			{
			case 2: return m_offset < b->m_offset;
			case 3: return m_lenght < b->m_lenght;
			case 4: return m_text < b->m_text;
			}
			return m_id < b->m_id;
		}

		virtual void OnGenerateHTML( CDebugPageHTMLDocument& doc, const Int32 columnID ) override
		{
			switch ( columnID )
			{
			case 1: doc.Writef("%d", m_id); break;
			case 2: doc.Writef("%d", m_offset); break;
			case 3: doc.Writef("%d", m_lenght); break;
			case 4: doc.Write(m_text.AsChar()); break;
			}
		}
	};

	// information about name
	class NameInfo : public CDebugPageHTMLTable::IRow
	{
	public:
		Uint32			m_id;
		StringAnsi		m_text;
		Uint32			m_lenght;

	public:
		NameInfo( const Uint32 id, const StringAnsi& str )
			: m_id( id )
			, m_text( str )
		{
			m_lenght = m_text.GetLength();
		}

		virtual Bool OnCompare( const IRow* other, const Int32 columnID ) override
		{
			const auto* b = ((const NameInfo*)other);

			switch ( columnID )
			{
			case 2: return m_lenght < b->m_lenght;
			case 3: return m_text < b->m_text;
			}
			return m_id < b->m_id;
		}

		virtual void OnGenerateHTML( CDebugPageHTMLDocument& doc, const Int32 columnID ) override
		{
			switch ( columnID )
			{
			case 1: doc.Writef("%d", m_id); break;
			case 2: doc.Writef("%d", m_lenght); break;
			case 3: doc.Write(m_text.AsChar()); break;
			}
		}
	};

	// information about property
	class PropertyInfo : public CDebugPageHTMLTable::IRow
	{
	public:
		Uint32			m_id;
		StringAnsi		m_className;
		StringAnsi		m_typeName;
		StringAnsi		m_propertyName;
		Bool			m_isMapped;
		Uint64			m_hash;

	public:
		PropertyInfo( const Uint32 id, const StringAnsi& className, const StringAnsi& typeName, const StringAnsi& propertyName, const Bool isMapped, const Uint64 hash )
			: m_id( id )
			, m_className( className )
			, m_typeName( typeName )
			, m_propertyName( propertyName )
			, m_isMapped( isMapped )
			, m_hash( hash )
		{
		}

		virtual Bool OnCompare( const IRow* other, const Int32 columnID ) override
		{
			const auto* b = ((const PropertyInfo*)other);

			switch ( columnID )
			{
			case 2: return m_className < b->m_className;
			case 3: return m_typeName < b->m_typeName;
			case 4: return m_propertyName < b->m_propertyName;
			case 5: return (Int32)m_isMapped < (Int32)b->m_isMapped;
			case 6: return m_hash < b->m_hash;
			}
			return m_id < b->m_id;
		}

		virtual void OnGenerateHTML( CDebugPageHTMLDocument& doc, const Int32 columnID ) override
		{
			switch ( columnID )
			{
			case 1: doc.Writef("%d", m_id); break;
			case 2: doc.Write(m_className.AsChar()); break;
			case 3: doc.Write(m_typeName.AsChar()); break;
			case 4: doc.Write(m_propertyName.AsChar()); break;
			case 5: doc.Write(m_isMapped ? "" : "<span class=\"error\">MISSING</span>"); break;
			case 6: doc.Writef("0x%016llX", m_hash); break;
			}
		}
	};

	// information about export
	class ExportInfo : public CDebugPageHTMLTable::IRow
	{
	public:
		Uint32			m_id;
		Uint32			m_parent;
		StringAnsi		m_class;
		Uint32			m_dataOffset;
		Uint32			m_dataSize;

	public:
		ExportInfo( const Uint32 id, const Uint32 parentId, const StringAnsi& className, const Uint32 dataOffset, const Uint32 dataSize )
			: m_id( id )
			, m_parent( parentId )
			, m_class( className )
			, m_dataOffset( dataOffset )
			, m_dataSize( dataSize )
		{
		}

		virtual Bool OnCompare( const IRow* other, const Int32 columnID ) override
		{
			const auto* b = ((const ExportInfo*)other);

			switch ( columnID )
			{
			case 2: return m_parent < b->m_parent;
			case 3: return m_class < b->m_class;
			case 4: return m_dataOffset < b->m_dataOffset;
			case 5: return m_dataSize < b->m_dataSize;
			}
			return m_id < b->m_id;
		}

		virtual void OnGenerateHTML( CDebugPageHTMLDocument& doc, const Int32 columnID ) override
		{
			switch ( columnID )
			{
			case 1: doc.Writef("%d", m_id); break;
			case 2: if ( m_parent ) { doc.Writef("%d", m_parent); } break;
			case 3: doc.Write(m_class.AsChar()); break;
			case 4: doc.Writef("%d", m_dataOffset); break;
			case 5: doc.Writef("%d", m_dataSize); break;
			}
		}
	};

	// information about import
	class ImportInfo : public CDebugPageHTMLTable::IRow
	{
	public:
		Uint32			m_id;
		StringAnsi		m_class;
		StringAnsi		m_path;
		Bool			m_isSoft;

	public:
		ImportInfo( const Uint32 id, const StringAnsi& className, const StringAnsi& path, const Bool isSoft )
			: m_id( id )
			, m_class( className )
			, m_path( path )
			, m_isSoft( isSoft )
		{
		}

		virtual Bool OnCompare( const IRow* other, const Int32 columnID ) override
		{
			const auto* b = ((const ImportInfo*)other);

			switch ( columnID )
			{
			case 2: return m_class < b->m_class;
			case 3: return (Int32)m_isSoft < (Int32)b->m_isSoft;
			case 4: return m_path < b->m_path;
			}
			return m_id < b->m_id;
		}

		virtual void OnGenerateHTML( CDebugPageHTMLDocument& doc, const Int32 columnID ) override
		{
			switch ( columnID )
			{
			case 1: doc.Writef("%d", m_id); break;
			case 2: doc.Write(m_class.AsChar()); break;
			case 3: doc.Write(m_isSoft ? "SOFT" : ""); break;
			case 4: doc.Link("/file/%hs", m_path.AsChar()).Write(m_path.AsChar()); break;
			}
		}
	};

	// Dump tables
	void DumpFileTables( const CDependencyLoader& loader, class CDebugPageHTMLDocument& doc, const CBasicURL& url )
	{
		auto tables = loader.GetFileTables();
		if ( !tables )
		{
			doc << "<span class=\"error\">Unable to load tables from file</span>";
			return;
		}

		// Header information
		{
			CDebugPageHTMLInfoBlock info( doc, "Chunks" );
			info.Info( "Strings: " ).Writef( "%d (%1.2fKB)", tables->m_strings.DataSize(), tables->m_strings.DataSize() / 1024.0f );
			info.Info( "Names: " ).Writef( "%d", tables->m_names.Size() );
			info.Info( "Imports: " ).Writef( "%d", tables->m_imports.Size() );
			info.Info( "Exports: " ).Writef( "%d", tables->m_exports.Size() );
			info.Info( "Properties: " ).Writef( "%d", tables->m_properties.Size() );
			info.Info( "Buffers: " ).Writef( "%d", tables->m_buffers.Size() );
		}

		// Strings
		if ( tables->m_strings.Size() > 1 )
		{
			CDebugPageHTMLInfoBlock info( doc, "Strings" );
			CDebugPageHTMLTable table( doc, "str" );

			// colums
			table.AddColumn( "ID", 40, true );
			table.AddColumn( "Ofs", 50, true );
			table.AddColumn( "Length", 60, true );
			table.AddColumn( "Data", 750, true );

			// extract strings from the buffer (messy)
			Uint32 stringId = 1;
			const AnsiChar* base = &tables->m_strings[0];
			const AnsiChar* end = base + tables->m_strings.DataSize();
			const AnsiChar* pos = base + 1;
			while ( (pos < end) && (*pos != 0) )
			{
				// extract string
				const AnsiChar* start = pos;
				while ( *pos++ ) {};

				// add string to table
				table.AddRow( new StringInfo( stringId, (Uint32)(start-base), start) );
			}

			table.Render( 900, "generic", url );
		}

		// Names
		if ( tables->m_names.Size() > 1 )
		{
			CDebugPageHTMLInfoBlock info( doc, "Names" );
			CDebugPageHTMLTable table( doc, "names" );

			// colums
			table.AddColumn( "ID", 40, true );
			table.AddColumn( "Length", 60, true );
			table.AddColumn( "Data", 400, true );

			for ( Uint32 i=1; i<tables->m_names.Size(); ++i )
			{
				const AnsiChar* str = &tables->m_strings[ tables->m_names[i].m_string ];
				table.AddRow( new NameInfo( i, str ) );
			}

			table.Render( 500, "generic", url );
		}

		// Properties
		if ( tables->m_properties.Size() > 1 )
		{
			CDebugPageHTMLInfoBlock info( doc, "Properties" );
			CDebugPageHTMLTable table( doc, "props" );

			// colums
			table.AddColumn( "ID", 40, true );
			table.AddColumn( "Class", 150, true );
			table.AddColumn( "Type", 150, true );
			table.AddColumn( "Name", 150, true );
			table.AddColumn( "Valid", 50, true );
			table.AddColumn( "Hash", 120, true );

			for ( Uint32 i=1; i<tables->m_properties.Size(); ++i )
			{
				const auto& p = tables->m_properties[i];

				// extract mapping names
				const AnsiChar* className = &tables->m_strings[ tables->m_names[ p.m_className ].m_string ];
				const AnsiChar* typeName = &tables->m_strings[ tables->m_names[ p.m_typeName ].m_string ];
				const AnsiChar* propName = &tables->m_strings[ tables->m_names[ p.m_propertyName].m_string ];

				// find
				Bool isMapped = false;
				const CClass* propClass = SRTTI::GetInstance().FindClass( CName( ANSI_TO_UNICODE( className ) ) );
				if ( propClass )
				{
					const CProperty* prop = propClass->FindProperty( CName( ANSI_TO_UNICODE( propName ) ) );
					if ( prop )
					{
						isMapped = true;
					}
				}

				table.AddRow( new PropertyInfo( i, className, typeName, propName, isMapped, p.m_hash ) );
			}

			table.Render( 700, "generic", url );
		}

		// Imports
		if ( tables->m_imports.Size() > 0 )
		{
			CDebugPageHTMLInfoBlock info( doc, "Imports" );
			CDebugPageHTMLTable table( doc, "imports" );

			// colums
			table.AddColumn( "ID", 40, true );
			table.AddColumn( "Class", 150, true );
			table.AddColumn( "Type", 50, true );
			table.AddColumn( "Path", 700, true );

			for ( Uint32 i=0; i<tables->m_imports.Size(); ++i )
			{
				const auto& p = tables->m_imports[i];

				const AnsiChar* className = &tables->m_strings[ tables->m_names[ p.m_className ].m_string ];
				const AnsiChar* path = &tables->m_strings[ p.m_path ];

				const Bool isSoft = (p.m_flags & CDependencyFileData::eImportFlags_Soft) != 0;

				table.AddRow( new ImportInfo( i, className, path, isSoft ) );
			}

			table.Render( 950, "generic", url );
		}

		// Exports
		if ( tables->m_exports.Size() > 0 )
		{
			CDebugPageHTMLInfoBlock info( doc, "Exports" );
			CDebugPageHTMLTable table( doc, "exports" );

			// colums
			table.AddColumn( "ID", 40, true );
			table.AddColumn( "Parent", 60, true );
			table.AddColumn( "Class", 150, true );
			table.AddColumn( "DataOffset", 100, true );
			table.AddColumn( "DataSize", 100, true );

			for ( Uint32 i=0; i<tables->m_exports.Size(); ++i )
			{
				const auto& p = tables->m_exports[i];

				const AnsiChar* className = &tables->m_strings[ tables->m_names[ p.m_className ].m_string ];

				table.AddRow( new ExportInfo( i, p.m_parent, className, p.m_dataOffset, p.m_dataSize ) );
			}

			table.Render( 450, "generic", url );
		}
	}
}

class CDebugPageFile : public IDebugPageHandlerHTML
{
public:
	CDebugPageFile()
		: IDebugPageHandlerHTML( "/file/" )
	{}

private:
	// IDebugPageHandler interface
	virtual StringAnsi GetTitle() const override { return "File information"; }
	virtual StringAnsi GetCategory() const override { return "Core"; }
	virtual Bool HasIndexPage() const override { return false; }

	//! Process command, commands are always passed via URLs, return NULL if not handled
	virtual bool OnFillPage( const class CBasicURL& fullURL, const class CBasicURL& relativeURL, class CDebugPageHTMLDocument& doc ) override
	{
		// get conformed path
		StringAnsi temp;
		const StringAnsi& safeDepotPath = CFilePath::ConformPath( relativeURL.GetPath(), temp );

		// get the file
		CDiskFile* file = GDepot->FindFileUseLinks( ANSI_TO_UNICODE( safeDepotPath.AsChar() ), 0 );
		if ( !file )
		{
			// display a message instead of 404
			doc << "<span class=\"error\">File not found in depot</span>";
			return true;
		}

		// get query type
		StringAnsi mode;
		relativeURL.GetKey( "action", mode );

		// break on load helpers
		if ( mode == "boff" )
		{
			file->SetBreakOnLoad(false);
			mode = "";
		}
		else if ( mode == "bon" )
		{
			file->SetBreakOnLoad(true);
			mode = "";
		}

		// loading
		if ( mode == "load" )
		{
			if ( !file->Load() )
			{
				doc << "<span class=\"error\">Failed to force load the file</span>";
				return true;
			}
		}

		// action type
		if ( mode == "tables" )
		{
			// title
			doc.Open("p").Writef( "Serialization tables from file '%hs'", safeDepotPath.AsChar() );

			// open stuff
			Red::TScopedPtr< IFile > reader( file->CreateReader() );
			if ( !reader )
			{
				doc << "<span class=\"error\">Unable to load file content</span>";
			}
			else
			{
				CDependencyLoader loader( *reader.Get(), file );
				loader.LoadTables();
				Helper::DumpFileTables( loader, doc, fullURL );
			}
		}
		else if ( mode == "softrefs" )
		{
			// title
			doc.Open("p").Writef( "Soft handles referencing '%hs'", safeDepotPath.AsChar() );

			// find the dependencies
			Uint32 numVisitedObjects = 0;
			TDynArray< Helper::CSoftHandleDepFinder::Dependency > deps;
			{
				Helper::CSoftHandleDepFinder scaner;
				scaner.Scan( file, deps, numVisitedObjects );
			}

			// no references
			if ( deps.Empty() )
			{
				doc.Open("p").Writef( "Visited %d objects. No reachable soft handles referencing this file were found", numVisitedObjects );
			}
			else
			{
				doc.Open("p").Writef( "Visited %d objects. Found %d reachable soft handles referencing this file", numVisitedObjects, deps.Size() );

				for ( Uint32 index=0; index<deps.Size(); ++index )
				{
					CDebugPageHTMLInfoBlock info( doc, "Dependency %d", index );

					const auto& dep = deps[index];
					info.Info( "Parent object: " ).LinkObject(dep.m_baseObject);

					// parent resource
					CObject* rootObject = dep.m_baseObject->GetRoot();
					if ( rootObject && rootObject != dep.m_baseObject && rootObject->IsA< CResource >() )
					{
						info.Info( "Parent resource: " ).LinkObject( rootObject );
					}

					// props
					info.Info( "Property chain:" ).Write("<ul>");
					for ( const auto& prop : dep.m_properties )
					{
						doc.Open("li").Writef( "<b>%hs</b> in %hs, type %hs", 
							prop.m_property->GetName().AsAnsiChar(),
							prop.m_class->GetName().AsAnsiChar(),
							prop.m_property->GetType()->GetName().AsAnsiChar() );
					}
					doc.Write("</ul>");
				}
			}
		}

		// default page
		else
		{
			// title
			doc.Open("p").Writef( "Information about file '%hs'", safeDepotPath.AsChar() );

			// basic file information
			{
				CDebugPageHTMLInfoBlock info( doc, "File information" );

				info.Info( "File type: " ).Write( file->IsLooseFile() ? "Loose" : "Bundle" );
				info.Info( "State: " ).Write( file->IsLoaded() ? "Loaded" : (file->IsFailed() ? "Failed" : "Unloaded") );
				info.Info( "Overridden: " ).Write( file->IsOverriden() ? "Yes" : "No" );

				// bundled file
				if ( !file->IsLooseFile() )
				{
					CBundleDiskFile* bundleFile = static_cast< CBundleDiskFile* >( file );
					info.Info( "Bundle file ID: " ).Link( "/bundlefile/?id=%d", bundleFile->GetFileID() ).Writef( "%d", bundleFile->GetFileID() );
				}

				// dependency cache info
				if ( file->GetDepCacheIndex() )
				{
					info.Info( "Dep file ID: " ).Writef( "%d", file->GetDepCacheIndex() );
				}
				else
				{
					info.Info( "Dep file ID: " ).Write( "NONE" );
				}
			}

			// dependencies from dep cache
			if ( file->GetDepCacheIndex() )
			{
				CDebugPageHTMLInfoBlock info( doc, "Dependency cache" );

				CDependencyCollector* deps = GDepot->GetDependencyCache().AllocateCollector();
				GDepot->GetDependencyCache().CollectDependencies( *deps, file->GetDepCacheIndex() );

				CDebugPageHTMLTable table( doc, "deps" );
				table.AddColumn( "Index", 80, true );
				table.AddColumn( "DepID", 80, true );
				table.AddColumn( "Path", 500, true );
				for ( Uint32 i=0; i<deps->Size(); ++i )
				{
					const Uint32 depFileIndex = deps->GetFileIndex( i );
					CDiskFile* diskFile = GDepot->GetMappedDiskFile( depFileIndex );

					table.AddRow( 
						table.CreateRowData( (Int64)i ),
						table.CreateRowData( (Int64)depFileIndex ), 
						diskFile ? table.CreateRowData( diskFile ) : nullptr );
				}
				table.Render( 800, "table", relativeURL );
			}

			// resource information
			const CClass* rootClass = file->GetResourceClass();
			if ( rootClass && rootClass->IsA< CResource >() )
			{
				CDebugPageHTMLInfoBlock info( doc, "Resource information" );

				info.Info( "Resource class: " ).Write( rootClass ? rootClass->GetName().AsAnsiChar() : "Unknown" );
				if ( file->GetResource() )
					info.Info( "Resource pointer: " ).Link( "/object/?id=%d", file->GetResource()->GetObjectIndex() ).Writef( "0x%016llX", (Uint64) file->GetResource() );
			}

			// actions
			{
				CDebugPageHTMLInfoBlock info( doc, "Actions" );
				info.Info("").Link( "/file/%hs?action=tables", safeDepotPath.AsChar() ).Write("Dump tables");

				info.Info("").Link( "/file/%hs?action=softrefs", safeDepotPath.AsChar() ).Write("Find soft references");
				if ( file->IsLoaded() )
				{
					CResource* loadedResource = file->GetResource();
					info.Info("").Link( "/object/?id=%d&action=references", loadedResource->GetObjectIndex() ).Write("Find hard references");
				}
				else
				{
					info.Info("").Link( "/file/%hs?action=load", safeDepotPath.AsChar() ).Write("Force load (unsafe)");
				}

				info.Info("").Link( "/download/%hs", safeDepotPath.AsChar() ).Write("Download");
			}

			// debug
			{
				CDebugPageHTMLInfoBlock info( doc, "Debug" );

				if ( file->IsBreakOnLoadSet() )
				{
					info.Info("Break on load is enabled ").Link( "/file/%hs?action=boff", safeDepotPath.AsChar() ).Write("(Disable)");
				}
				else
				{
					info.Info("Break on load is disabled ").Link( "/file/%hs?action=bon", safeDepotPath.AsChar() ).Write("(Enable)");
				}
			}
		}

		return true;
	}
}; 

class CDebugPageFileDownload : public IDebugPageHandler
{
public:
	CDebugPageFileDownload()
		: IDebugPageHandler( "/download/" )
	{}

private:
	// IDebugPageHandler interface
	virtual StringAnsi GetTitle() const override { return "Download file"; }
	virtual StringAnsi GetCategory() const override { return "Core"; }
	virtual Bool HasIndexPage() const override { return false; }

	// Direct request processing
	virtual class CHTTPResponseData* OnPage( const class CBasicURL& fullURL, const class CBasicURL& relativeUrl ) override
	{
		// get conformed path
		StringAnsi temp;
		const StringAnsi& safeDepotPath = CFilePath::ConformPath( relativeUrl.GetPath(), temp );

		// log all requests (for safety)
		LOG_CORE( TXT("HTTP Download request: '%hs'"), safeDepotPath.AsChar() );

		// get the file
		CDiskFile* file = GDepot->FindFileUseLinks( ANSI_TO_UNICODE( safeDepotPath.AsChar() ), 0 );
		if ( !file )
		{
			return new CHTTPResponseData( CHTTPResponseData::eResultCode_NotFound );
		}

		// create file reader
		Red::TScopedPtr< IFile > reader( file->CreateReader() );
		if ( !reader )
		{
			return new CHTTPResponseData( CHTTPResponseData::eResultCode_ServerError );
		}

		// file to big
		const Uint32 maxSize = 8 * 1024 * 1024;
		const Uint32 size = (Uint32) reader->GetSize();
		if ( size > maxSize )
		{
			CDebugPageHTMLResponse response( fullURL );
			{
				CDebugPageHTMLDocument doc( response, GetTitle() );
				doc << "<span class=\"error\">File is to big to be downloaded directly</span>";
			}
			return response.GetData();
		}

		// load file data
		DataBlobPtr data( new CDataBlob( size ) );
		reader->Serialize( data->GetData(), data->GetDataSize() );

		// send it as octet stream
		return new CHTTPResponseData( "application/octet-stream", data );
	}
};

void InitDepotDebugPages()
{
	new CDebugPageDepot(); // autoregister
	new CDebugPageFile(); // autoregister
	new CDebugPageFileDownload(); // autoregister
}

#endif