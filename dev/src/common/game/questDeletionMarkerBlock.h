/**
 * Copyright © 2011 CD Projekt Red. All Rights Reserved.
 */
#pragma once

class CQuestDeletionMarkerBlock : public CQuestGraphBlock
{
	DECLARE_ENGINE_CLASS( CQuestDeletionMarkerBlock, CQuestGraphBlock, 0 )

private:
	TDynArray< CGUID >				m_guids;

public:

	CQuestDeletionMarkerBlock() { m_name = TXT("Deletion marker"); }

#ifndef NO_EDITOR_GRAPH_SUPPORT

	virtual void OnPasted( Bool wasCopied );

	//! CGraphBlock interface
	virtual void OnRebuildSockets();
	virtual EGraphBlockShape GetBlockShape() const { return GBS_Default; }
	virtual Color GetClientColor() const { return Color( 155, 155, 155 ); }
	virtual String GetBlockCategory() const { return TXT( "" ); }

	// this block can't be added to the graph by the user
	virtual Bool CanBeAddedToGraph( const CQuestGraph* graph ) const { return false; }

	// Adds a new block
	void AddBlock( const CQuestGraphBlock* block ); 

	// Adds a new connection
	void AddConnection( CGraphSocket* socket, CGraphConnection* connection ); 

#endif
	virtual void GetGUIDs( TDynArray< CGUID >& outGUIDs ) const;
	virtual Bool MatchesGUID( const CGUID& guid ) const;
	virtual void OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const;
};

BEGIN_CLASS_RTTI( CQuestDeletionMarkerBlock );
	PARENT_CLASS( CQuestGraphBlock );
	PROPERTY_RO( m_guids, TXT( "Replaced GUIDs" ) );
END_CLASS_RTTI();
