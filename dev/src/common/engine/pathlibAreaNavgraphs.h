/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "pathlibAreaRes.h"


class CSimpleBufferWriter;
class CSimpleBufferReader;


namespace PathLib
{
	class CNavGraph;

class CAreaNavgraphs
{
public:
	static const Int16	RES_VERSION = 15;

	CAreaNavgraphs();
	~CAreaNavgraphs();

	CNavGraph*			GetGraph( Uint32 i ) const							{ return m_graph[ i ]; }

	void				ClearGraph( Uint32 i );
	void				ClearDetachedGraph( Uint32 i );
	CNavGraph*			NewGraph( Uint32 i, CAreaDescription* area );
	void				Clear();
	void				CompactData();

	void				operator=( CAreaNavgraphs&& n );

	Bool				Save( const String& depotPath ) const;
	Bool				ReadFromBuffer( CSimpleBufferReader& reader, CAreaDescription* area );
	void				WriteToBuffer( CSimpleBufferWriter& writer ) const;

	template < class Functor >
	void				IterateGraphs( Functor& f ) const
	{
		for( Uint32 i = 0; i < MAX_ACTOR_CATEGORIES; ++i )
		{
			if ( m_graph[ i ] )
			{
				f( m_graph[ i ] );
			}
		}
	}

protected:
	CNavGraph*			m_graph[ MAX_ACTOR_CATEGORIES ];
};

class CAreaNavgraphsRes : public CAreaRes, public CAreaNavgraphs
{
	friend class CAreaNavgraphsFactory;
public:
	CAreaNavgraphsRes()
		: CAreaNavgraphs()													{}
	~CAreaNavgraphsRes()													{}

	void				OnPreLoad( CAreaDescription* area  );
	void				OnPostLoad( CAreaDescription* area );

	Bool				VHasChanged() const override;
	Bool				VSave( const String& depotPath ) const override;
	void				VOnPreLoad( CAreaDescription* area ) override;
	Bool				VLoad( const String& depotPath, CAreaDescription* area ) override;
	void				VOnPostLoad( CAreaDescription* area ) override;
	const Char*			VGetFileExtension() const override;
	ENavResType			VGetResType() const override;

	static const Char*  GetFileExtension()							{ return TXT("navgraph"); }
	static ENavResType	GetResType()								{ return NavRes_Graph; }
};

};			// namespace PathLib