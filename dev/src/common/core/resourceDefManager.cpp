/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "resourceDefManager.h"
#include "profiler.h"
#include "directory.h"
#include "xmlReader.h"
#include "xmlWriter.h"
#include "depot.h"
#include "xmlFileReader.h"
#include "directory.h"
#include "filePath.h"
#include "dataError.h"

const String CResourceDefManager::DIRECTORY			= TXT("gameplay\\globals\\resources\\");
const String CResourceDefManager::RESDEF_PROTOCOL	= TXT("resdef:");

const String CResourceDefManager::NODE_RESOURCES	= TXT("resources");
const String CResourceDefManager::NODE_RES			= TXT("res");
const String CResourceDefManager::ATTR_ALIAS		= TXT("alias");
const String CResourceDefManager::ATTR_PATH			= TXT("path");

CResourceDefEntry::CResourceDefEntry( const Red::System::GUID& creatorTag, const String& alias, const String& path  ) : m_creatorTag( creatorTag ), m_alias( alias ), m_path( path )
{

}

CResourceDefManager::CResourceDefManager()
{
	m_creatorTag = Red::System::GUID::Create();
}

CResourceDefManager::~CResourceDefManager()
{

}

const String& CResourceDefManager::GetPath( const String& alias )
{
	if ( !DepotHelpers::ValidatePathString( alias.AsChar() ) )
		return String::EMPTY;

	const CResourceDefEntry* resourceDef = m_resourceMap.FindPtr( alias );
	if( resourceDef )
	{
		return resourceDef->GetPath();
	}
	else
	{
		ERR_CORE( TXT( "CResourceDefManager: alias '%ls' not found" ), alias.AsChar() );		
		return String::EMPTY;
	}
}

void CResourceDefManager::AddEntry( const String& alias, const String& path, const Red::System::GUID& creatorTag )
{
	if ( !DepotHelpers::ValidatePathString( alias.AsChar() ) )
		return;
	if ( !DepotHelpers::ValidatePathString( path.AsChar() ) )
		return;

	const CResourceDefEntry* resourceDef = m_resourceMap.FindPtr( alias );
	if( resourceDef )
	{
		ERR_CORE( TXT( "CResourceDefManager: alias '%ls' already exists with path '%ls'" ), alias.AsChar(), resourceDef->GetPath().AsChar() );		
	}
	else
	{
		m_resourceMap.Insert( alias, CResourceDefEntry( creatorTag, alias, path ) );
	}
}

void CResourceDefManager::RemoveEntry( const String& alias )
{
	TResourceMap::iterator resourceFound = m_resourceMap.Find( alias );
	if( resourceFound != m_resourceMap.End() )
	{
		m_resourceMap.Erase( resourceFound );
	}	
}
void CResourceDefManager::LoadAllDefinitions()
{
	CTimeCounter timer;

	m_resourceMap.Clear();

	// Find directory with definition files
	CDirectory* dir = GDepot->FindPath( DIRECTORY.AsChar() );
	if ( !dir )
	{
		ERR_CORE( TXT("CResourceDefManager::LoadDefinitions() - resource directory not found!") );
		return;
	}

	// iterate definition files and load item/ability data
	for ( CDiskFile* file : dir->GetFiles() )
	{
		const String path = file->GetDepotPath();
		if ( path.EndsWith( TXT(".xml") ) )
		{
			LoadDefinitions( path, m_creatorTag );
		}
	}

	LOG_CORE( TXT("CResourceDefManager loading %i definitions in %1.2fs "), dir->GetFiles().Size(), timer.GetTimePeriod() );
}

void CResourceDefManager::LoadDefinitions( const String& xmlFilePath, const Red::System::GUID& creatorTag )
{
	CLEAR_XML_ERRORS( xmlFilePath.AsChar() );

	IFile* file = NULL;
	CDiskFile *diskFile = GDepot->FindFile( xmlFilePath );

	if( diskFile )
	{
		file = diskFile->CreateReader();
	}

	if( file )
	{
		String alias;
		String path;		

		CXMLFileReader xmlReader( *file );
		if ( xmlReader.BeginNode( NODE_RESOURCES ) )
		{
			while( xmlReader.BeginNode( NODE_RES ) )
			{
				if ( xmlReader.Attribute( ATTR_ALIAS, alias ) )
				{
					if ( xmlReader.Attribute( ATTR_PATH, path ) )
					{
						String temp1, temp2;

						const String& safeAlias = CFilePath::ConformPath( alias, temp1 );
						const String& safePath = CFilePath::ConformPath( path, temp2 );
						AddEntry( safeAlias, safePath, creatorTag );
					}
					else
					{
						ReportParseErrorAttr( ATTR_PATH, NODE_RES, alias, xmlFilePath );
					}	
				}
				else
				{
					ReportParseErrorAttr( ATTR_ALIAS, NODE_RES, String::EMPTY, xmlFilePath );
				}

				xmlReader.EndNode(); // NODE_RES
			}

			xmlReader.EndNode(); // NODE_RESOURCES
		}
		else
		{
			ReportParseErrorNode( NODE_RESOURCES, String::EMPTY, xmlFilePath );
		}
	}
	else
	{
		ERR_CORE( TXT( "CResourceDefManager: cannot open '%ls'" ), xmlFilePath.AsChar() );
		XML_ERROR( xmlFilePath.AsChar(), TXT("ERROR CResourceDefManager: cannot open file") );
	}

	// Deletion after xmlReader destroyed
	if( file )
	{
		delete file;
		file = NULL;	
	}
}

void CResourceDefManager::RemoveDefinitions( const Red::System::GUID& creatorTag )
{ 	
	TDynArray<String> entriesToRemove;
	for ( TResourceMap::iterator current = m_resourceMap.Begin() ; current != m_resourceMap.End() ; ++current )
	{
		if( current->m_second.GetCreatorTag() == creatorTag )
		{
			entriesToRemove.PushBack( current->m_first );
		}
	}

	for ( TDynArray<String>::const_iterator current = entriesToRemove.Begin() ; current != entriesToRemove.End() ; ++current )
	{
		m_resourceMap.Erase( *current );
	}
}

void CResourceDefManager::ReportParseErrorNode( const String& nodeName, const String& context, const String& filepath ) const
{
	String err = String::Printf( TXT( "ERROR: CResourceDefManager - node parse error for node '%ls', context '%ls'" ), nodeName.AsChar(), context.AsChar() );
	ERR_CORE( err.AsChar() );
	XML_ERROR( filepath.AsChar(), err.AsChar() );
	RED_UNUSED(nodeName);
	RED_UNUSED(context);
}

void CResourceDefManager::ReportParseErrorValue( const String& nodeName, const String& context, const String& filepath ) const
{
	String err = String::Printf( TXT( "ERROR: CResourceDefManager - value parse error for node '%ls', context '%ls'" ), nodeName.AsChar(), context.AsChar() );
	ERR_CORE( err.AsChar() );
	XML_ERROR( filepath.AsChar(), err.AsChar() );
	RED_UNUSED(nodeName);
	RED_UNUSED(context);
}

void CResourceDefManager::ReportParseErrorAttr( const String& attrName, const String& nodeName, const String& context, const String& filepath ) const
{
	String err = String::Printf( TXT( "ERROR: CResourceDefManager - attribute parse error for node '%ls', attribute '%ls', context '%ls'" ), nodeName.AsChar(), attrName.AsChar(), context.AsChar() );
	ERR_CORE( err.AsChar() );
	XML_ERROR( filepath.AsChar(), err.AsChar() );
	RED_UNUSED(attrName);
	RED_UNUSED(nodeName);
	RED_UNUSED(context);
}

void CResourceDefManager::ReportErrorFileNotFound( const String& attrName, const String& nodeName, const String& context, const String& filepath ) const
{
	String err = String::Printf( TXT( "ERROR: CResourceDefManager - file not found for node '%ls', attribute '%ls', context '%ls'" ), nodeName.AsChar(), attrName.AsChar(), context.AsChar() );
	ERR_CORE( err.AsChar() );
	XML_ERROR( filepath.AsChar(), err.AsChar() );
	RED_UNUSED(attrName);
	RED_UNUSED(nodeName);
	RED_UNUSED(context);
}

