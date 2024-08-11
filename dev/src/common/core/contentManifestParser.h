/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "xmlReader.h"
#include "xmlWriter.h"
#include "contentManifest.h"

//////////////////////////////////////////////////////////////////////////
// CContentManifestReader
//////////////////////////////////////////////////////////////////////////
class CContentManifestReader
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Engine );

public:
	CContentManifestReader( const String& xmlData );
	~CContentManifestReader();

public:
	Bool ParseManifest( SContentManifest& outContentManifest );

private:
	Bool ParsePack( SContentPack& outContentPack );

private:
	CXMLReader m_xmlReader;
};

//////////////////////////////////////////////////////////////////////////
// CContentManifestWriter
//////////////////////////////////////////////////////////////////////////
class CContentManifestWriter
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Engine );

public:
	CContentManifestWriter( const SContentManifest& contentManifest );
	~CContentManifestWriter();

public://fixme parse one time
	Bool ParseManifest( String& outXml );

private:
	Bool ParsePack( const SContentPack& contentPack );

private:
	SContentManifest	m_contentManifest;
	CXMLWriter			m_xmlWriter;
};
