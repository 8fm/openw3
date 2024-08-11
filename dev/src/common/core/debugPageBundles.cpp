/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "filePath.h"
#include "depot.h"
#include "debugPageHandler.h"
#include "debugPageHTMLDoc.h"

#include "depotBundles.h"
#include "diskBundle.h"

#ifndef NO_DEBUG_PAGES

/////

namespace Helper
{
	// get compression name
	static const AnsiChar* GetCompressionName( Red::Core::Bundle::ECompressionType type )
	{
		switch ( type )
		{
		case Red::Core::Bundle::CT_Uncompressed: return "none";
		case Red::Core::Bundle::CT_Zlib: return "zlib";
		case Red::Core::Bundle::CT_Snappy: return "snappy";
		case Red::Core::Bundle::CT_Doboz: return "doboz";
		case Red::Core::Bundle::CT_LZ4: return "lz4";
		case Red::Core::Bundle::CT_LZ4HC: return "lz4hc";
		case Red::Core::Bundle::CT_ChainedZlib: return "chained";
		}

		return "Unknown";
	}
}

/////

class CDebugPageBundles : public IDebugPageHandlerHTML
{
public:
	CDebugPageBundles()
		: IDebugPageHandlerHTML( "/bundles/" )
	{}

private:
	// IDebugPageHandler interface
	virtual StringAnsi GetTitle() const override { return "Bundles"; }
	virtual StringAnsi GetCategory() const override { return "Core"; }
	virtual Bool HasIndexPage() const override { return GDepot->GetBundles() && GFileManager->IsReadOnly(); }

	// bundle info
	class BundleInfo : public CDebugPageHTMLTable::IRow
	{
	public:
		const CDiskBundle*	m_bundle;
		StringAnsi			m_mounted;
		Uint64				m_size;
		Uint32				m_numFiles;

		enum EColumn
		{
			eColumn_ID=1,
			eColumn_Name=2,
			eColumn_Mounted=3,
			eColumn_DataSize=4,
			eColumn_NumFiles=5,
		};

	public:
		BundleInfo( const CDiskBundle* bundle )
			: m_bundle( bundle  )
		{
			if ( GDepot->GetBundles()->IsBundleMounted( bundle->GetBundleID() ) )
				m_mounted = "Mounted";

			TDynArray< Red::Core::Bundle::SMetadataFileInBundlePlacement > allElems;
			GDepot->GetBundles()->GetBundleContentAll( bundle->GetBundleID(), allElems );

			m_numFiles = allElems.Size();

			for ( const auto& it : allElems )
				m_size += it.m_sizeInBundle;
		}

		virtual Bool OnCompare( const IRow* other, const Int32 columnID )
		{
			const CDiskBundle* a = m_bundle;
			const CDiskBundle* b = ((BundleInfo*)other)->m_bundle;

			if ( columnID == eColumn_Name )
			{
				const auto ca = a->GetFullPath();
				const auto cb = b->GetFullPath();
				return Red::StringCompare( ca.AsChar(), cb.AsChar() ) < 0;
			}
			else if ( columnID == eColumn_DataSize )
			{
				return m_size < ((BundleInfo*)other)->m_size;
			}
			else if ( columnID == eColumn_NumFiles )
			{
				return m_numFiles < ((BundleInfo*)other)->m_numFiles;
			}

			// default sorting by property offset
			return a->GetBundleID() < b->GetBundleID();
		}

		virtual void OnGenerateHTML( CDebugPageHTMLDocument& doc, const Int32 columnID )
		{
			if ( columnID == eColumn_ID )
			{
				doc.Writef( "%d", m_bundle->GetBundleID() );
			}
			else if ( columnID == eColumn_Name)
			{
				doc.Link( "/bundle/?id=%d", m_bundle->GetBundleID() ).Write( m_bundle->GetFullPath().AsChar() );
			}
			else if ( columnID == eColumn_NumFiles )
			{
				doc.Writef( "%d", m_numFiles );
			}
			else if ( columnID == eColumn_DataSize )
			{
				doc.Writef( "%1.3f MB", m_size / (1024.0f*1024.0f) );
			}
			else if ( columnID == eColumn_Mounted )
			{
				doc.Write( m_mounted.AsChar() );
			}
		}
	};

	//! Process command, commands are always passed via URLs, return NULL if not handled
	virtual bool OnFillPage( const class CBasicURL& fullURL, const class CBasicURL& relativeURL, class CDebugPageHTMLDocument& doc ) override
	{
		// get bundle information
		if ( !GDepot->GetBundles() )
		{
			doc << "<span class=\"error\">No bundles attached to the depot</span>";
			return true;
		}

		TDynArray< CDiskBundle* > bundles;
		GDepot->GetBundles()->GetAllBundles( bundles );

		// generic information
		{
			CDebugPageHTMLInfoBlock info( doc, "Generic information" );
			info.Info( "Bundle count: " ).Writef( "%d", bundles.Size() );

			Uint32 numMountedBundles = 0;
			for ( CDiskBundle* bundle : bundles )
			{
				if ( bundle && GDepot->GetBundles()->IsBundleMounted( bundle->GetBundleID() ) )
				{
					numMountedBundles += 1;
				}
			}
			info.Info( "Mounted bundles: " ).Writef( "%d", numMountedBundles );
		}

		// bundle table
		{
			CDebugPageHTMLInfoBlock info( doc, "Bundle list" );
			CDebugPageHTMLTable table( doc, "bundles" );

			// table definition
			table.AddColumn( "ID", 40, true );
			table.AddColumn( "Name", 150, true );
			table.AddColumn( "State", 70, true );
			table.AddColumn( "Size", 140, true );
			table.AddColumn( "NumFiles", 90, true);

			for ( CDiskBundle* bundle : bundles )
			{
				if ( bundle )
				{
					table.AddRow( new BundleInfo( bundle ) );
				}
			}

			// render the table
			table.Render( 500, "generic", fullURL );
		}

		// list of bundles
		return true;
	}
};

/////

class CDebugPageBundle : public IDebugPageHandlerHTML
{
public:
	CDebugPageBundle()
		: IDebugPageHandlerHTML( "/bundle/" )
	{}

private:
	// IDebugPageHandler interface
	virtual StringAnsi GetTitle() const override { return "Bundle"; }
	virtual StringAnsi GetCategory() const override { return "Core"; }
	virtual Bool HasIndexPage() const override { return false; }

	// file info
	class EntryInfo : public CDebugPageHTMLTable::IRow
	{
	public:
		Red::Core::Bundle::SMetadataFileInBundlePlacement	m_info;
		StringAnsi											m_name;
		Bool												m_depotFile;
		Float												m_ratio;
		Bool												m_unique;
		Bool												m_aviable;

		enum EColumn
		{
			eColumn_ID=1,
			eColumn_Offset=2,
			eColumn_Compression=3,
			eColumn_SizeInBundle=4,
			eColumn_SizeInMemory=5,
			eColumn_Ratio=6,
			eColumn_State=7,
			eColumn_Name=8,
		};

	public:
		EntryInfo( const Red::Core::Bundle::SMetadataFileInBundlePlacement& info )
			: m_info( info )
			, m_aviable( false )
		{
			auto file = GDepot->GetBundles()->GetBundleDiskFile( info.m_fileID );
			if ( file )
			{
				m_name = UNICODE_TO_ANSI( file->GetDepotPath().AsChar() );
				m_depotFile = true;
			}
			else
			{
				m_name = StringAnsi::Printf( "Buffer%d", info.m_fileID );
				m_depotFile = false;
			}

			TDynArray< Red::Core::Bundle::SMetadataFileInBundlePlacement > placements;
			GDepot->GetBundles()->GetFileEntries( info.m_fileID, placements );
			m_unique = (placements.Size() == 1);
			
			for ( const auto& it : placements )
			{
				if ( GDepot->GetBundles()->IsBundleMounted( it.m_bundleID ) )
				{
					m_aviable = true;
					break;
				}
			}

			if ( info.m_sizeInMemory )
				m_ratio = (Float)m_info.m_sizeInBundle / (Float)info.m_sizeInMemory;
			else
				m_ratio = 1.0f;
		}

		virtual Bool OnCompare( const IRow* other, const Int32 columnID ) override
		{
			const auto& a = m_info;
			const auto& b = ((EntryInfo*)other)->m_info;

			if ( columnID == eColumn_Offset )
			{
				return a.m_offsetInBundle < b.m_offsetInBundle;
			}
			else if ( columnID == eColumn_Compression )
			{
				return a.m_compression < b.m_compression;
			}
			else if ( columnID == eColumn_SizeInBundle )
			{
				return a.m_sizeInBundle < b.m_sizeInBundle;
			}
			else if ( columnID == eColumn_SizeInMemory )
			{
				return a.m_sizeInMemory < b.m_sizeInMemory;
			}
			else if ( columnID == eColumn_Name )
			{
				return m_name < ((EntryInfo*)other)->m_name;
			}
			else if ( columnID == eColumn_State )
			{
				const Uint8 af = (m_aviable ? 2 : 0) | (m_unique ? 1 : 0);
				const Uint8 bf = (((EntryInfo*)other)->m_aviable ? 2 : 0) | (((EntryInfo*)other)->m_unique ? 1 : 0);
				return af < bf;
			}
			else if ( columnID == eColumn_Ratio )
			{
				return m_ratio < ((EntryInfo*)other)->m_ratio;
			}

			return a.m_fileID < b.m_fileID;
		}

		virtual void OnGenerateHTML( CDebugPageHTMLDocument& doc, const Int32 columnID )
		{
			if ( columnID == eColumn_ID )
			{
				doc.Link( "/bundlefile/?id=%d", m_info.m_fileID ).Writef( "%d", m_info.m_fileID );
			}
			else if ( columnID == eColumn_Name)
			{
				if ( m_depotFile )
				{
					doc.Link( "/file/%hs", m_name.AsChar() ).Write( m_name.AsChar() );
				}
				else
				{
					doc.Write( m_name.AsChar() );
				}
			}
			else if ( columnID == eColumn_Compression )
			{
				doc.Write( Helper::GetCompressionName( m_info.m_compression ) );
			}
			else if ( columnID == eColumn_Offset )
			{
				doc.Writef( "%d", m_info.m_offsetInBundle );
			}
			else if ( columnID == eColumn_SizeInBundle )
			{
				doc.Writef( "%d", m_info.m_sizeInBundle );
			}
			else if ( columnID == eColumn_SizeInMemory )
			{
				doc.Writef( "%d", m_info.m_sizeInBundle );
			}
			else if ( columnID == eColumn_State )
			{
				if ( !m_aviable ) doc.Write( "<b>M</b> " );
				if ( m_unique ) doc.Write( "U " );
			}
			else if ( columnID == eColumn_Ratio )
			{
				doc.Writef( "%1.3f", m_ratio );
			}
		}
	};

	//! Process command, commands are always passed via URLs, return NULL if not handled
	virtual bool OnFillPage( const class CBasicURL& fullURL, const class CBasicURL& relativeURL, class CDebugPageHTMLDocument& doc ) override
	{
		// get bundle information
		if ( !GDepot->GetBundles() )
		{
			doc << "<span class=\"error\">No bundles attached to the depot</span>";
			return true;
		}

		// get object index
		Int32 bundleID = -1;
		relativeURL.GetKey( "id", bundleID );

		// find bundle
		CDiskBundle* bundle = GDepot->GetBundles()->GetBundle( bundleID );
		if ( !bundle )
		{
			doc << "<span class=\"error\">Invalid bundle ID specified</span>";
			return true;
		}

		// title
		doc.Open("p").Writef( "Bundle information" );

		// get bundle entries
		TDynArray< Red::Core::Bundle::SMetadataFileInBundlePlacement > entries;
		GDepot->GetBundles()->GetBundleContentAll( bundleID, entries );

		// generic information
		{
			CDebugPageHTMLInfoBlock info( doc, "Generic information" );

			info.Info( "Bundle path: " ).Write( bundle->GetFullPath().AsChar() );
			info.Info( "Number of entries: " ).Writef( "%d", entries.Size() );
		}

		// per compression stats
		{
			CDebugPageHTMLInfoBlock info( doc, "Compression statistics" );

			Uint32 fileCount[ Red::Core::Bundle::CT_Max ];
			Uint64 compressedSize[ Red::Core::Bundle::CT_Max ];
			Uint64 uncompressedSize[ Red::Core::Bundle::CT_Max ];
			Red::MemoryZero( fileCount, sizeof(fileCount) );
			Red::MemoryZero( compressedSize, sizeof(compressedSize) );
			Red::MemoryZero( uncompressedSize, sizeof(uncompressedSize) );

			for ( const auto& it : entries )
			{
				fileCount[ it.m_compression ] += 1;
				compressedSize[ it.m_compression ] += it.m_sizeInBundle;
				uncompressedSize[ it.m_compression ] += it.m_sizeInMemory;
			}

			for ( Uint32 i=0; i<Red::Core::Bundle::CT_Max; ++i )
			{
				if ( fileCount[i] )
				{
					info.Info( "Compression %hs: ", Helper::GetCompressionName( (Red::Core::Bundle::ECompressionType)i ) ).Writef( "%d files, %1.3f->%1.3f KB, ratio %1.3f",
						fileCount[i], compressedSize[i] / 1024.0f, uncompressedSize[i] / 1024.0f,
						(Double)compressedSize[i] / (Double)uncompressedSize[i] );
				}
			}
		}

		// entry table
		{
			CDebugPageHTMLInfoBlock info( doc, "Bundle file entries" );
			CDebugPageHTMLTable table( doc, "files" );

			// table definition
			table.AddColumn( "ID", 40, true );
			table.AddColumn( "Offset", 80, true );
			table.AddColumn( "Compression", 50, true);
			table.AddColumn( "DiskSize", 90, true );
			table.AddColumn( "MemSize", 90, true );
			table.AddColumn( "Ratio", 50, true );
			table.AddColumn( "Flags", 40, true );
			table.AddColumn( "Path", 1.0f, true );

			for ( const auto& it : entries )
			{
				table.AddRow( new EntryInfo( it ) );
			}

			// render the table
			table.Render( 1000, "generic", fullURL );
		}

		return true;
	}
};

/////

class CDebugPageBundleFile : public IDebugPageHandlerHTML
{
public:
	CDebugPageBundleFile()
		: IDebugPageHandlerHTML( "/bundlefile/" )
	{}

private:
	// IDebugPageHandler interface
	virtual StringAnsi GetTitle() const override { return "Bundle file"; }
	virtual StringAnsi GetCategory() const override { return "Core"; }
	virtual Bool HasIndexPage() const override { return false; }

	// buffer information
	class BufferInfo : public CDebugPageHTMLTable::IRow
	{
	public:
		Red::Core::Bundle::SMetadataFileInBundlePlacement	m_info;
		Uint32												m_id;

		enum EColumn
		{
			eColumn_ID=1,
			eColumn_FileID=2,
			eColumn_Compression=3,
			eColumn_SizeInBundle=4,
			eColumn_SizeInMemory=5,
		};

	public:
		BufferInfo( const Uint32 id, const Red::Core::Bundle::SMetadataFileInBundlePlacement& info )
			: m_info( info )
			, m_id( id )
		{
		}

		virtual Bool OnCompare( const IRow* other, const Int32 columnID ) override
		{
			const auto& a = m_info;
			const auto& b = ((BufferInfo*)other)->m_info;

			if ( columnID == eColumn_FileID )
			{
				return a.m_fileID < b.m_fileID;
			}
			else if ( columnID == eColumn_Compression )
			{
				return a.m_compression < b.m_compression;
			}
			else if ( columnID == eColumn_SizeInBundle )
			{
				return a.m_sizeInBundle < b.m_sizeInBundle;
			}
			else if ( columnID == eColumn_SizeInMemory )
			{
				return a.m_sizeInMemory < b.m_sizeInMemory;
			}

			return m_id < ((BufferInfo*)other)->m_id;
		}

		virtual void OnGenerateHTML( CDebugPageHTMLDocument& doc, const Int32 columnID )
		{
			if ( columnID == eColumn_ID )
			{
				doc.Writef( "%d", m_id );
			}
			else if ( columnID == eColumn_FileID )
			{
				doc.Link( "/bundlefile/?id=%d", m_info.m_fileID ).Writef( "%d", m_info.m_fileID );
			}
			else if ( columnID == eColumn_Compression )
			{
				doc.Write( Helper::GetCompressionName( m_info.m_compression ) );
			}
			else if ( columnID == eColumn_SizeInBundle )
			{
				doc.Writef( "%d", m_info.m_sizeInBundle );
			}
			else if ( columnID == eColumn_SizeInMemory )
			{
				doc.Writef( "%d", m_info.m_sizeInMemory );
			}
		}
	};

	//! Process command, commands are always passed via URLs, return NULL if not handled
	virtual bool OnFillPage( const class CBasicURL& fullURL, const class CBasicURL& relativeURL, class CDebugPageHTMLDocument& doc ) override
	{
		// get bundle information
		if ( !GDepot->GetBundles() )
		{
			doc << "<span class=\"error\">No bundles attached to the depot</span>";
			return true;
		}

		// get object index
		Int32 fileID = -1;
		if ( !relativeURL.GetKey( "id", fileID ) )
		{
			doc << "<span class=\"error\">No valid file ID specified</span>";
			return true;
		}

		// get placement information
		TDynArray< Red::Core::Bundle::SMetadataFileInBundlePlacement > placement;
		GDepot->GetBundles()->GetFileEntries( fileID, placement );
		if ( placement.Empty() )
		{
			doc << "<span class=\"error\">File with given ID is not present in bundles</span>";
			return true;
		}

		// info block
		{
			CDebugPageHTMLInfoBlock info( doc, "File information" );

			info.Info( "FileID: ").Writef( "%d", fileID );
			info.Info( "Compression: ").Write( Helper::GetCompressionName( placement[0].m_compression ) );
			info.Info( "Size on disk: ").Writef( " %1.2f KB", placement[0].m_sizeInBundle / 1024.0f );
			info.Info( "Size in memory: ").Writef( " %1.2f KB", placement[0].m_sizeInMemory / 1024.0f );

			// get depot file
			auto file = GDepot->GetBundles()->GetBundleDiskFile( fileID );
			if ( file )
			{
				const StringAnsi path( UNICODE_TO_ANSI( file->GetDepotPath().AsChar() ) );
				info.Info( "Depot file: ").Link( "/file/%hs", path.AsChar() ).Write( path.AsChar() );
			}
		}

		// placements
		for ( const Red::Core::Bundle::SMetadataFileInBundlePlacement& it : placement )
		{
			CDebugPageHTMLInfoBlock info( doc, "Placement information" );

			info.Info( "BundleID: ").Writef( "%d", it.m_bundleID );

			CDiskBundle* bundle = GDepot->GetBundles()->GetBundle( it.m_bundleID );
			if ( bundle != nullptr )
			{
				info.Info( "Bundle: ").Link( "/bundle/?id=%d", it.m_bundleID ).Writef( "%hs", bundle->GetFullPath().AsChar() );
			}

			info.Info( "State: ").Write( GDepot->GetBundles()->IsBundleMounted( it.m_bundleID ) ? "<b>Mounted</b>" : "Not mounted" );

			info.Info( "Offset in file: ").Writef( "%d", it.m_offsetInBundle );
		}

		// buffers
		if ( GDepot->GetBundles()->GetBufferFileID( fileID, 1 ) != 0 )
		{
			CDebugPageHTMLInfoBlock info( doc, "Buffers" );
			CDebugPageHTMLTable table( doc, "bundles" );

			// table definition
			table.AddColumn( "ID", 40, true );
			table.AddColumn( "FileID", 80, true );
			table.AddColumn( "Compression", 70, true );
			table.AddColumn( "Memory Size", 100, true );
			table.AddColumn( "Disk Size", 100, true );

			// total buffer size
			Uint32 totalBufferCompressedData = 0;
			Uint32 totalBufferUncompressedData = 0;
			Uint32 totalBuffers = 0;

			// extract buffer information
			{
				Uint32 bufferIndex = 1;
				for ( ;; )
				{
					const Uint32 bufferID = GDepot->GetBundles()->GetBufferFileID( fileID, bufferIndex );
					if ( !bufferID )
						break;

					TDynArray< Red::Core::Bundle::SMetadataFileInBundlePlacement > placement;
					GDepot->GetBundles()->GetFileEntries( bufferID, placement );

					if ( !placement.Empty() )
					{
						table.AddRow( new BufferInfo( bufferIndex, placement[0] ) );

						totalBufferCompressedData += placement[0].m_sizeInBundle;
						totalBufferUncompressedData += placement[0].m_sizeInMemory;
						totalBuffers += 1;
					}

					bufferIndex++;
				}
			}

			// add info blocks
			info.Info( "Number of buffers: " ).Writef( "%d", totalBuffers );
			info.Info( "Buffer compressed data: " ).Writef( "%1.3f KB", totalBufferCompressedData / 1024.0f );
			info.Info( "Buffer uncompressed data: " ).Writef( "%1.3f KB", totalBufferUncompressedData / 1024.0f );

			// render the table
			table.Render( 400, "generic", fullURL );
		}

		return true;
	}
};

/////

void InitBundlesDebugPages()
{
	new CDebugPageBundles(); // autoregister
	new CDebugPageBundle();  // autoregister
	new CDebugPageBundleFile(); // autoregister
}

#endif