/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "../core/depot.h"
#include "../core/debugPageHandler.h"
#include "../core/debugPageHTMLDoc.h"
#include "../core/httpResponseData.h"
#include "../engine/world.h"
#include "../engine/layerGroup.h"
#include "../engine/layer.h"
#include "../engine/layerInfo.h"
#include "../engine/layerStorage.h"

#ifndef NO_DEBUG_PAGES

//-----

/// layer storage link
class CDebugPageHTMLRowLayerStorage : public CDebugPageHTMLRowString
{
public:
	CDebugPageHTMLRowLayerStorage( const StringAnsi& txt )
		: CDebugPageHTMLRowString( txt )
	{};

	virtual void OnGenerateHTML( CDebugPageHTMLDocument& doc ) const
	{
		doc.Link( "/layerstorage/%hs", m_data.AsChar() ).Write( m_data.AsChar() );
	}
};

/// list of layer storages in the world
class CDebugPageLayerStorageList : public IDebugPageHandlerHTML
{
public:
	CDebugPageLayerStorageList()
		: IDebugPageHandlerHTML( "/layerstoragelist/" )
	{}

private:
	// IDebugPageHandler interface
	virtual StringAnsi GetTitle() const override { return "Layer Storage"; }
	virtual StringAnsi GetCategory() const override { return "Game"; }
	virtual Bool HasIndexPage() const override { return GGame && GGame->GetActiveWorld(); }

	//! Process command, commands are always passed via URLs, return NULL if not handled
	virtual bool OnFillPage( const class CBasicURL& fullURL, const class CBasicURL& relativeURL, class CDebugPageHTMLDocument& doc ) override
	{
		// no layers
		if ( !GGame || !GGame->GetActiveWorld() )
		{
			doc << "<span class=\"error\">No active world</span>";
			return true;
		}

		// get all the layer (I mean all)
		TDynArray< CLayerInfo* > layerInfo;
		GGame->GetActiveWorld()->GetWorldLayers()->GetLayers( layerInfo, false, true );

		// get global counts
		Uint32 numTotalLayers = 0;
		Uint32 numTotalEntities = 0;
		Uint32 numTotalComponents = 0;
		Uint32 numTotalDataSize = 0;
		Uint32 numTotalStorages = 0;
		Uint32 numTotalEmptyStorages = 0;
		for ( CLayerInfo* info : layerInfo )
		{
			numTotalLayers += 1;

			// TODO: env layers will NOT have layer storage
			CLayerStorage* storage = info->GetLayerStorage();
			if ( storage )
			{
				if ( storage->GetNumEntities() > 0 )
				{
					numTotalEntities += storage->GetNumEntities();
					numTotalComponents += storage->GetNumComponents();
					numTotalDataSize += storage->GetDataSize();
					numTotalStorages += 1;
				}
				else
				{
					numTotalEmptyStorages += 1;
				}
			}
		}

		// show global counts
		{
			CDebugPageHTMLInfoBlock info( doc, "Global stats" );

			info.Info("Number of layers: ").Writef("%d", numTotalLayers);
			info.Info("Number of storages: ").Writef("%d (%d empty)", numTotalStorages, numTotalEmptyStorages);
			info.Info("Number of entities: ").Writef("%d", numTotalEntities);
			info.Info("Number of components: ").Writef("%d", numTotalComponents);
			info.Info("Data size: ").Writef("%1.2f KB", numTotalDataSize / 1024.0f);
		}

		// show storage table
		{
			CDebugPageHTMLInfoBlock info( doc, "Storage list" );

			CDebugPageHTMLTable table( doc, "storage" );
			table.AddColumn( "Entities", 90, true );
			table.AddColumn( "Components", 90, true );
			table.AddColumn( "Data Size", 90, true );
			table.AddColumn( "Layer", 500, true );

			for ( CLayerInfo* info : layerInfo )
			{
				String path;
				info->GetHierarchyPath( path, true );

				CLayerStorage* storage = info->GetLayerStorage();
				if ( storage && (storage->GetDataSize() > 0) )
				{
					table.AddRow(
						table.CreateRowData((Int64)storage->GetNumEntities()),
						table.CreateRowData((Int64)storage->GetNumComponents()),
						table.CreateRowData((Int64)storage->GetDataSize()),
						new CDebugPageHTMLRowLayerStorage(UNICODE_TO_ANSI(path.AsChar()))
					);
				}
			}

			table.Render(800, "generic", doc.GetURL());
		}

		return true;
	}
};

//-----

/// entity storage link
class CDebugPageHTMLRowEntityStorage : public CDebugPageHTMLRowString
{
public:
	static StringAnsi GetStringFromTag( const IdTag& tag )
	{
		String ret;
		tag.ToString(ret);
		return UNICODE_TO_ANSI( ret.AsChar() );
	}

	CDebugPageHTMLRowEntityStorage( const IdTag& tag )
		: CDebugPageHTMLRowString( GetStringFromTag( tag ) )
		, m_tag( tag )
	{};

	virtual void OnGenerateHTML( CDebugPageHTMLDocument& doc ) const
	{
		const CGUID& guid = m_tag.GetGuid();
		doc.Link( "/entitystorage/%08X-%08X-%08X-%08X", guid.parts.A, guid.parts.B, guid.parts.C, guid.parts.D ).Write( m_data.AsChar() );
	}

protected:
	IdTag		m_tag;
};

/// content of single layer storage
class CDebugPageLayerStorage : public IDebugPageHandlerHTML
{
public:
	CDebugPageLayerStorage()
		: IDebugPageHandlerHTML( "/layerstorage/" )
	{}

private:
	// IDebugPageHandler interface
	virtual StringAnsi GetTitle() const override { return "Layer Storage"; }
	virtual StringAnsi GetCategory() const override { return "Game"; }
	virtual Bool HasIndexPage() const override { return false; }

	//! Process command, commands are always passed via URLs, return NULL if not handled
	virtual bool OnFillPage( const class CBasicURL& fullURL, const class CBasicURL& relativeURL, class CDebugPageHTMLDocument& doc ) override
	{
		// get conformed path
		StringAnsi temp;
		const StringAnsi& safeDepotPath = CFilePath::ConformPath( relativeURL.GetPath(), temp );

		// no world
		if ( !GGame || !GGame->GetActiveWorld() )
		{
			doc << "<span class=\"error\">No world loaded</span>";
			return true;
		}

		// find matching layer info
		CLayerInfo* layer = GGame->GetActiveWorld()->GetWorldLayers()->FindLayerByPath( ANSI_TO_UNICODE( safeDepotPath.AsChar() ) );
		if ( !layer )
		{
			doc << "<span class=\"error\">Unknown layer</span>";
			return true;
		}

		// general information
		{
			CDebugPageHTMLInfoBlock info( doc, "Layer information" );

			info.Info( "File: " ).LinkFile( UNICODE_TO_ANSI( layer->GetDepotPath().AsChar() ) );

			if ( layer->GetLayer() )
			{
				info.Info( "Layer: " ).LinkObject( layer->GetLayer() );
				info.Info( "Attached: " ).Write( layer->GetLayer()->IsAttached() ? "Yes" : "No" );
			}
		}

		// layer storage information
		CLayerStorage* storage = layer->GetLayerStorage();
		if ( storage )
		{
			CDebugPageHTMLInfoBlock info( doc, "Storage stats" );

			info.Info("Number of entities: ").Writef("%d", storage->GetNumEntities() );
			info.Info("Number of components: ").Writef("%d", storage->GetNumComponents() );
			info.Info("Data size: ").Writef("%1.3f KB", storage->GetDataSize() / 1024.0f);
		}

		// entities :)
		{
			CDebugPageHTMLInfoBlock info( doc, "Storage data" );

			CDebugPageHTMLTable table( doc, "entities" );
			table.AddColumn( "IDTag", 350 );
			table.AddColumn( "Components", 90, true );
			table.AddColumn( "aSize", 50, true );
			table.AddColumn( "Entity", 300 );

			for ( const auto& it : storage->m_entities )
			{
				const auto* data = it;

				// find entity in layer
				CEntity* resolvedEntity = GGame->GetActiveWorld()->FindPersistentEntity( data->m_idTag );

				table.AddRow(
					new CDebugPageHTMLRowEntityStorage( data->m_idTag ),
					table.CreateRowData( (Int64) data->GetNumComponents() ),
					table.CreateRowData( (Int64) data->GetDataSize() ),
					table.CreateRowData( resolvedEntity )
				);
			}

			table.Render( 850, "generic", doc.GetURL() );
		}

		// handled
		return true;
	}
};

//-----

class CDebugPageEntityStorageDumper : public IGameSaveDumper
{
public:
	CDebugPageEntityStorageDumper( CDebugPageHTMLDocument& doc )
		: m_doc( doc )
		, m_indent( 0 )
		, m_payloadSize( 0 )
		, m_numBlocks( 0 )
	{
		Red::MemoryZero( &m_indentBuffer, sizeof(m_indentBuffer) );
		m_doc.Write("<pre>");
	}

	~CDebugPageEntityStorageDumper()
	{
		m_doc.Write("</pre>");
	}

	virtual void OnBlockStart( const Uint32 offset, const Uint32 blockSize, const CName blockName ) override
	{
		m_numBlocks += 1;
		m_doc.Writef("[%05d]%sBlock %s (size %d)\n", offset, m_indentBuffer, blockName.AsAnsiChar(), blockSize );
		Indent();
	}

	virtual void OnBlockEnd( const Uint32 offset ) override
	{
		m_numBlocks += 1;
		Unindent();
		m_doc.Writef("[%05d]%sEndBlock\n", offset, m_indentBuffer );
	}

	virtual void OnStorageStart( const Uint32 offset, const Uint32 size ) override
	{
		m_numBlocks += 1;
		m_doc.Writef("[%05d]%sStorage (size %d)\n", offset, m_indentBuffer, size );
		Indent();
	}

	virtual void OnStorageEnd( const Uint32 offset ) override
	{
		m_numBlocks += 1;
		Unindent();
		m_doc.Writef("[%05d]%sEndStorage\n", offset, m_indentBuffer );
	}

	virtual void OnValue( const Uint32 offset, const Bool isProp, const CName name, const IRTTIType* type, const Uint32 dataSize, const void* dataPtr ) override
	{
		m_numBlocks += 1;
		m_payloadSize += dataSize;
		m_doc.Writef("[%05d]%sValue %hs, type %hs, size %d\n", offset, m_indentBuffer, name.AsAnsiChar(), type ? type->GetName().AsAnsiChar() : "Unknown", dataSize );

		String valueString;
		if ( type->ToString( dataPtr, valueString ) )
		{
			m_doc.Writef("[%05d]%s  '%s'\n", offset, m_indentBuffer, UNICODE_TO_ANSI(valueString.AsChar()) );
		}
	}

	virtual void OnError( const Uint32 offset, const AnsiChar* txt, ... ) override
	{
		AnsiChar buf[256];
		va_list args;

		va_start(args, txt);
		Red::VSNPrintF( buf, ARRAY_COUNT(buf), txt, args );
		va_end(args);

		m_doc.Writef("[%05d]%sError %s\n", offset, m_indentBuffer, buf );
	}

	RED_INLINE const Uint32 GetPayloadSize() const
	{
		return m_payloadSize;
	}

	RED_INLINE const Uint32 GetNumBlocks() const
	{
		return m_numBlocks;
	}

private:
	static const Uint32 MAX_INDENT = 64;

	void Indent()
	{
		if ( m_indent < MAX_INDENT )
		{
			m_indentBuffer[ m_indent+0 ] = ' ';
			m_indentBuffer[ m_indent+1 ] = ' ';
			m_indentBuffer[ m_indent+2 ] = ' ';
		}

		m_indent += 3;
	}

	void Unindent()
	{
		if ( m_indent > 0 )
		{
			m_indent -= 3;

			if ( m_indent < MAX_INDENT )
				m_indentBuffer[ m_indent ] = 0;
		}
	}

	Uint32						m_indent;
	AnsiChar					m_indentBuffer[ MAX_INDENT+1 ];
	CDebugPageHTMLDocument&		m_doc;

	Uint32						m_payloadSize;
	Uint32						m_numBlocks;
};

/// content of layer storage for given entity
class CDebugPageEntityStorage : public IDebugPageHandlerHTML
{
public:
	CDebugPageEntityStorage()
		: IDebugPageHandlerHTML( "/entitystorage/" )
	{}

private:
	// IDebugPageHandler interface
	virtual StringAnsi GetTitle() const override { return "Entity Storage"; }
	virtual StringAnsi GetCategory() const override { return "Game"; }
	virtual Bool HasIndexPage() const override { return false; }

	//! Process command, commands are always passed via URLs, return NULL if not handled
	virtual bool OnFillPage( const class CBasicURL& fullURL, const class CBasicURL& relativeURL, class CDebugPageHTMLDocument& doc ) override
	{
		// no world
		if ( !GGame || !GGame->GetActiveWorld() )
		{
			doc << "<span class=\"error\">No world loaded</span>";
			return true;
		}

		// get conformed path
		CGUID guid;
		const String guidName = ANSI_TO_UNICODE( relativeURL.GetPath().AsChar() );
		if ( !guid.FromString( guidName.AsChar() ) )
		{
			doc << "<span class=\"error\">Unable to parse GUID</span>";
			return true;
		}

		// find layer storage with data for given entity
		TDynArray< CLayerInfo* > allLayers;
		GGame->GetActiveWorld()->GetWorldLayers()->GetLayers( allLayers, false, true );

		// scan layers
		const CLayerStorage::EntityData* entityData = nullptr;
		for ( CLayerInfo* layerInfo : allLayers )
		{
			CLayerStorage* storage = layerInfo->GetLayerStorage();
			if ( storage )
			{
				// HACK!
				struct
				{
					CGUID	m_guid;
					Bool	m_isDynamic;
				} x;

				x.m_guid = guid;
				x.m_isDynamic = false;

				entityData = storage->FindEntityData( *(const IdTag*) &x );
				if ( entityData )
				{
					break;
				}
			}
		}

		// no data found :*(
		if ( !entityData || !entityData->m_data )
		{
			doc << "<span class=\"error\">No data found in storage</span>";
			return true;
		}

		// dump content of the entity 
		Uint32 totalPayloadSize = 0;
		Uint32 totalBlockCount = 0;
		{
			CDebugPageHTMLInfoBlock info( doc, "Entity data" );

			{
				CDebugPageEntityStorageDumper dumper( doc );
				CGameSaveManager::DumpContent( entityData->m_data, &dumper );
				totalPayloadSize += dumper.GetPayloadSize();
				totalBlockCount += dumper.GetNumBlocks();
			}
		}

		// components
		for ( auto comp = entityData->m_componentsData; comp; comp = comp->m_next )
		{
			if ( comp->m_storedData != nullptr )
			{
				Char compGuidName[64];
				comp->m_guid.ToString( compGuidName, ARRAY_COUNT(compGuidName) );

				CDebugPageHTMLInfoBlock info( doc, "Component data (%s)", UNICODE_TO_ANSI(compGuidName) );

				{
					CDebugPageEntityStorageDumper dumper( doc );
					CGameSaveManager::DumpContent( comp->m_storedData, &dumper );
					totalPayloadSize += dumper.GetPayloadSize();
					totalBlockCount += dumper.GetNumBlocks();
				}
			}
		}

		// summary
		{
			CDebugPageHTMLInfoBlock info( doc, "Summary" );

			info.Info( "Blocks: %d", totalBlockCount );
			info.Info( "Total data: %d", entityData->GetDataSize() );
			info.Info( "Useful data: %d", totalPayloadSize );
			info.Info( "Fluff data: %d", entityData->GetDataSize() - totalPayloadSize );
		}

		// handled
		return true;
	}
};

//-----

void InitLayerStorageDebugPages()
{
	new CDebugPageLayerStorageList();
	new CDebugPageLayerStorage();
	new CDebugPageEntityStorage();
}

//-----
#endif
