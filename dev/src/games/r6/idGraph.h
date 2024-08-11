/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#pragma once
#include "../../common/engine/graphContainer.h"
#include "../../common/engine/localizableObject.h"

// Graph
class CIDGraph : public CObject, public IGraphContainer, public ILocalizableObject
{
	DECLARE_ENGINE_CLASS( CIDGraph, CObject, 0 );

private:
	TDynArray< CGraphBlock* >	m_graphBlocks;			//!< Blocks in the graph
	Vector						m_backgroundOffset;		//!< Graph offset

public:
	CIDGraph();

public:
	// Graph interface
	virtual CObject *GraphGetOwner();
	virtual Vector GraphGetBackgroundOffset() const;
	virtual void GraphSetBackgroundOffset( const Vector& offset );

	// helper
	CIDGraphBlock* GetLastBlockByXPosition() const;

	virtual TDynArray< CGraphBlock* >& GraphGetBlocks() { return m_graphBlocks; }
	// const version
	RED_INLINE virtual const TDynArray< CGraphBlock* >& GraphGetBlocks() const { return m_graphBlocks; }

	// ILocalizableObject interface
	virtual void GetLocalizedStrings( TDynArray< LocalizedStringEntry >& localizedStrings ) /*const */;
};

BEGIN_CLASS_RTTI( CIDGraph )
	PARENT_CLASS( CObject )
	PROPERTY( m_graphBlocks )
END_CLASS_RTTI()

// Socket 
class CIDGraphSocket : public CGraphSocket
{
	DECLARE_RTTI_SIMPLE_CLASS( CIDGraphSocket );

public:
	//! Called to check if we can make connection between sockets
	virtual Bool CanConnectTo( CGraphSocket *otherSocket );

	//! Called when link on this socket is created
	virtual void OnConnectionCreated( CGraphConnection* connection );

public:
	//! Set the name of the socket
	RED_INLINE void SetName( const CName& name ) { m_name = name; }
};

BEGIN_CLASS_RTTI( CIDGraphSocket );
	PARENT_CLASS( CGraphSocket );
END_CLASS_RTTI();

// Spawn info 
class SIDGraphSocketSpawnInfo : public GraphSocketSpawnInfo
{
public:
	//! Constructor for output sockets ( links )
	SIDGraphSocketSpawnInfo( const CName& name,ELinkedSocketDirection direction )
		: GraphSocketSpawnInfo( ClassID< CIDGraphSocket >() )
	{
		m_name = name;
		m_direction = direction;
		m_placement = direction == LSD_Input ? LSP_Left : LSP_Right;
		m_isMultiLink = true;
	}
};