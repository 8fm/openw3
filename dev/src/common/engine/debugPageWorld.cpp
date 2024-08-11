/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "../core/depot.h"
#include "../core/debugPageHandler.h"
#include "../core/debugPageHTMLDoc.h"
#include "../core/objectMap.h"

#ifndef NO_DEBUG_PAGES

#include "layer.h"
#include "dynamicLayer.h"
#include "layerInfo.h"
#include "layerGroup.h"
#include "sectorData.h"
#include "world.h"
#include "game.h"
#include "entity.h"
#include "cameraDirector.h"
#include "tagManager.h"
#include "sectorDataStreaming.h"
#include "streamingSectorData.h"
#include "streamingAreaComponent.h"
#include "../core/handleMap.h"

//-----

namespace Helper
{
	static Vector GetCameraPosition()
	{
		if ( GGame && GGame->GetActiveWorld() && GGame->GetActiveWorld()->GetCameraDirector() )
			return GGame->GetActiveWorld()->GetCameraDirector()->GetCameraPosition();

		return Vector::ZEROS;
	}
}

//-----

/// layer list in layer group
class CDebugPageLayers : public IDebugPageHandlerHTML
{
public:
	CDebugPageLayers()
		: IDebugPageHandlerHTML( "/layers/" )
	{}

private:
	// IDebugPageHandler interface
	virtual StringAnsi GetTitle() const override { return "Layers"; }
	virtual StringAnsi GetCategory() const override { return "World"; }
	virtual Bool HasIndexPage() const override { return GGame && GGame->GetActiveWorld(); }

	// information about layer
	class LayerInfo : public CDebugPageHTMLTable::IRow
	{
	public:
		StringAnsi		m_path; // path or name
		StringAnsi		m_buildTag; // layer build tag
		StringAnsi		m_state; // layer state
		Int32			m_numEntities;
		Int32			m_numCookedObjects;

	public:
		LayerInfo( const StringAnsi& path, const StringAnsi& layerBuildTag, const StringAnsi& state, const Uint32 numEntities, const Uint32 numSectorObjects )
			: m_path( path )
			, m_state( state )
			, m_buildTag( layerBuildTag )
			, m_numEntities( numEntities )
			, m_numCookedObjects( numSectorObjects )
		{
		}

		virtual Bool OnCompare( const IRow* other, const Int32 columnID ) override
		{
			const auto* b = ((const LayerInfo*)other);

			switch ( columnID )
			{
				case 1: return m_state < b->m_state;
				case 2: return m_buildTag < b->m_buildTag;
				case 3: return m_numEntities < b->m_numEntities;
				case 4: return m_numCookedObjects < b->m_numCookedObjects;
			}

			return m_path < b->m_path; // 4
		}

		virtual void OnGenerateHTML( CDebugPageHTMLDocument& doc, const Int32 columnID ) override
		{
			switch ( columnID )
			{
				case 1: doc.Write( m_state.AsChar() ); break;
				case 2: doc.Write( m_buildTag.AsChar() ); break;
				case 3: if ( m_numEntities >= 0 ) {doc.Writef("%d", m_numEntities);} break;
				case 4: if ( m_numCookedObjects >= 0 ) {doc.Writef("%d", m_numCookedObjects);} break;
				case 5: doc.Link("/layer/%hs", m_path.AsChar() ).Write(m_path.AsChar()); break;
			}
		}
	};

	//! Process command, commands are always passed via URLs, return NULL if not handled
	virtual bool OnFillPage( const class CBasicURL& fullURL, const class CBasicURL& relativeURL, class CDebugPageHTMLDocument& doc ) override
	{
		// no layers
		if ( !GGame || !GGame->GetActiveWorld() )
		{
			doc << "<span class=\"error\">No active world</span>";
			return true;
		}

		// get conformed path
		StringAnsi temp;
		const StringAnsi& safeDepotPath = CFilePath::ConformPath( relativeURL.GetPath(), temp );

		// get the dir
		CLayerGroup* group = GGame->GetActiveWorld()->GetWorldLayers()->FindGroupByPath( ANSI_TO_UNICODE( safeDepotPath.AsChar() ) );
		if ( !group )
		{
			doc << "<span class=\"error\">World does not contain specified layer group</span>";
			return true;
		}

		// display content of directory
		{
			String layerGroupPathUni;
			group->GetLayerGroupPath( layerGroupPathUni );
			doc << "<p>Content of layer group '";
			doc << UNICODE_TO_ANSI( layerGroupPathUni.AsChar() ) ;
			doc << "'</p>";
		}

		// relative path base
		doc.AddReplacementKey( "dir", relativeURL.GetPath() );

		// flat view ?
		const Bool flatMode = relativeURL.HasKey( "flat" );

		// flat list
		if ( !flatMode )
		{
			// go to flat list
			doc.Open("p").Doc().Link("/layers/%hs?flat=1", relativeURL.GetPath().AsChar() ).Write( "Show flat list");

			// print directories
			if ( group->GetSubGroups().Size() > 0 )
			{
				// collect directories
				TDynArray< CLayerGroup* > items;
				for ( auto item : group->GetSubGroups() )
				{
					items.PushBack( item );
				}

				// sort by name
				::Sort( items.Begin(), items.End(), []( CLayerGroup* a, CLayerGroup* b ) { return a->GetName() < b->GetName(); } );

				// print
				doc << "<p>Layer groups:<ul>";
				for ( auto item : items )
				{
					const StringAnsi name( UNICODE_TO_ANSI( item->GetName().AsChar() ) );
					doc.Open("li").Doc().Link("/layers/#<dir>#%hs/", name.AsChar() ).Write( name.AsChar() );
				}
				doc << "</ul></p>";
			}
		}
		else
		{
			// go to tree
			doc.Open("p").Doc().Link("/layers/%hs", relativeURL.GetPath().AsChar() ).Write( "Show tree");
		}

		// print layers
		TDynArray< CLayerInfo* > layers;
		group->GetLayers( layers, false, flatMode /* recursive if in flat mode */ );
		if ( layers.Size() > 0 )
		{
			CDebugPageHTMLInfoBlock info( doc, "Layers" );

			CDebugPageHTMLTable table( doc, "layers" );
			table.AddColumn( "State", 90, true );
			table.AddColumn( "BuildTag", 100, true );
			table.AddColumn( "Entities", 80, true );
			table.AddColumn( "Objects", 80, true );
			table.AddColumn( "Path", 1.0f, true );

			// add entries
			for ( CLayerInfo* info : layers )
			{
				// layer path
				const StringAnsi path = UNICODE_TO_ANSI( info->GetDepotPath().AsChar() );

				// layer state
				CLayer* layer = info->GetLayer();
				StringAnsi state;
				if ( layer )
				{
					if ( info->IsVisible() )
					{
						state = "Visible";
					}
					else
					{
						state = "Hidden";
					}
				}
				else
				{
					CDiskFile* file = GDepot->FindFileUseLinks( info->GetDepotPath(), 0 );
					layer = file ? Cast< CLayer >( file->GetResource() ) : nullptr;

					if ( layer )
					{
						state = "Deattached";
					}
					else
					{
						state = "Unloaded";
					}
				}

				// layer build tag
				StringAnsi buildTagName = "-";
				static const CEnum* buildTagEnum = (const CEnum*) GetTypeObject< ELayerBuildTag >();
				if ( buildTagEnum )
				{
					const auto buildTag = info->GetLayerBuildTag();
					String temp;
					if ( buildTagEnum->ToString( &buildTag, temp ) )
					{
						buildTagName = UNICODE_TO_ANSI( temp.AsChar() );
					}
				}

				// number of entities & sector objects
				Uint32 numObjects = 0;
				Uint32 numEntities = 0;
				if ( info->GetLayer() )
				{
					numEntities = info->GetLayer()->GetEntities().Size();

					if ( info->GetLayer()->GetSectorData() )
						numObjects = info->GetLayer()->GetSectorData()->GetNumObjects();
				}

				// add row to table
				table.AddRow( new LayerInfo( path, buildTagName, state, numEntities, numObjects ) );
			}

			// render table
			table.Render( 900, "generic", fullURL );
		}

		return true;
	}
};

//-----

namespace Helper
{
	// entity table (helper)
	class CDebugPageHTMLEntityTable : public CDebugPageHTMLTable
	{
	public:
		CDebugPageHTMLEntityTable( CDebugPageHTMLDocument& doc )
			: CDebugPageHTMLTable( doc, "entities" )
		{
			AddColumn( " ", 30, false ); // Icon shit
			AddColumn( "Name", 150, true );
			AddColumn( "Class", 100, true );
			AddColumn( "State", 100, true );
			AddColumn( "Tags", 150, true );
			AddColumn( "Distance", 60, true );
			AddColumn( "Ptr", 120, true );
			AddColumn( "Comps", 60, true );
			AddColumn( "Template", 1.0f, true );
		}
	};

	// information about entity on layer
	class EntityInfo : public CDebugPageHTMLTable::IRow
	{
	public:
		const CEntity*		m_entity;
		StringAnsi			m_name; // entity name if known
		CName				m_class; // entity class
		StringAnsi			m_tags; // entity tags
		CEntityTemplate*	m_template; // entity tags
		Uint32				m_numComponents;
		StringAnsi			m_state;
		Float				m_distance;

		enum EColumn
		{
			eColumn_Icon=1,
			eColumn_Name=2,
			eColumn_Class=3,
			eColumn_State=4,
			eColumn_Tags=5,
			eColumn_Distance=6,
			eColumn_Ptr=7,
			eColumn_NumComponents=8,
			eColumn_Template=9,
		};

	public:
		EntityInfo( const CEntity* entity )
			: m_entity( entity )
			, m_name( UNICODE_TO_ANSI( entity->GetName().AsChar() ) )
			, m_class( entity->GetClass()->GetName() )
			, m_tags( UNICODE_TO_ANSI( entity->GetTags().ToString().AsChar() ) )
			, m_template( entity->GetEntityTemplate() )
			, m_numComponents( entity->GetComponents().Size() )
		{
			// entity streaming state
			if ( entity->CheckStaticFlag( ESF_Streamed ) )
			{
				if ( entity->IsStreamedIn() )
				{
					m_state = "StreamedIn";
				}
				else
				{
					m_state = "StreamedOut";
				}
			}
			else
			{
				m_state = "Persistent";
			}

			// distance from camera
			m_distance = entity->GetLocalToWorld().GetTranslationRef().DistanceTo( Helper::GetCameraPosition() );
		}

		virtual Bool OnCompare( const IRow* other, const Int32 columnID ) override
		{
			const auto* b = ((const EntityInfo*)other);

			switch ( columnID )
			{
			case eColumn_Class: return Red::StringCompare( m_class.AsAnsiChar(), b->m_class.AsAnsiChar() ) < 0;
			case eColumn_State: return m_state < b->m_state;
			case eColumn_Tags: return m_tags < b->m_tags;
			case eColumn_Distance: return m_distance < b->m_distance;
			case eColumn_Ptr: return (Uint64)m_entity < (Uint64)b->m_entity;
			case eColumn_NumComponents: return m_numComponents < b->m_numComponents;
			case eColumn_Template: return (Uint64)m_template < (Uint64)b->m_template;
			}

			return m_name < b->m_name;
		}

		virtual void OnGenerateHTML( CDebugPageHTMLDocument& doc, const Int32 columnID ) override
		{
			switch ( columnID )
			{
				case eColumn_Icon: doc.LinkScript( "img_eye", "focusnode('#<host>#', %d);", m_entity->GetObjectIndex() ); return;
				case eColumn_Name: doc.Write( m_name.AsChar() ); return;
				case eColumn_Class: doc.Write( m_class.AsAnsiChar() ); return;
				case eColumn_State: doc.Write( m_state.AsChar() ); return;
				case eColumn_Tags: doc.Write( m_tags.AsChar() ); return;
				case eColumn_Distance: doc.Writef( "%1.2fm", m_distance ); return;
				case eColumn_Ptr: doc.LinkObject( m_entity ); return;
				case eColumn_NumComponents: doc.Writef( "%d", m_numComponents ); return;
				case eColumn_Template: doc.LinkObject( m_template ); return;
			}
		}
	};

	// information about component
	class ComponentInfo : public CDebugPageHTMLTable::IRow
	{
	public:
		const CComponent*	m_component;
		StringAnsi			m_name; // entity name if known
		CName				m_class; // entity class
		StringAnsi			m_tags; // entity tags

		enum EColumn
		{
			eColumn_Name=1,
			eColumn_Class=2,
			eColumn_Tags=4,
			eColumn_Ptr=6,
		};

	public:
		ComponentInfo( const CComponent* comp )
			: m_component( comp)
			, m_name( UNICODE_TO_ANSI( comp->GetName().AsChar() ) )
			, m_class( comp->GetClass()->GetName() )
			, m_tags( UNICODE_TO_ANSI( comp->GetTags().ToString().AsChar() ) )
		{
		}

		virtual Bool OnCompare( const IRow* other, const Int32 columnID ) override
		{
			const auto* b = ((const ComponentInfo*)other);

			switch ( columnID )
			{
				case eColumn_Class: return Red::StringCompare( m_class.AsAnsiChar(), b->m_class.AsAnsiChar() ) < 0;
				case eColumn_Tags: return m_tags < b->m_tags;
				case eColumn_Ptr: return (Uint64)m_component < (Uint64)b->m_component;
			}

			return m_name < b->m_name;
		}

		virtual void OnGenerateHTML( CDebugPageHTMLDocument& doc, const Int32 columnID ) override
		{
			switch ( columnID )
			{
				case eColumn_Name: doc.Write( m_name.AsChar() ); return;
				case eColumn_Class: doc.Write( m_class.AsAnsiChar() ); return;
				case eColumn_Tags: doc.Write( m_tags.AsChar() ); return;
				case eColumn_Ptr: doc.LinkObject( m_component ); return;
				}
		}
	};
}

//-----

/// info about layer
class CDebugPageLayer : public IDebugPageHandlerHTML
{
public:
	CDebugPageLayer()
		: IDebugPageHandlerHTML( "/layer/" )
	{}

private:
	// IDebugPageHandler interface
	virtual StringAnsi GetTitle() const override { return "Layer"; }
	virtual StringAnsi GetCategory() const override { return "World"; }
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
			doc << "<span class=\"error\">No layer with that name</span>";
			return true;
		}

		// get the layer
		THandle< CLayer > layer = Cast< CLayer >( file->GetResource() );
		if ( !layer )
		{
			doc << "<span class=\"error\">Layer is not loaded</span>";
			return true;
		}

		// generic stuff
		{
			CDebugPageHTMLInfoBlock info( doc, "General information" );

			info.Info("Object: ").LinkObject( layer.Get() );
			info.Info("Attached: ").Write( layer->IsAttached() ? "Yes" : "No" );
		}

		// show data
		{
			CDebugPageHTMLInfoBlock info( doc, "Entities" );
			Helper::CDebugPageHTMLEntityTable table( doc );

			const auto& entities = layer->GetEntities();
			for ( auto* entity : entities )
			{
				table.AddRow( new Helper::EntityInfo(entity) );
			}

			table.Render( 1000, "generic", fullURL );
		}

		// done
		return true;
	}
};

//-----

/// tag list
class CDebugPageTags : public IDebugPageHandlerHTML
{
public:
	CDebugPageTags()
		: IDebugPageHandlerHTML( "/tags/" )
	{}

private:
	// IDebugPageHandler interface
	virtual StringAnsi GetTitle() const override { return "Tags"; }
	virtual StringAnsi GetCategory() const override { return "World"; }
	virtual Bool HasIndexPage() const override { return GGame && GGame->GetActiveWorld(); }

	// information about tag
	class TagInfo : public CDebugPageHTMLTable::IRow
	{
	public:
		StringAnsi		m_name; // path or name
		Int32			m_numEntities;
		Int32			m_numComponents;

	public:
		TagInfo( const StringAnsi& name, const Uint32 numEntities, const Uint32 numComponents )
			: m_name( name )
			, m_numEntities( numEntities )
			, m_numComponents( numComponents )
		{
		}

		virtual Bool OnCompare( const IRow* other, const Int32 columnID ) override
		{
			const auto* b = ((const TagInfo*)other);

			switch ( columnID )
			{
				case 2: return m_numEntities < b->m_numEntities;
				case 3: return m_numComponents < b->m_numComponents;
			}

			return m_name < b->m_name; // 1
		}

		virtual void OnGenerateHTML( CDebugPageHTMLDocument& doc, const Int32 columnID ) override
		{
			switch ( columnID )
			{
				case 1: doc.Link( "/tag/?tag=%hs", m_name.AsChar() ).Write( m_name.AsChar() ); break;
				case 2: doc.Writef( "%d", m_numEntities ); break;
				case 3: doc.Writef( "%d", m_numComponents ); break;
			}
		}
	};

	//! Process command, commands are always passed via URLs, return NULL if not handled
	virtual bool OnFillPage( const class CBasicURL& fullURL, const class CBasicURL& relativeURL, class CDebugPageHTMLDocument& doc ) override
	{
		// check presence
		if ( !GGame || !GGame->GetActiveWorld() )
		{
			doc << "<span class=\"error\">No world is loaded</span>";
			return true;
		}

		// get tags
		TDynArray< CName > usedTags;
		GGame->GetActiveWorld()->GetTagManager()->GetAllTags( usedTags );

		// show data
		{
			CDebugPageHTMLInfoBlock info( doc, "Tags" );

			CDebugPageHTMLTable table( doc, "tags" );
			table.AddColumn( "Name", 240, true );
			table.AddColumn( "Entities", 120, true );

			for ( auto tag : usedTags )
			{
				TDynArray< CNode* > allNodes;
				GGame->GetActiveWorld()->GetTagManager()->CollectTaggedNodes( tag, allNodes );

				Uint32 numEntities = 0;
				for ( CNode* node : allNodes )
				{
					if ( node->IsA<CEntity>() )
						numEntities += 1;
				}

				table.AddRow( new TagInfo( tag.AsAnsiChar(), numEntities, allNodes.Size() - numEntities ) );
			}

			table.Render( 500, "generic", fullURL );
		}

		// done
		return true;
	}
};

//-----

/// info about layer
class CDebugPageTagNodes : public IDebugPageHandlerHTML
{
public:
	CDebugPageTagNodes()
		: IDebugPageHandlerHTML( "/tag/" )
	{}

private:
	// IDebugPageHandler interface
	virtual StringAnsi GetTitle() const override { return "Tagged nodes"; }
	virtual StringAnsi GetCategory() const override { return "World"; }
	virtual Bool HasIndexPage() const override { return false; }

	//! Process command, commands are always passed via URLs, return NULL if not handled
	virtual bool OnFillPage( const class CBasicURL& fullURL, const class CBasicURL& relativeURL, class CDebugPageHTMLDocument& doc ) override
	{
		// check presence
		if ( !GGame || !GGame->GetActiveWorld() )
		{
			doc << "<span class=\"error\">No world is loaded</span>";
			return true;
		}

		// get tag name
		StringAnsi tagName;
		if ( !relativeURL.GetKey( "tag", tagName ) )
		{
			doc << "<span class=\"error\">No tag name specified</span>";
			return true;
		}

		// collect all nodes
		TDynArray< CNode* > allNodes;
		GGame->GetActiveWorld()->GetTagManager()->CollectTaggedNodes( CName( ANSI_TO_UNICODE( tagName.AsChar() ) ), allNodes );

		// show data
		{
			CDebugPageHTMLInfoBlock info( doc, "Tagged Entities" );
			Helper::CDebugPageHTMLEntityTable table( doc );

			for ( auto* node : allNodes )
			{
				if ( node->IsA< CEntity >() )
				{
					table.AddRow( new Helper::EntityInfo( (CEntity*) node) );
				}
			}

			table.Render( 1000, "generic", fullURL );
		}

		// components
		{
			CDebugPageHTMLInfoBlock info( doc, "Tagged Components" );

			CDebugPageHTMLTable table( doc, "comps" );
			table.AddColumn( "Name", 150, true );
			table.AddColumn( "Class", 100, true );
			table.AddColumn( "Tags", 150, true );
			table.AddColumn( "Ptr", 120, true );

			for ( auto* node : allNodes )
			{
				if ( node->IsA< CComponent >() )
				{
					table.AddRow( new Helper::ComponentInfo( (CComponent*) node) );
				}
			}

			table.Render( 550, "generic", fullURL );
		}

		// done
		return true;
	}
};

//-----

/// info about layer
class CDebugPageDynamicLayer : public IDebugPageHandlerHTML
{
public:
	CDebugPageDynamicLayer()
		: IDebugPageHandlerHTML( "/dynamiclayer/" )
	{}

private:
	// IDebugPageHandler interface
	virtual StringAnsi GetTitle() const override { return "Dynamic Layer"; }
	virtual StringAnsi GetCategory() const override { return "World"; }
	virtual Bool HasIndexPage() const override { return GGame && GGame->GetActiveWorld(); }

	//! Process command, commands are always passed via URLs, return NULL if not handled
	virtual bool OnFillPage( const class CBasicURL& fullURL, const class CBasicURL& relativeURL, class CDebugPageHTMLDocument& doc ) override
	{
		// check presence
		if ( !GGame || !GGame->GetActiveWorld() )
		{
			doc << "<span class=\"error\">Dynamic Layer not found (no world is loaded)</span>";
			return true;
		}

		// generic stuff
		{
			CDebugPageHTMLInfoBlock info( doc, "General information" );

			info.Info("Object: ").LinkObject( GGame->GetActiveWorld()->GetDynamicLayer() );
		}

		// show data
		{
			CDebugPageHTMLInfoBlock info( doc, "Dynamic Entities" );
			Helper::CDebugPageHTMLEntityTable table( doc );

			const auto& entities = GGame->GetActiveWorld()->GetDynamicLayer()->GetEntities();
			for ( auto* entity : entities )
			{
				table.AddRow( new Helper::EntityInfo(entity) );
			}

			table.Render( 1000, "generic", fullURL );
		}

		// done
		return true;
	}
};

//-----

namespace Helper
{
	Uint32 CountEntitiesInRange( const TDynArray< CEntity* >& entities, const Uint32 range )
	{
		Uint32 num = 0;

		const Vector cameraPos = Helper::GetCameraPosition();
		for ( CEntity* entity : entities )
		{
			const Float dist = entity->GetWorldPositionRef().DistanceTo( cameraPos );
			if ( dist <= (Float) range )
			{
				num += 1;
			}
		}
		return num;
	}
}

/// list entities near the camera
class CDebugPageNearbyEntities : public IDebugPageHandlerHTML
{
public:
	CDebugPageNearbyEntities()
		: IDebugPageHandlerHTML( "/nearentities/" )
	{}

private:
	// IDebugPageHandler interface
	virtual StringAnsi GetTitle() const override { return "Nearby Entities"; }
	virtual StringAnsi GetCategory() const override { return "World"; }
	virtual Bool HasIndexPage() const override { return GGame && GGame->GetActiveWorld(); }

	//! Process command, commands are always passed via URLs, return NULL if not handled
	virtual bool OnFillPage( const class CBasicURL& fullURL, const class CBasicURL& relativeURL, class CDebugPageHTMLDocument& doc ) override
	{
		// check presence
		if ( !GGame || !GGame->GetActiveWorld() )
		{
			doc << "<span class=\"error\">No world is loaded</span>";
			return true;
		}

		// cutoff distance
		Int32 range = 20;
		relativeURL.GetKey( "range ", range );

		// get all entities from world
		TDynArray< CEntity* > allEntities;
		TDynArray< CLayerInfo* > allLayers;
		GGame->GetActiveWorld()->GetWorldLayers()->GetLayers( allLayers, true, true );
		for ( CLayerInfo* layer : allLayers )
		{
			CLayer* loadedLayer = layer->GetLayer();
			if ( loadedLayer && loadedLayer->IsAttached() )
			{
				loadedLayer->GetEntities( allEntities );
			}
		}

		// get entities from dynamic layer
		GGame->GetActiveWorld()->GetDynamicLayer()->GetEntities( allEntities );

		// range selection
		{
			CDebugPageHTMLInfoBlock info( doc, "Nearby Entities" );

			const Int32 ranges[] = { 5, 10, 20, 50, 100 };
			for ( Uint32 i=0; i<ARRAY_COUNT(ranges); ++i )
			{
				CBasicURL linkURL(fullURL);
				linkURL.SetKey( "range", (Int32)ranges[i] );

				const Uint32 numEntities = Helper::CountEntitiesInRange( allEntities, ranges[i] );
				info.Info( "Range %dm: ", ranges[i] ).Open("a" ).Attr( "href", linkURL.ToString().AsChar() ).Writef( "(%d entities)", numEntities );
			}
		}

		// show data
		{
			CDebugPageHTMLInfoBlock info( doc, "Nearby Entities" );
			Helper::CDebugPageHTMLEntityTable table( doc );

			const Vector cameraPos = Helper::GetCameraPosition();
			for ( auto* entity : allEntities )
			{
				const Float dist = entity->GetWorldPositionRef().DistanceTo( cameraPos );
				if ( dist <= range )
				{
					table.AddRow( new Helper::EntityInfo(entity) );
				}
			}

			table.Render( 1000, "generic", fullURL );
		}

		// done
		return true;
	}
};

//----

/// list entities near the camera
class CDebugPageStreamedEntities : public IDebugPageHandlerHTML
{
public:
	CDebugPageStreamedEntities()
		: IDebugPageHandlerHTML( "/streamedentities/" )
	{}

private:
	// IDebugPageHandler interface
	virtual StringAnsi GetTitle() const override { return "Streamed Entities"; }
	virtual StringAnsi GetCategory() const override { return "World"; }
	virtual Bool HasIndexPage() const override { return GGame && GGame->GetActiveWorld(); }

	//! Process command, commands are always passed via URLs, return NULL if not handled
	virtual bool OnFillPage( const class CBasicURL& fullURL, const class CBasicURL& relativeURL, class CDebugPageHTMLDocument& doc ) override
	{
		// check presence
		if ( !GGame || !GGame->GetActiveWorld() )
		{
			doc << "<span class=\"error\">No world is loaded</span>";
			return true;
		}

		// get all entities from world
		TDynArray< CEntity* > allEntities;
		TDynArray< CLayerInfo* > allLayers;
		GGame->GetActiveWorld()->GetWorldLayers()->GetLayers( allLayers, true, true );
		for ( CLayerInfo* layer : allLayers )
		{
			CLayer* loadedLayer = layer->GetLayer();
			if ( loadedLayer && loadedLayer->IsAttached() )
			{
				loadedLayer->GetEntities( allEntities );
			}
		}

		// get entities from dynamic layer
		GGame->GetActiveWorld()->GetDynamicLayer()->GetEntities( allEntities );

		// show data
		{
			CDebugPageHTMLInfoBlock info( doc, "Streamed Entities" );
			Helper::CDebugPageHTMLEntityTable table( doc );

			for ( CEntity* entity : allEntities )
			{				
				if ( entity->CheckStaticFlag( ESF_Streamed ) && entity->IsStreamedIn() )
				{
					table.AddRow( new Helper::EntityInfo(entity) );
				}
			}

			table.Render( 1000, "generic", fullURL );
		}

		// done
		return true;
	}
};

//----

/// entity streaming data
class CDebugPageStreamedStreamingData : public IDebugPageHandlerHTML
{
public:
	CDebugPageStreamedStreamingData()
		: IDebugPageHandlerHTML( "/streamingdata/" )
	{}

private:
	// IDebugPageHandler interface
	virtual StringAnsi GetTitle() const override { return "Streaming data"; }
	virtual StringAnsi GetCategory() const override { return "World"; }
	virtual Bool HasIndexPage() const override { return false; }

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

		// not an entity
		CEntity* entity = Cast< CEntity >( object );
		if ( !entity )
		{
			doc << "<span class=\"error\">Not an entity</span>";
			return true;
		}

		// get streaming data
		const auto& data = entity->GetLocalStreamedComponentDataBuffer();
		if ( data.GetSize() == 0 )
		{
			doc << "<span class=\"error\">There's no streaming data in the entity</span>";
			return true;
		}

		// create components - note, we will LEAK IT (that's cool)
		CMemoryFileReader reader( (const Uint8*) data.GetData(), data.GetSize(), 0 );
		CDependencyLoader loader( reader, nullptr );
		DependencyLoadingContext context;
		if ( !loader.LoadObjects( context ) )
		{
			doc << "<span class=\"error\">Failed to deserialize streaming buffer</span>";
			return true;
		}

		// dump loaded objects
		{
			CDebugPageHTMLInfoBlock info( doc, "Objects in streaming buffer" );

			CDebugPageHTMLTable table( doc, "comps" );
			table.AddColumn( "Index", 70, true );
			table.AddColumn( "Ptr", 400, true );

			for ( Uint32 i=0; i<context.m_loadedRootObjects.Size(); ++i )
			{
				table.AddRow( 
					table.CreateRowData((Int64)i),
					table.CreateRowData(context.m_loadedRootObjects[i]) );
			}

			table.Render( 550, "generic", fullURL );
		}

		// done
		return true;
	}
};

//----

namespace Helper
{
	extern void DumpFileTables( const CDependencyLoader& loader, class CDebugPageHTMLDocument& doc, const CBasicURL& url );
}

/// entity streaming data - tables
class CDebugPageStreamedStreamingDataTables : public IDebugPageHandlerHTML
{
public:
	CDebugPageStreamedStreamingDataTables()
		: IDebugPageHandlerHTML( "/streamingdatatables/" )
	{}

private:
	// IDebugPageHandler interface
	virtual StringAnsi GetTitle() const override { return "Streaming data tables"; }
	virtual StringAnsi GetCategory() const override { return "World"; }
	virtual Bool HasIndexPage() const override { return false; }

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

		// not an entity
		CEntity* entity = Cast< CEntity >( object );
		if ( !entity )
		{
			doc << "<span class=\"error\">Not an entity</span>";
			return true;
		}

		// get streaming data
		const auto& data = entity->GetLocalStreamedComponentDataBuffer();
		if ( data.GetSize() == 0 )
		{
			doc << "<span class=\"error\">There's no streaming data in the entity</span>";
			return true;
		}

		// create components - note, we will LEAK IT (that's cool)
		CMemoryFileReader reader( (const Uint8*) data.GetData(), data.GetSize(), 0 );
		CDependencyLoader loader( reader, nullptr );
		loader.LoadTables();
		Helper::DumpFileTables( loader, doc, fullURL );

		// done
		return true;
	}
};

//----

class CDebugHTMLRowStreamingAreaStatus : public CDebugPageHTMLTable::IRowData
{
public:
	CDebugHTMLRowStreamingAreaStatus( THandle< CStreamingAreaComponent > sa )
		: m_area( sa )
	{
	}

	virtual Bool OnCompare( const IRowData* other ) const
	{
		return false;
	}

	virtual void OnGenerateHTML( CDebugPageHTMLDocument& doc ) const
	{
		const auto tag = m_area->GetEntity()->GetTags().GetTag(0);
		if ( tag )
		{
			const bool isEnabled = GGame->IsStreamingLockdownEnabled( tag );
			if ( isEnabled )
			{
				doc.Link( "/streaminglocks/?zoneoff=%hs", tag.AsAnsiChar() ).Write( "(Enabled)" );
			}
			else
			{
				doc.Link( "/streaminglocks/?zoneon=%hs", tag.AsAnsiChar() ).Write( "(Disabled)" );
			}
		}
		else
		{
			doc.Write( "(Unknown)" );
		}
	}


private:
	THandle< CStreamingAreaComponent >		m_area;
};

/// entity streaming data - tables
class CDebugPageStreamedStreamingLocks : public IDebugPageHandlerHTML
{
public:
	CDebugPageStreamedStreamingLocks()
		: IDebugPageHandlerHTML( "/streaminglocks/" )
	{}

private:
	// IDebugPageHandler interface
	virtual StringAnsi GetTitle() const override { return "Streaming locks"; }
	virtual StringAnsi GetCategory() const override { return "World"; }
	virtual Bool HasIndexPage() const override { return true; }

	//! Process command, commands are always passed via URLs, return NULL if not handled
	virtual bool OnFillPage( const class CBasicURL& fullURL, const class CBasicURL& relativeURL, class CDebugPageHTMLDocument& doc ) override
	{
		// check presence
		if ( !GGame || !GGame->GetActiveWorld() )
		{
			doc << "<span class=\"error\">No world is loaded</span>";
			return true;
		}

		// get all entities from world
		TDynArray< CEntity* > allEntities;
		TDynArray< CLayerInfo* > allLayers;
		GGame->GetActiveWorld()->GetWorldLayers()->GetLayers( allLayers, true, true );
		for ( CLayerInfo* layer : allLayers )
		{
			CLayer* loadedLayer = layer->GetLayer();
			if ( loadedLayer && loadedLayer->IsAttached() )
			{
				loadedLayer->GetEntities( allEntities );
			}
		}

		// get entities from dynamic layer
		GGame->GetActiveWorld()->GetDynamicLayer()->GetEntities( allEntities );

		// find streaming area components
		TDynArray< CStreamingAreaComponent* > streamingAreas;
		for ( CEntity* ptr : allEntities )
		{
			const auto& comps = ptr->GetComponents();
			for ( CComponent* comp : comps )
			{
				if ( comp && comp->IsA< CStreamingAreaComponent >() )
					streamingAreas.PushBack( (CStreamingAreaComponent*) comp );
			}
		}

		// turning some zone on
		{
			StringAnsi zoneName;
			if ( relativeURL.GetKey( "zoneon", zoneName ) )
			{				
				const String zoneNameUni( ANSI_TO_UNICODE( zoneName.AsChar() ) );
				GGame->EnableStreamingLockdown( zoneNameUni, CName( zoneNameUni.AsChar() ) );
			}
			else if ( relativeURL.GetKey( "zoneoff", zoneName ) )
			{				
				const String zoneNameUni( ANSI_TO_UNICODE( zoneName.AsChar() ) );
				GGame->DisableStreamingLockdown( CName( zoneNameUni.AsChar() ) );
			}
		}

		// current streaming status
		{
			CDebugPageHTMLInfoBlock info( doc, "Sector streaming stats" );

			SSectorStreamingDebugData debugInfo;
			GGame->GetActiveWorld()->GetSectorDataStreaming()->GetDebugInfo( debugInfo );

			info.Info( "Total objects: ").Writef( "%d", debugInfo.m_numObjectsRegistered );
			info.Info( "Objects in range: ").Writef( "%d", debugInfo.m_numObjectsInRange );
			info.Info( "Objects streaming: ").Writef( "%d", debugInfo.m_numObjectsStreaming );
			info.Info( "Objects streamed: ").Writef( "%d", debugInfo.m_numObjectsStreamed );
			info.Info( "Objects locked: ").Writef( "%d", debugInfo.m_numObjectsLocked );
		}

		// current streaming status
		{
			CDebugPageHTMLInfoBlock info( doc, "Entity streaming stats" );

			SStreamingDebugData debugInfo;
			GGame->GetActiveWorld()->GetStreamingSectorData()->GetDebugInfo( debugInfo );

			info.Info( "Total entities: ").Writef( "%d", debugInfo.m_numProxiesRegistered );
			info.Info( "Entities in range: ").Writef( "%d", debugInfo.m_numProxiesInRange );
			info.Info( "Entities streaming: ").Writef( "%d", debugInfo.m_numProxiesStreaming );
			info.Info( "Entities streamed: ").Writef( "%d", debugInfo.m_numProxiesStreamed );
			info.Info( "Entities locked: ").Writef( "%d", debugInfo.m_numProxiesLocked );
			info.Info( "Grid levels: ").Writef( "%d", debugInfo.m_numGridLevels );
			info.Info( "Grid buckets used: ").Writef( "%d", debugInfo.m_numGridBucketsUsed );
			info.Info( "Grid buckets max: ").Writef( "%d", debugInfo.m_numGridBucketsMax );
		}

		// show table with streaming locks
		{
			CDebugPageHTMLInfoBlock info( doc, "Streaming lock areas" );

			CDebugPageHTMLTable table( doc, "areas" );
			table.AddColumn( "Lock", 200, true );
			table.AddColumn( "Component", 400, true );
			table.AddColumn( "Status", 70, true );

			for ( const auto* ptr : streamingAreas )
			{
				const auto tag = ptr->GetEntity()->GetTags().GetTag(0);
				table.AddRow( table.CreateRowData( tag.AsAnsiChar() ), 
					table.CreateRowData( ptr ),
					new CDebugHTMLRowStreamingAreaStatus( ptr ) );
			}

			table.Render( 800, "generic", fullURL );
		}

		// done
		return true;
	}
};

void InitLayerDebugPages()
{
	new CDebugPageLayers(); // autoregister
	new CDebugPageLayer(); // autoregister
	new CDebugPageDynamicLayer(); // autoregister
	new CDebugPageTags(); // autoregister
	new CDebugPageTagNodes(); // autoregister
	new CDebugPageNearbyEntities(); // autoregister
	new CDebugPageStreamedEntities(); // autoregister
	new CDebugPageStreamedStreamingData(); // autoregister
	new CDebugPageStreamedStreamingDataTables(); // autoregister
	new CDebugPageStreamedStreamingLocks(); // autoregister
}


#endif