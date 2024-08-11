/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "debugPageHandler.h"
#include "debugPageHTMLDoc.h"

#ifndef NO_DEBUG_PAGES

#include "objectMap.h"
#include "resource.h"
#include "diskFile.h"

/// List of all resources
class CDebugPageResources : public IDebugPageHandlerHTML
{
public:
	CDebugPageResources()
		: IDebugPageHandlerHTML( "/resources/" )
	{}

private:
	// IDebugPageHandler interface
	virtual StringAnsi GetTitle() const override { return "Resource list"; }
	virtual StringAnsi GetCategory() const override { return "Core"; }
	virtual Bool HasIndexPage() const override { return true; }

	class ResourceInfo : public CDebugPageHTMLTable::IRow
	{
	public:
		enum EColumn
		{
			eColumn_Class=1,
			eColumn_State=2,
			eColumn_Ptr=3,
			eColumn_Path=4,
		};

		enum EState
		{
			eState_Loaded,
			eState_Quarantined,
		};

		const CResource*	m_resource;
		EState				m_state;

	public:
		ResourceInfo( const CResource* object )
			: m_resource( object )
		{
			if ( object->GetFile()->IsQuarantined() )
			{
				m_state = eState_Quarantined;
			}
			else
			{
				m_state = eState_Loaded;
			}
		}

		virtual Bool OnCompare( const IRow* other, const Int32 columnID )
		{
			const CResource* a = m_resource;
			const CResource* b = ((ResourceInfo*)other)->m_resource;

			if ( columnID == eColumn_Path )
			{
				const String pa = a->GetFile()->GetDepotPath();
				const String pb = b->GetFile()->GetDepotPath();
				return pa < pb;
			}
			else if ( columnID == eColumn_Class )
			{
				const CName ca = a->GetClass()->GetName();
				const CName cb = b->GetClass()->GetName();
				return Red::StringCompare( ca.AsAnsiChar(), cb.AsAnsiChar() ) < 0;
			}
			else if ( columnID == eColumn_State )
			{
				return (int)m_state < (int)(((ResourceInfo*)other)->m_state);
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
			if ( columnID == eColumn_Path )
			{
				const StringAnsi path( UNICODE_TO_ANSI( m_resource->GetDepotPath().AsChar() ) );
				doc.Link( "/file/%hs", path.AsChar() ).Write( path.AsChar() );
			}
			else if ( columnID == eColumn_State )
			{
				if ( m_state == eState_Loaded )
				{
					doc.Write( "Loaded" );
				}
				else if ( m_state == eState_Quarantined )
				{
					doc.Write( "Quarantined" );
				}
			}
			else if ( columnID == eColumn_Class )
			{
				doc.Write( m_resource->GetClass()->GetName().AsAnsiChar() );
			}
			else if ( columnID == eColumn_Ptr )
			{
				doc.Link( "/object/?id=%d", m_resource->GetObjectIndex() ).Writef( "0x%016llX", (Uint64)m_resource );
			}
		}
	};

	//! Process command, commands are always passed via URLs, return NULL if not handled
	virtual bool OnFillPage( const class CBasicURL& fullURL, const class CBasicURL& relativeURL, class CDebugPageHTMLDocument& doc ) override
	{
		// count default/root and scripted objects
		Uint32 numResourcesLoaded = 0;
		Uint32 numResourcesQuarantined = 0;

		TDynArray< const CResource* > resources;
		{
			TDynArray< CObject* > allObjects;
			GObjectsMap->GetAllObjects( allObjects, ~0, 0 );

			for ( CObject* object : allObjects )
			{
				if ( object->IsA<CResource>() )
				{
					const CResource* res = static_cast< const CResource* >( object );
					if ( res->GetFile() )
					{
						resources.PushBack( res );

						if ( res->GetFile()->IsQuarantined() )
						{
							numResourcesQuarantined += 1;
						}
						else if ( res->GetFile()->IsLoaded() )
						{
							numResourcesLoaded += 1;
						}
					}
				}
			}
		}

		// basic file information
		{
			CDebugPageHTMLInfoBlock info( doc, "General information" );

			info.Info( "All resources: " ).Writef( "%d  ", resources.Size() );
			info.Info( "Loaded: " ).Writef( "%d  ", numResourcesLoaded );
			info.Info( "Quarantined: " ).Writef( "%d  ", numResourcesQuarantined );
		}

		// object table
		{
			CDebugPageHTMLInfoBlock info( doc, "Resource list" );
			CDebugPageHTMLTable table( doc, "obj" );

			// table definition
			table.AddColumn( "Class", 300, true );
			table.AddColumn( "State", 50, true );
			table.AddColumn( "Ptr", 150, true );
			table.AddColumn( "Path", 500, true );

			// push objects to table
			for ( const CResource* res : resources )
			{
				table.AddRow( new ResourceInfo( res ) );
			}

			// render the table
			table.Render( 1000, "generic", fullURL );
		}

		return true;
	}
};

void InitResourceDebugPages()
{
	new CDebugPageResources();
}

#endif