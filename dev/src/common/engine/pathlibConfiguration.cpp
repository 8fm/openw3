/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "../core/depot.h"

#include "pathlibConfiguration.h"

#include "pathlibAreaDescription.h"
#include "pathlibTerrainInfo.h"
#include "pathlibNavmeshArea.h"
#include "pathlibSimpleBuffers.h"
#include "pathlibWorld.h"
#include "pathlibWorldLayersMapping.h"
#include "pathlibHLGraph.h"
#include "pathlib.h"

namespace PathLib
{

//////////////////////////////////////////////////////////////////////////
// CPathLibConfiguration
//////////////////////////////////////////////////////////////////////////
CPathLibConfiguration::CPathLibConfiguration()
	: m_terrainInfoHash( 0xffffffff )
{
}

CPathLibConfiguration::~CPathLibConfiguration()
{
	for ( auto it = m_areas.Begin(), end = m_areas.End(); it != end; ++it )
	{
		delete *it;
	}

}
Bool CPathLibConfiguration::AddArea( CAreaDescription* area )
{
	if ( area->IsNavmeshArea() )
	{
		CNavmeshAreaDescription* naviArea = area->AsNavmeshArea();

		Bool isNaviValid;
		if( naviArea->IsLoaded() )
		{
			CNavmesh* navi = naviArea->GetNavmesh();
			isNaviValid = navi != nullptr;
		}
		else
		{
			Bool wasLoaded = true;
			CNavmeshResPtr& navmeshPtr = naviArea->GetNavmeshHandle();
			if ( !navmeshPtr.IsConstructed() )
			{
				if ( navmeshPtr.SyncLoad( naviArea ) )
				{
					wasLoaded = false;
				}
			}
			CNavmeshRes* navi = navmeshPtr.Get();
			isNaviValid = navi != nullptr;
			if ( !wasLoaded )
			{
				navmeshPtr.Release();
			}
		}
		if ( !isNaviValid )
		{
			return false;
		}
	}
	m_areas.PushBack( area );

	return true;
}
void CPathLibConfiguration::SetTerrainInfo( const CTerrainInfo& terrainInfo )
{
	m_terrainInfoHash = terrainInfo.ComputeHash();
}

void CPathLibConfiguration::SetWorldLayersInfo( const CWorldLayersMapping& worldLayers )
{
	m_worldLayers = worldLayers;
}

void CPathLibConfiguration::Clear()
{
	m_areas.ClearFast();
}

Bool CPathLibConfiguration::ValidateTerrainInfo( const CTerrainInfo& terrainInfo )
{
	return terrainInfo.ComputeHash() == m_terrainInfoHash;
}

void CPathLibConfiguration::WriteToBuffer( CSimpleBufferWriter& writer )
{
	writer.Put( m_terrainInfoHash );
	AreaId areasCount = AreaId( m_areas.Size() );
	writer.Put( areasCount );

	for ( AreaId i = 0; i < areasCount; ++i )
	{
		m_areas[ i ]->WriteToBuffer( writer );
	}
	m_worldLayers.WriteToBuffer( writer );
}
Bool CPathLibConfiguration::ReadFromBuffer( CSimpleBufferReader& reader )
{
	if ( !reader.Get( m_terrainInfoHash ) )
	{
		return false;
	}
	AreaId areasCount;
	if ( !reader.Get( areasCount ) )
	{
		return false;
	}

	m_areas.Resize( areasCount );
	Red::System::MemoryZero( m_areas.TypedData(), m_areas.DataSize() );

	for ( AreaId i = 0; i < areasCount; ++i )
	{
		if ( (m_areas[ i ] = CAreaDescription::NewFromBuffer( reader )) == nullptr )
		{
			return false;
		}
	}

	if ( !m_worldLayers.ReadFromBuffer( reader ) )
	{
		return false;
	}

	return true;
}

void CPathLibConfiguration::GetFileName( String& fileName )
{
	fileName = String::Printf( TXT("base.%") RED_PRIWs, GetFileExtension() );
}

Bool CPathLibConfiguration::Save( CPathLibWorld& pathlib )
{
	String fileName;
	GetFileName( fileName );
	CDiskFile* file = pathlib.GetFile4Save( fileName );
	if ( !file )
	{
		return false;
	}
	
	TDynArray< Int8 > buffer;
	CSimpleBufferWriter writer( buffer, RES_VERSION );
	WriteToBuffer( writer );
	IFile* fileWriter = GFileManager->CreateFileWriter( file->GetDepotPath() );
	if( !fileWriter )
	{
		return false;
	}
	fileWriter->Serialize( buffer.Data(), buffer.DataSize() );
	delete fileWriter;
	return true;
}

CPathLibConfiguration* CPathLibConfiguration::Load( CPathLibWorld& pathlib )
{
	String fileName;
	GetFileName( fileName );
	CDiskFile* diskFile = pathlib.GetFile4Load( fileName );
	if ( !diskFile )
	{
		return nullptr;
	}

	IFile* fileReader = diskFile->CreateReader();
	if( !fileReader )
	{
		return nullptr;
	}
	size_t fileSize = static_cast< size_t >( fileReader->GetSize() );
	PATHLIB_ASSERT( (Uint64)fileSize == fileReader->GetSize(), TXT("Unexpectedly large file '%ls'"), fileReader->GetFileNameForDebug() );
	TDynArray< Int8 > buffer;
	buffer.Resize( fileSize );
	fileReader->Serialize( buffer.Data(), fileSize );
	delete fileReader;

	CSimpleBufferReader reader( buffer );

	// version test
	if ( reader.GetVersion() != RES_VERSION )
	{
		return nullptr;
	}

	CPathLibConfiguration* configuration = new CPathLibConfiguration();
	Bool result = configuration->ReadFromBuffer( reader );

	if ( !result )
	{
		delete configuration;
		return nullptr;
	}
	return configuration;
}

};				// namespace PathLib