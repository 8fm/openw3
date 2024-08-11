/**
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */
#pragma once


class CQuestStartBlock : public CQuestGraphBlock
{
	DECLARE_ENGINE_CLASS( CQuestStartBlock, CQuestGraphBlock, 0 )

public:

	CQuestStartBlock() { m_name = TXT("Start"); }

	//! CObject interface
	virtual void OnSerialize( IFile& file ) { TBaseClass::OnSerialize( file ); }

	//! CGraphBlock interface
#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnRebuildSockets();
	virtual EGraphBlockShape GetBlockShape() const { return GBS_Arrow; }
	virtual Color GetClientColor() const { return Color( 192, 80, 77 ); }
	virtual String GetBlockCategory() const { return TXT( "Complexity management" ); }
	virtual Bool CanBeAddedToGraph( const CQuestGraph* graph ) const;
#endif

	// ------------------------------------------------------------------------
	// Block related action execution methods
	// ------------------------------------------------------------------------
	virtual void OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const;

};

BEGIN_CLASS_RTTI( CQuestStartBlock )
	PARENT_CLASS( CQuestGraphBlock )
END_CLASS_RTTI()
