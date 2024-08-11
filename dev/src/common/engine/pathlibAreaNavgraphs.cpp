/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "pathlibAreaNavgraphs.h"

#include "pathlibAreaDescription.h"
#include "pathlibNavgraph.h"
#include "pathlibSimpleBuffers.h"

namespace PathLib
{

////////////////////////////////////////////////////////////////////////////
// CAreaNavgraphs
////////////////////////////////////////////////////////////////////////////
CAreaNavgraphs::CAreaNavgraphs()
{
	for( Uint32 i = 0; i < MAX_ACTOR_CATEGORIES; ++i )
	{
		m_graph[ i ] = NULL;
	}
}

CAreaNavgraphs::~CAreaNavgraphs()
{
	for( Uint32 i = 0; i < MAX_ACTOR_CATEGORIES; ++i )
	{
		if ( m_graph[ i ] )
		{
			delete m_graph[ i ];
		}
	}
}

void CAreaNavgraphs::ClearGraph( Uint32 i )
{
	ASSERT( i < MAX_ACTOR_CATEGORIES );
	if ( m_graph[ i ] )
	{
		m_graph[ i ]->DetachFromNeighbours();
		delete m_graph[ i ];
		m_graph[ i ] = NULL;
	}
}

void CAreaNavgraphs::ClearDetachedGraph( Uint32 i )
{
	ASSERT( i < MAX_ACTOR_CATEGORIES );
	if ( m_graph[ i ] )
	{
		delete m_graph[ i ];
		m_graph[ i ] = NULL;
	}
}

CNavGraph* CAreaNavgraphs::NewGraph( Uint32 i, CAreaDescription* area )
{
	ASSERT( i < MAX_ACTOR_CATEGORIES );
	ClearGraph( i );
	m_graph[ i ] = new CNavGraph( i, area );
	return m_graph[ i ];
}

void CAreaNavgraphs::Clear()
{
	for( Uint32 i = 0; i < MAX_ACTOR_CATEGORIES; ++i )
	{
		ClearGraph( i );
	}
}
void CAreaNavgraphs::CompactData()
{
	for ( Uint32 i = 0; i < PathLib::MAX_ACTOR_CATEGORIES; ++i )
	{
		if ( m_graph[ i ] )
		{
			m_graph[ i ]->CompactData();
		}
	}
}

void CAreaNavgraphs::operator=( CAreaNavgraphs&& n )
{
	for( Uint32 i = 0; i < MAX_ACTOR_CATEGORIES; ++i )
	{
		if ( n.m_graph[ i ] != m_graph[ i ] )
		{
			if ( m_graph[ i ] )
			{
				m_graph[ i ]->DetachFromNeighbours();
				delete m_graph[ i ];
			}
			m_graph[ i ] = n.m_graph[ i ];
		}
		n.m_graph[ i ] = NULL;
	}
}

Bool CAreaNavgraphs::ReadFromBuffer( CSimpleBufferReader& reader, CAreaDescription* area )
{
	if ( reader.GetVersion() != RES_VERSION )
	{
		return false;
	}

	CategoriesBitMask usedCategories;
	if ( !reader.Get( usedCategories ) )
	{
		return false;
	}
	//area->SetUsedCategories( usedCategories );
	for ( Uint16 i = 0; i < MAX_ACTOR_CATEGORIES; ++i )
	{
		if ( CAreaDescription::IsUsingCategory( i, usedCategories ) )
		{
			CNavGraph* naviGraph = new CNavGraph( i, area );
			if ( !naviGraph->ReadFromBuffer( reader ) )
			{
				delete naviGraph;
				return false;
			}

			m_graph[ i ] = naviGraph;
		}
	}

	return true;
}
void CAreaNavgraphs::WriteToBuffer( CSimpleBufferWriter& writer ) const
{
	CategoriesBitMask usedCategories = 0;
	for ( Uint32 i = 0; i < PathLib::MAX_ACTOR_CATEGORIES; ++i )
	{
		if ( m_graph[ i ] /*&& area->IsUsingCategory( i )*/ )
		{
			usedCategories |= 1 << i;
		}
	}

	writer.Put( usedCategories );

	for ( Uint32 i = 0; i < PathLib::MAX_ACTOR_CATEGORIES; ++i )
	{
		if ( CAreaDescription::IsUsingCategory( i, usedCategories ) )
		{
			m_graph[ i ]->WriteToBuffer( writer );
		}
	}
}

Bool CAreaNavgraphs::Save( const String& depotPath ) const
{
	TDynArray< Int8 > buffer;
	CSimpleBufferWriter writer( buffer, RES_VERSION );

	WriteToBuffer( writer );

	IFile* fileWriter = GFileManager->CreateFileWriter( depotPath );
	if( !fileWriter )
	{
		return false;
	}

	fileWriter->Serialize( buffer.Data(), buffer.DataSize() );
	delete fileWriter;

	return true;
}

////////////////////////////////////////////////////////////////////////////
// CAreaNavgraphsRes
////////////////////////////////////////////////////////////////////////////

void CAreaNavgraphsRes::OnPreLoad( CAreaDescription* area  )
{

}
void CAreaNavgraphsRes::OnPostLoad( CAreaDescription* area )
{
	for ( Uint32 i = 0; i < PathLib::MAX_ACTOR_CATEGORIES; ++i )
	{
		if ( m_graph[ i ]  )
		{
			m_graph[ i ]->OnPostLoad( area );
		}
	}
}
Bool CAreaNavgraphsRes::VHasChanged() const
{
	struct Functor 
	{
		Functor()
			: m_hasChanged( false ) {}
		void operator()( CNavGraph* nav )
		{
			m_hasChanged = m_hasChanged || !nav->IsInitialVersion();
		}
		Bool m_hasChanged;
	} f;
	IterateGraphs( f );
	return f.m_hasChanged;
}
Bool CAreaNavgraphsRes::VSave( const String& depotPath ) const
{
	return CAreaNavgraphs::Save( depotPath );
}
void CAreaNavgraphsRes::VOnPreLoad( CAreaDescription* area )
{
	return OnPreLoad( area );
}
Bool CAreaNavgraphsRes::VLoad( const String& depotPath, CAreaDescription* area )
{
	return Load( this, depotPath, area );
}
void CAreaNavgraphsRes::VOnPostLoad( CAreaDescription* area )
{
	return OnPostLoad( area );
}
const Char* CAreaNavgraphsRes::VGetFileExtension() const
{
	return GetFileExtension();
}
ENavResType CAreaNavgraphsRes::VGetResType() const
{
	return GetResType();
}


};			// namespace PathLib