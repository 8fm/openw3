/**
 * Copyright © 2011 CD Projekt Red. All Rights Reserved.
 */
#pragma once

class CQuestStateChangeRequestResetBlock : public CQuestGraphBlock
{
	DECLARE_ENGINE_CLASS( CQuestStateChangeRequestResetBlock, CQuestGraphBlock, 0 )

private:
	CName	m_entityTag;

public:

	CQuestStateChangeRequestResetBlock() { m_name = TXT("State Change Request Reset"); }

#ifndef NO_EDITOR_GRAPH_SUPPORT

	//! CGraphBlock interface
	virtual void OnRebuildSockets();
	virtual EGraphBlockShape GetBlockShape() const { return GBS_Default; }
	virtual Color GetClientColor() const { return Color( 155, 187, 89 ); }
	virtual String GetBlockCategory() const { return TXT( "Game systems control" ); }
	virtual Bool CanBeAddedToGraph( const CQuestGraph* graph ) const { return false; } // OBSOLETE

#endif

	// ------------------------------------------------------------------------
	// Block related action execution methods
	// ------------------------------------------------------------------------
	virtual void OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const;

};

BEGIN_CLASS_RTTI( CQuestStateChangeRequestResetBlock )
	PARENT_CLASS( CQuestGraphBlock )
	PROPERTY_EDIT( m_entityTag, TXT( "Tag for which the request is registered" ) )
END_CLASS_RTTI()
