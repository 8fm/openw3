/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "hashmap.h"
#include "string.h"
#include "object.h"

class CResourceDefEntry
{

public:
	CResourceDefEntry();
	CResourceDefEntry( const Red::System::GUID& creatorTag, const String& alias, const String& path );
	RED_FORCE_INLINE const String& GetAlias() const { return m_alias; }
	RED_FORCE_INLINE const String& GetPath() const { return m_path; }
	RED_FORCE_INLINE const Red::System::GUID& GetCreatorTag() const { return m_creatorTag; }

private:
	String m_alias;
	String m_path;
	Red::System::GUID m_creatorTag;
};

class CResourceDefManager
{
public:
	typedef THashMap< String, CResourceDefEntry >	TResourceMap;

private:
	Red::System::GUID   m_creatorTag;

	TResourceMap		m_resourceMap;	//!< Resource map (alias,path)

	static const String NODE_RESOURCES;
	static const String NODE_RES;
	static const String ATTR_ALIAS;
	static const String ATTR_PATH;

public:
	static const String DIRECTORY;
	static const String RESDEF_PROTOCOL;

	CResourceDefManager();
	~CResourceDefManager();

	//! Load all definitions
	void LoadAllDefinitions();

	//! Load single definition file
	void LoadDefinitions( const String& xmlFilePath, const Red::System::GUID& creatorTag );

	//! Remove entries from resources map with creatorTag set
	void RemoveDefinitions( const Red::System::GUID& creatorTag );

	//! Get path fro given alias
	const String& GetPath( const String& alias );

	//! Get resource map
	const TResourceMap& GetResourceMap() const { return m_resourceMap; }

private:
	
	//! Add entry to map
	void AddEntry( const String& alias, const String& path, const Red::System::GUID& creatorTag );

	//! Remove entry from map
	void RemoveEntry( const String& alias );

	//! Report node parse error
	void ReportParseErrorNode( const String& nodeName, const String& context, const String& filepath ) const;

	//! Report value parse error
	void ReportParseErrorValue( const String& nodeName, const String& context, const String& filepath ) const;

	//! Report attribute parse error
	void ReportParseErrorAttr( const String& attrName, const String& nodeName, const String& context, const String& filepath ) const;

	//! Report file not found error
	void ReportErrorFileNotFound( const String& attrName, const String& nodeName, const String& context, const String& filepath ) const;
};

/// Handle system singleton
typedef TSingleton< CResourceDefManager > SResourceDefManager;