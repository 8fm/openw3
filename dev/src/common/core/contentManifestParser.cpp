/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "contentManifestParser.h"
#include "xmlReader.h"
#include "filePath.h"
#include "scopedPtr.h"
#include "file.h"

//////////////////////////////////////////////////////////////////////////
// SScopedReadNode
//////////////////////////////////////////////////////////////////////////
struct SScopedReadNode : Red::System::NonCopyable
{
	CXMLReader&	m_xml;
	Bool		m_found;

	SScopedReadNode( CXMLReader& xml, const String& name )
		: m_xml( xml )
	{
		m_found = m_xml.BeginNode( name );
	}

	SScopedReadNode( SScopedReadNode&& rhs )
		: m_xml( rhs.m_xml )
		, m_found( rhs.m_found )
	{
		rhs.m_found = false;	
	}

	~SScopedReadNode()
	{
		if ( m_found )
		{
			m_xml.EndNode();
		}
	}

	operator Bool const() { return m_found; }
};

//////////////////////////////////////////////////////////////////////////
// SScopedWriteNode
//////////////////////////////////////////////////////////////////////////
struct SScopedWriteNode : Red::System::NonCopyable
{
	CXMLWriter&	m_xml;
	Bool		m_found;

	SScopedWriteNode( CXMLWriter& xml, const String& name )
		: m_xml( xml )
	{
		m_found = m_xml.BeginNode( name );
	}

	SScopedWriteNode( SScopedWriteNode&& rhs )
		: m_xml( rhs.m_xml )
		, m_found( rhs.m_found )
	{
		rhs.m_found = false;	
	}

	~SScopedWriteNode()
	{
		if ( m_found )
		{
			m_xml.EndNode();
		}
	}

	operator Bool const() { return m_found; }
};

//////////////////////////////////////////////////////////////////////////
// Common defs
//////////////////////////////////////////////////////////////////////////
namespace
{
	const Char* const MANIFEST_NODE = TXT("manifest");
	const Char* const PACK_NODE = TXT("pack");
	const Char* const DLC_NODE = TXT("dlc");
	const Char* const CHUNK_NODE = TXT("chunk");
	const Char* const FILE_NODE = TXT("file");

	const Char* const TYPE_ATTR = TXT("type");
	const Char* const ID_ATTR = TXT("id");
	const Char* const DEPENDENCY_ATTR = TXT("dependency");
	const Char* const PRIORITY_ATTR = TXT("priority");
	const Char* const MIN_VERSION_ATTR = TXT("min_version");
	const Char* const PATCH_ATTR = TXT("patch");
	const Char* const SIZE_ATTR = TXT("size");
	const Char* const CRC_ATTR = TXT("crc");
	const Char* const PATH_ATTR = TXT("path");
}

//////////////////////////////////////////////////////////////////////////
// CContentManifestReader
//////////////////////////////////////////////////////////////////////////
CContentManifestReader::CContentManifestReader( const String& xmlData )
	: m_xmlReader( xmlData )
{
}

CContentManifestReader::~CContentManifestReader()
{
}

Bool CContentManifestReader::ParseManifest( SContentManifest& outContentManifest )
{
	if ( SScopedReadNode xmlMft = SScopedReadNode( m_xmlReader, MANIFEST_NODE ) )
	{
		if ( SScopedReadNode base = SScopedReadNode( m_xmlReader, PACK_NODE ) )
		{
			SContentPack contentPack;
			if ( ! ParsePack( contentPack ) )
			{
				return false;
			}
			outContentManifest.m_contentPack = Move( contentPack );
		}
	}

	return true;
}

Bool CContentManifestReader::ParsePack( SContentPack& outContentPack )
{
	SContentPack contentPack;

	String packID;
	if ( !m_xmlReader.AttributeT(ID_ATTR, packID ) )//contentPack.m_id ) )
	{
		ERR_CORE(TXT("CContentManifestReader: pack ID missing"));
		return false;
	}
	contentPack.m_id = CName( packID );

	String depID;
	if ( m_xmlReader.AttributeT(DEPENDENCY_ATTR, depID ) )//contentPack.m_dependency );
	{
		contentPack.m_dependency = CName( depID );
	}

	m_xmlReader.AttributeT(PRIORITY_ATTR, contentPack.m_priority );
	m_xmlReader.AttributeT(MIN_VERSION_ATTR, contentPack.m_minVersion );

	while ( SScopedReadNode xmlChunk = SScopedReadNode( m_xmlReader, CHUNK_NODE ) )
	{
		SContentChunk contentChunk;

		String chunkID;
		if ( !m_xmlReader.AttributeT(ID_ATTR, chunkID ) )//contentChunk.m_chunkLabel) )
		{
			ERR_CORE(TXT("CreateContentManifest: chunk ID missing"));
			return false;
		}
		contentChunk.m_chunkLabel = CName( chunkID );

		while ( SScopedReadNode file = SScopedReadNode( m_xmlReader, FILE_NODE ) )
		{
			SContentFile contentFile;

			m_xmlReader.AttributeT( PATCH_ATTR, contentFile.m_isPatch );
			m_xmlReader.AttributeT( SIZE_ATTR, contentFile.m_size );
			m_xmlReader.AttributeT( CRC_ATTR, contentFile.m_crc );
			if ( !m_xmlReader.AttributeT( PATH_ATTR, contentFile.m_path) )
			{
				ERR_CORE(TXT("CreateContentManifest: file path missing"));
				return false;
			}

			// Conform here since the XML may have been edited by hand externally (e.g., user created content)
			CFilePath::ConformPath( contentFile.m_path, contentFile.m_path );
			contentChunk.m_contentFiles.PushBack( Move( contentFile ) );
		}

		contentPack.m_contentChunks.PushBack( Move( contentChunk ) );
	}

	outContentPack = Move( contentPack );

	return true;
}

//////////////////////////////////////////////////////////////////////////
// CContentManifestWriter
//////////////////////////////////////////////////////////////////////////
CContentManifestWriter::CContentManifestWriter( const SContentManifest& contentManifest )
	: m_contentManifest( contentManifest )
{
}

CContentManifestWriter::~CContentManifestWriter()
{
}

Bool CContentManifestWriter::ParseManifest( String& outXml )
{
	if ( SScopedWriteNode manifest = SScopedWriteNode( m_xmlWriter, MANIFEST_NODE ) )
	{
		if ( SScopedWriteNode pack = SScopedWriteNode( m_xmlWriter, PACK_NODE ) )
		{
			if ( ! ParsePack( m_contentManifest.m_contentPack ) )
			{
				return false;
			}
		}
	}

	m_xmlWriter.Flush();
	outXml = m_xmlWriter.GetContent();

	return true;
}

Bool CContentManifestWriter::ParsePack( const SContentPack& contentPack  )
{
	if ( !contentPack.m_id )
	{
		ERR_CORE(TXT("CContentManifestWriter: pack ID missing"));
		return false;
	}
	const String id = contentPack.m_id.AsChar();
	if ( !m_xmlWriter.AttributeT(ID_ATTR, id ) )//contentPack.m_id) )
	{
		ERR_CORE(TXT("CContentManifestWriter: pack ID missing"));
		return false;
	}

	if ( contentPack.m_dependency )
	{
		const String dep = contentPack.m_dependency.AsChar();
		m_xmlWriter.AttributeT(DEPENDENCY_ATTR, dep );//contentPack.m_dependency );
	}

	m_xmlWriter.AttributeT(PRIORITY_ATTR, contentPack.m_priority );
	m_xmlWriter.AttributeT(MIN_VERSION_ATTR, contentPack.m_minVersion );

	for ( const SContentChunk& contentChunk : contentPack.m_contentChunks )
	{
		if ( SScopedWriteNode chunkNode = SScopedWriteNode( m_xmlWriter, CHUNK_NODE ) )
		{
			if ( ! contentChunk.m_chunkLabel )
			{
				ERR_CORE(TXT("CContentManifestWriter: chunk ID missing"));
				return false;
			}
			const String label = contentChunk.m_chunkLabel.AsChar(); // FIXME: CNames and Char* get saved as as blank from AttributeT
			if ( !m_xmlWriter.AttributeT(ID_ATTR, label ) )
			{
				ERR_CORE(TXT("CContentManifestWriter: chunk ID missing"));
				return false;
			}

			for ( const SContentFile& contentFile : contentChunk.m_contentFiles )
			{
				if ( SScopedWriteNode file = SScopedWriteNode( m_xmlWriter, FILE_NODE ) )
				{
					if ( !m_xmlWriter.AttributeT( PATH_ATTR, contentFile.m_path) )
					{
						ERR_CORE(TXT("CContentManifestWriter: file path missing"));
						return false;
					}
					
					if ( contentFile.m_isPatch )
					{
						m_xmlWriter.AttributeT( PATCH_ATTR, contentFile.m_isPatch );
					}

					if ( contentFile.m_size > 0 )
					{
						m_xmlWriter.AttributeT( SIZE_ATTR, contentFile.m_size );
					}
					if ( contentFile.m_crc > 0 )
					{
						m_xmlWriter.AttributeT( CRC_ATTR, contentFile.m_crc );
					}			
				}
			}
		}
	}

	return true;
}
