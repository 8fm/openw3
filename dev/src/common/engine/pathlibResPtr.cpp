/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "pathlibResPtr.h"

#include "../core/directory.h"

#include "pathlibAreaDescription.h"
#include "pathlibAreaNavgraphs.h"
#include "pathlibAreaRes.h"
#include "pathlibObstaclesMap.h"
#include "pathlibNavmesh.h"
#include "pathlibSimpleBuffers.h"
#include "pathlibTerrain.h"




namespace PathLib
{
///////////////////////////////////////////////////////////////////////////////
// CResPtr
///////////////////////////////////////////////////////////////////////////////
CResPtr::~CResPtr()
{
	if ( m_res )
	{
		delete m_res;
	}
}

void CResPtr::Release()
{
	if ( m_res )
	{
		delete m_res;
		m_res = nullptr;
	}
}


void CResPtr::Feed( CAreaRes* res )
{
	PATHLIB_ASSERT( !m_res && VGetResType() == res->VGetResType() );
	m_res = res;
}

Bool CResPtr::Save( CAreaDescription* area, CDirectory* dir )
{
	CAreaRes* res = GetRes();
	if( !res )
	{
		return false;
	}

	// get file depo path
	String fileName;
	VGetResName( area, fileName );
	String depotPath;
	dir->GetDepotPath( depotPath );
	depotPath += fileName;

	if ( res->VSave( depotPath ) )
	{
		// TODO: This is hack that attaches newly created file into our file system - that is currently required by new streaming system
		if ( !dir->FindLocalFile( fileName.AsChar() ) )
		{
			CDiskFile* newFile = new CDiskFile( dir, fileName );
			dir->AddFile( newFile );
		}
		return true;
	}
	return false;
}


Bool CResPtr::SyncLoad( CAreaDescription* area )
{
	CAreaRes* res = GetRes();
	if( res )
	{
		return true;
	}
	String fileName;
	VGetResName( area, fileName );
	CPathLibWorld& pathlib = area->GetPathLib();
	CDiskFile* file = pathlib.GetFile4Load( fileName );
	if ( !file )
	{
		return false;
	}
	if ( file->GetDirectory() != pathlib.GetCookedDataDirectory() )
	{
		area->MarkNotFullyCooked();
	}

	res = VConstruct();
	res->VOnPreLoad( area );
	if ( !res->VLoad( file->GetDepotPath(), area ) )
	{
		Release();
		return false;
	}
	return true;
}

template < class ResType >
void TResPtr< ResType >::GetLocalResName( CAreaDescription* area, String& outName )
{
	area->GetPathLib().GetGenericFileName( area->GetId(), outName, ResType::GetFileExtension() );
}


///////////////////////////////////////////////////////////////////////////////
// CObstaclesMapResPtr
///////////////////////////////////////////////////////////////////////////////
CAreaRes* CObstaclesMapResPtr::VConstruct()
{
	return Construct();
}
ENavResType CObstaclesMapResPtr::VGetResType() const
{
	return GetResType();
}
void CObstaclesMapResPtr::VGetResName( CAreaDescription* area, String& outName )
{
	GetLocalResName( area, outName );
}

///////////////////////////////////////////////////////////////////////////////
// CAreaNavgraphsResPtr
///////////////////////////////////////////////////////////////////////////////
CAreaRes* CAreaNavgraphsResPtr::VConstruct()
{
	return Construct();
}
ENavResType CAreaNavgraphsResPtr::VGetResType() const
{
	return GetResType();
}
void CAreaNavgraphsResPtr::VGetResName( CAreaDescription* area, String& outName )
{
	GetLocalResName( area, outName );
}


///////////////////////////////////////////////////////////////////////////////
// CTerrainMapResPtr
///////////////////////////////////////////////////////////////////////////////
CAreaRes* CTerrainMapResPtr::VConstruct()
{
	return Construct();
}
ENavResType CTerrainMapResPtr::VGetResType() const
{
	return GetResType();
}
void CTerrainMapResPtr::VGetResName( CAreaDescription* area, String& outName )
{
	GetLocalResName( area, outName );
}

///////////////////////////////////////////////////////////////////////////////
// CNavmeshResPtr
///////////////////////////////////////////////////////////////////////////////
CAreaRes* CNavmeshResPtr::VConstruct()
{
	return Construct();
}
ENavResType CNavmeshResPtr::VGetResType() const
{
	return GetResType();
}
void CNavmeshResPtr::VGetResName( CAreaDescription* area, String& outName )
{
	GetLocalResName( area, outName );
}

};				// namespace PathLib