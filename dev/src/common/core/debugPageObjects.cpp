/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "debugPageHandler.h"
#include "debugPageHTMLDoc.h"

#ifndef NO_DEBUG_PAGES

#include "hashset.h"
#include "resource.h"
#include "scriptableState.h"
#include "diskFile.h"
#include "objectMap.h"
#include "objectRootSet.h"

/// List of all objects
class CDebugPageObjects : public IDebugPageHandlerHTML
{
public:
	CDebugPageObjects()
		: IDebugPageHandlerHTML( "/objects/" )
	{}

private:
	// IDebugPageHandler interface
	virtual StringAnsi GetTitle() const override { return "Objects list"; }
	virtual StringAnsi GetCategory() const override { return "Core"; }
	virtual Bool HasIndexPage() const override { return true; }

	// add object infos
	class ObjectInfo : public CDebugPageHTMLTable::IRow
	{
	public:
		CObject*		m_object;

		enum EColumn
		{
			eColumn_ID=1,
			eColumn_Parent=2,
			eColumn_Class=3,
			eColumn_Flags=4,
			eColumn_Ptr=5,
		};

	public:
		ObjectInfo( CObject* object )
			: m_object( object )
		{}

		virtual Bool OnCompare( const IRow* other, const Int32 columnID )
		{
			CObject* a = m_object;
			CObject* b = ((ObjectInfo*)other)->m_object;

			if ( columnID == eColumn_Parent )
			{
				const Int32 pa = a->GetParent() ? a->GetParent()->GetObjectIndex() : -1;
				const Int32 pb = b->GetParent() ? b->GetParent()->GetObjectIndex() : -1;
				return pa < pb;
			}
			else if ( columnID == eColumn_Class )
			{
				const CName ca = a->GetClass()->GetName();
				const CName cb = b->GetClass()->GetName();
				return Red::StringCompare( ca.AsAnsiChar(), cb.AsAnsiChar() ) < 0;
			}
			else if ( columnID == eColumn_Flags )
			{
				return a->GetFlags() < b->GetFlags();
			}
			else if ( columnID == eColumn_Ptr )
			{
				return (Uint64)a < (Uint64)b;
			}

			// default sorting by object ID
			return a->GetObjectIndex() < b->GetObjectIndex();
		}

		virtual void OnGenerateHTML( CDebugPageHTMLDocument& doc, const Int32 columnID )
		{
			if ( columnID == eColumn_ID )
			{
				doc.Writef( "%d", m_object->GetObjectIndex() );
			}
			else if ( columnID == eColumn_Parent )
			{
				if ( m_object->GetParent() )
				{
					doc.Writef( "%d", m_object->GetParent()->GetObjectIndex() );
				}
			}
			else if ( columnID == eColumn_Class )
			{
				doc.Write( m_object->GetClass()->GetName().AsAnsiChar() );
			}
			else if ( columnID == eColumn_Flags )
			{
				if ( m_object->HasFlag( OF_DefaultObject ) )
					doc.Write( "D" );
				if ( m_object->HasFlag( OF_Root ) )
					doc.Write( "R" );
				if ( m_object->HasFlag( OF_Inlined ) )
					doc.Write( "I" );
				if ( m_object->HasFlag( OF_HasHandle ) )
					doc.Write( "H" );
				if ( m_object->HasFlag( OF_Scripted ) )
					doc.Write( "S" );
				if ( m_object->HasFlag( OF_ScriptCreated ) )
					doc.Write( "C" );
			}
			else if ( columnID == eColumn_Ptr )
			{
				doc.Link( "/object/?id=%d", m_object->GetObjectIndex() ).Writef( "0x%016llX", (Uint64)m_object );
			}
		}
	};

	//! Process command, commands are always passed via URLs, return NULL if not handled
	virtual bool OnFillPage( const class CBasicURL& fullURL, const class CBasicURL& relativeURL, class CDebugPageHTMLDocument& doc ) override
	{
		// object list mode
		Uint32 includeFlags = ~0;
		Uint32 excludeFlags = 0;
		StringAnsi mode;
		relativeURL.GetKey( "mode", mode );
		if ( mode == "all" )
		{
			doc.Open("p").Writef( "List of all CObjects in the engine" );
			includeFlags = ~0;
		}
		else if ( mode == "scripted" )
		{
			doc.Open("p").Writef( "List of all scripted objects" );
			includeFlags = OF_Scripted;
		}
		else if ( mode == "default" )
		{
			doc.Open("p").Writef( "List of all DefaultObjects" );
			includeFlags = OF_DefaultObject;
		}
		else
		{
			doc.Open("p").Writef( "List of all root objects" );
			includeFlags = OF_Root;
		}

		// count default/root and scripted objects
		Uint32 numScriptedObjects = 0;
		Uint32 numRootObjects = 0;
		Uint32 numDefaultObjects = 0;
		{
			TDynArray< CObject* > allObjects;
			GObjectsMap->GetAllObjects( allObjects, ~0, 0 );

			for ( CObject* object : allObjects )
			{
				if ( object->HasFlag( OF_DefaultObject ) )
				{
					numDefaultObjects += 1;
				}
				else if ( object->HasFlag( OF_Scripted ) )
				{
					numScriptedObjects += 1;
				}
				else if ( object->HasFlag( OF_Root ) )
				{
					numRootObjects += 1;
				}
			}
		}

		// basic file information
		{
			CDebugPageHTMLInfoBlock info( doc, "General information" );

			info.Info( "All objects: " ).Writef( "%d  ", GObjectsMap->GetNumLiveObjects() ).Link( "/objects/" ).Write( "(Filter) - SLOW!" );;
			info.Info( "Root set size: " ).Writef( "%d  ", numRootObjects ).Link( "/objects/?mode=root" ).Write( "(Filter)" );
			info.Info( "Default objects: " ).Writef( "%d  ", numDefaultObjects ).Link( "/objects/?mode=default" ).Write( "(Filter)" );
			info.Info( "Scripted objects: " ).Writef( "%d  ", numScriptedObjects ).Link( "/objects/?mode=scripted" ).Write( "(Filter)" );
			info.Info( "Object map size: " ).Writef( "%d", GObjectsMap->GetMaxObjectIndex() );
		}

		// object table
		{
			CDebugPageHTMLInfoBlock info( doc, "Object list" );
			CDebugPageHTMLTable table( doc, "obj" );

			// table definition
			table.AddColumn( "ID", 50, true );
			table.AddColumn( "Parent", 50, true );
			table.AddColumn( "Class", 300, true );
			table.AddColumn( "Flags", 100, true );
			table.AddColumn( "Ptr", 200, true );

			// create object list
			TDynArray< CObject* > allObjects;
			GObjectsMap->GetAllObjects( allObjects, includeFlags, excludeFlags );

			// push objects to table
			for ( CObject* obj : allObjects )
			{
				if ( obj )
				{
					if ( !includeFlags || obj->HasFlag( includeFlags ) )
					{
						table.AddRow( new ObjectInfo( obj ) );
					}
				}
			}

			// render the table
			table.Render( 1000, "generic", fullURL );
		}

		return true;
	}
};

namespace Helper
{
	class CDerpFinder
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
		void Scan( const CObject* object, TDynArray< Dependency >& outDeps, Uint32& outNumVisitedObjects )
		{
			m_object = object;
			m_deps = &outDeps;
			m_visited.Clear();

			// scan root set objects ONLY
			GObjectsMap->VisitAllObjectsNoFilter(
				[this]( CObject* object, const Uint32 )
				{
					if ( object != m_object )
					{
						VisitObject( object );
					}
					return true;
				}
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
				// don't bother
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

						// hit ?
						if ( object == m_object )
						{
							Dependency* depInfo = new ( *m_deps ) Dependency();
							depInfo->m_baseObject = owner;
							depInfo->m_properties = propStack;
						}
						else
						{
							VisitObject( object );
						}
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
		const CObject*				m_object;
	};
}

/// Object information
class CDebugPageObject : public IDebugPageHandlerHTML
{
public:
	CDebugPageObject()
		: IDebugPageHandlerHTML( "/object/" )
	{}

private:
	// IDebugPageHandler interface
	virtual StringAnsi GetTitle() const override { return "Objects information"; }
	virtual StringAnsi GetCategory() const override { return "Core"; }
	virtual Bool HasIndexPage() const override { return false; }

	// add object infos
	class PropInfo : public CDebugPageHTMLTable::IRow
	{
	public:
		const void*			m_data;
		const CProperty*	m_prop;

		enum EColumn
		{
			eColumn_Offset=1,
			eColumn_Name=2,
			eColumn_Type=3,
			eColumn_Value=4,
		};

	public:
		PropInfo( const void* data, const CProperty* prop )
			: m_data( data )
			, m_prop( prop )
		{}

		virtual Bool OnCompare( const IRow* other, const Int32 columnID )
		{
			const CProperty* a = m_prop;
			const CProperty* b = ((PropInfo*)other)->m_prop;

			if ( columnID == eColumn_Name )
			{
				const CName ca = a->GetName();
				const CName cb = b->GetName();
				return Red::StringCompare( ca.AsAnsiChar(), cb.AsAnsiChar() ) < 0;
			}
			else if ( columnID == eColumn_Type )
			{
				const CName ca = a->GetType()->GetName();
				const CName cb = b->GetType()->GetName();
				return Red::StringCompare( ca.AsAnsiChar(), cb.AsAnsiChar() ) < 0;
			}

			// default sorting by property offset
			return a->GetDataOffset() < b->GetDataOffset();
		}

		virtual void OnGenerateHTML( CDebugPageHTMLDocument& doc, const Int32 columnID )
		{
			if ( columnID == eColumn_Offset )
			{
				doc.Writef( "%d", m_prop->GetDataOffset() );
			}
			else if ( columnID == eColumn_Name)
			{
				doc.Write( m_prop->GetName().AsAnsiChar() );
			}
			else if ( columnID == eColumn_Type )
			{
				doc.Write( m_prop->GetType()->GetName().AsAnsiChar() );
			}
			else if ( columnID == eColumn_Value )
			{
				CDebugPageHTMLPropertyDump dumper( doc );

				const void* propData = m_prop->GetOffsetPtr( m_data );
				dumper.DumpTypedData( m_prop->GetType(), propData );
			}
		}
	};

	// dynamic property info
	class DynPropInfo : public CDebugPageHTMLTable::IRow
	{
	public:
		CVariant			m_data;
		const IRTTIType*	m_type;
		const CName			m_name;

		enum EColumn
		{
			eColumn_Name=1,
			eColumn_Type=2,
			eColumn_Value=3,
		};

	public:
		DynPropInfo( const CName name, const IRTTIType* type, const void* data )
			: m_name( name )
			, m_type( type )
			, m_data(  type->GetName(), data )
		{
		}

		virtual Bool OnCompare( const IRow* other, const Int32 columnID )
		{
			const DynPropInfo& b = *( const DynPropInfo* ) other;

			if ( columnID == eColumn_Type )
			{
				const CName ca = m_type->GetName();
				const CName cb = b.m_type->GetName();
				return Red::StringCompare( ca.AsAnsiChar(), cb.AsAnsiChar() ) < 0;
			}

			return Red::StringCompare( m_name.AsAnsiChar(), b.m_name.AsAnsiChar() ) < 0;
		}

		virtual void OnGenerateHTML( CDebugPageHTMLDocument& doc, const Int32 columnID )
		{
			if ( columnID == eColumn_Name)
			{
				doc.Write( m_name.AsAnsiChar() );
			}
			else if ( columnID == eColumn_Type )
			{
				doc.Write( m_data.GetType().AsAnsiChar() );
			}
			else if ( columnID == eColumn_Value )
			{
				CDebugPageHTMLPropertyDump dumper( doc );
				dumper.DumpTypedData( m_data.GetRTTIType(), m_data.GetData() );
			}
		}
	};

	// child object info
	class ChildInfo : public CDebugPageHTMLTable::IRow
	{
	public:
		const CObject*		m_object;

		enum EColumn
		{
			eColumn_Class=1,
			eColumn_Ptr=2,
		};

	public:
		ChildInfo( const CObject* object )
			: m_object( object )
		{}

		virtual Bool OnCompare( const IRow* other, const Int32 columnID )
		{
			const auto* b = ((const ChildInfo*) other);

			if ( columnID == eColumn_Class )
			{
				const CName ca = m_object->GetClass()->GetName();
				const CName cb = b->m_object->GetClass()->GetName();
				return Red::StringCompare( ca.AsAnsiChar(), cb.AsAnsiChar() ) < 0;
			}

			const Uint64 ca = (Uint64) m_object;
			const Uint64 cb = (Uint64) b->m_object;
			return ca < cb;
		}

		virtual void OnGenerateHTML( CDebugPageHTMLDocument& doc, const Int32 columnID )
		{
			if ( columnID == eColumn_Class )
			{
				doc.Write( m_object->GetClass()->GetName().AsAnsiChar() );
			}
			else if ( columnID == eColumn_Ptr)
			{
				doc.Link( "/object/?id=%d", m_object->GetObjectIndex() ).Writef( "0x%016llX", (Uint64)m_object );
			}
		}
	};
	
	//! Process command, commands are always passed via URLs, return NULL if not handled
	virtual bool OnFillPage( const class CBasicURL& fullURL, const class CBasicURL& relativeURL, class CDebugPageHTMLDocument& doc ) override
	{
		// get object index
		Int32 objectID = -1;
		if ( !relativeURL.GetKey( "id", objectID ) )
			return false;

		// get object from object map that has given index
		CObject* object = nullptr;
		{
			CObjectsMap::ObjectIndexer indexer( GObjectsMap );
			object = indexer.GetObject( objectID );
			if ( !object )
			{
				doc << "<span class=\"error\">Invalid object</span>";
				return true;
			}
		}

		// get query type
		StringAnsi mode;
		relativeURL.GetKey( "action", mode );

		// dependencies
		if ( mode == "references" )
		{
			// title
			doc.Open("p").Writef( "Objects referencing object ").Doc().LinkObject( object );

			// show deps
			Uint32 numVisitedObjects = 0;
			TDynArray< Helper::CDerpFinder::Dependency > deps;
			{
				Helper::CDerpFinder scaner;
				scaner.Scan( object, deps, numVisitedObjects );
			}

			// no references
			if ( deps.Empty() )
			{
				doc.Open("p").Writef( "Visited %d objects. No reachable objects referencing given object were found", numVisitedObjects );
			}
			else
			{
				doc.Open("p").Writef( "Visited %d objects. Found %d objects referencing given object", numVisitedObjects, deps.Size() );

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

			return true;
		}

		// title
		doc.Open("p").Writef( "Object information" );

		// generic information
		{
			CDebugPageHTMLInfoBlock info( doc, "Generic information" );

			info.Info( "Object index: " ).Writef( "%d", object->GetObjectIndex() );
			info.Info( "Object class: " ).Write( object->GetClass()->GetName().AsAnsiChar() );
			info.Info( "Memory size: " ).Writef( "%d", object->GetClass()->GetSize() );

			// resource
			if ( object->IsA< CResource >() )
			{
				const CResource* res = static_cast< const CResource* >( object );
				if ( res->GetFile() )
				{
					const StringAnsi path( UNICODE_TO_ANSI( res->GetFile()->GetDepotPath().AsChar() ) );
					info.Info( "Resource file: " ).Link( "/file/%hs", path.AsChar() ).Write( path.AsChar() );
				}
			}

			// scripted size
			if ( object->HasFlag( OF_Scripted ) )
			{
				info.Info( "Scripted size: " ).Writef( "%d", object->GetClass()->GetScriptDataSize() );
			}

			// parent object
			if ( object->GetParent() != nullptr )
			{
				CObject* p = object->GetParent();
				info.Info( "Parent object: " ).Link( "/object/?id=%d", object->GetParent()->GetObjectIndex() ).
					Writef( "%d (0x%016llX) %hs", p->GetObjectIndex(), (Uint64)p, p->GetClass()->GetName().AsAnsiChar() );
			}

			// parent resource
			CObject* rootObject = object->GetRoot();
			if ( rootObject && rootObject != object && rootObject->IsA< CResource >() )
			{
				info.Info( "Parent resource: " ).LinkObject( rootObject );
			}

			// flags
			if ( object->GetFlags() != 0 )
			{
				info.Info( "Flags: " ).
					Write( object->HasFlag(OF_Finalized) ? "Finalized; " : "" ).
					Write( object->HasFlag(OF_Root) ? "Root; " : "" ).
					Write( object->HasFlag(OF_Inlined) ? "Inlined; " : "" ).
					Write( object->HasFlag(OF_Scripted) ? "Scripted; " : "" ).
					Write( object->HasFlag(OF_Discarded) ? "Discarded; " : "" ).
					Write( object->HasFlag(OF_Transient) ? "Transient; " : "" ).
					Write( object->HasFlag(OF_Referenced) ? "Referenced; " : "" ).
					Write( object->HasFlag(OF_Highlighted) ? "Highlighted; " : "" ).
					Write( object->HasFlag(OF_DefaultObject) ? "DefaultObject; " : "" ).
					Write( object->HasFlag(OF_ScriptCreated) ? "ScriptCreated;" : "" ).
					Write( object->HasFlag(OF_HasHandle) ? "HasHandle; " : "" ).
					Write( object->HasFlag(OF_WasCooked) ? "WasCooked; " : "" ).
					Write( object->HasFlag(OF_UserFlag) ? "UserFlag; " : "" );
			}
		}

		// scripted state
		if ( object->IsA< IScriptable >() )
		{
			CDebugPageHTMLInfoBlock info( doc, "Scripted state" );

			// get current state
			const IScriptable* scriptableObject = static_cast< const IScriptable* >( object );
			info.Info( "Active state: " ).Write( scriptableObject->GetCurrentStateName().AsAnsiChar() );

			// state properties
			auto state = scriptableObject->GetCurrentState();
			if ( state )
			{
				CDebugPageHTMLTable table( doc, "props" );

				// table definition
				table.AddColumn( "Offset", 150, true );
				table.AddColumn( "Name", 150, true );
				table.AddColumn( "Type", 150, true );
				table.AddColumn( "Value", 500, false );

				const auto& props = state->GetClass()->GetCachedProperties();
				for ( Uint32 i=0; i<props.Size(); ++i )
				{
					table.AddRow( new PropInfo( state, props[i] ) );
				}

				// render the table
				table.Render( 1000, "generic", fullURL );
			}
		}

		// special actions
		{
			CDebugPageHTMLInfoBlock info( doc, "Actions" );
			info.Info("").Link( "/object/?id=%d&action=references", object->GetObjectIndex() ).Write("Find references");
		}

		// custom data
		{
			object->OnDebugPageInfo( doc );
		}

		// properties
		{
			CDebugPageHTMLInfoBlock info( doc, "Properties" );
			CDebugPageHTMLTable table( doc, "props" );

			// table definition
			table.AddColumn( "Offset", 150, true );
			table.AddColumn( "Name", 150, true );
			table.AddColumn( "Type", 150, true );
			table.AddColumn( "Value", 500, false );

			const auto& props = object->GetClass()->GetCachedProperties();
			for ( Uint32 i=0; i<props.Size(); ++i )
			{
				table.AddRow( new PropInfo( object, props[i] ) );
			}

			// render the table
			table.Render( 1000, "generic", fullURL );
		}

		// dynamic properties
		{
			auto dynamicPropsInterface = object->QueryDynamicPropertiesSupplier();
			if ( dynamicPropsInterface )
			{
				TDynArray< CName > dynamicProperties;
				dynamicPropsInterface->GetDynamicProperties( dynamicProperties );
				if ( !dynamicProperties.Empty() )
				{
					CDebugPageHTMLInfoBlock info( doc, "Dynamic Properties" );
					CDebugPageHTMLTable table( doc, "props" );

					// table definition
					table.AddColumn( "Name", 150, true );
					table.AddColumn( "Type", 150, true );
					table.AddColumn( "Value", 500, false );

					for ( Uint32 i=0; i<dynamicProperties.Size(); ++i )
					{
						CName propName = dynamicProperties[i];

						CVariant value;
						if ( dynamicPropsInterface->ReadDynamicProperty( propName, value ) )
						{
							table.AddRow( new DynPropInfo( propName, value.GetRTTIType(), value.GetData() ) );
						}
					}

					// render the table
					table.Render( 1000, "generic", fullURL );
				}
			}
		}

		// children
		{
			// get object children
			TDynArray< CObject* > childObjects;
			object->GetChildren( childObjects );
			if ( !childObjects.Empty() )
			{
				CDebugPageHTMLInfoBlock info( doc, "Children" );
				CDebugPageHTMLTable table( doc, "children" );

				// table definition
				table.AddColumn( "Class", 150, true );
				table.AddColumn( "Ptr", 150, true );

				// children objects
				for ( CObject* child : childObjects )
				{
					table.AddRow( new ChildInfo( child ) );
				}

				// render the table
				table.Render( 300, "generic", fullURL );
			}
		}

		return true;
	}
};

void InitObjectDebugPages()
{
	new CDebugPageObjects();
	new CDebugPageObject();
}

#endif