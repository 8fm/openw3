/**
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */
#pragma once


class IQuestCondition;

class CQuestConditionBlock : public CQuestGraphBlock
{
	DECLARE_ENGINE_CLASS( CQuestConditionBlock, CQuestGraphBlock, 0 )

private:
	IQuestCondition*	m_questCondition;

public:
	CQuestConditionBlock() { m_name = TXT("Condition"); }

	//! CObject interface
	virtual void OnSerialize( IFile& file ) { TBaseClass::OnSerialize( file ); }

#ifndef NO_EDITOR_GRAPH_SUPPORT

	//! CGraphBlock interface
	virtual void OnRebuildSockets();
	virtual EGraphBlockShape GetBlockShape() const { return GBS_Triangle; }
	virtual Color GetClientColor() const { return Color( 128, 100, 162 ); }
	virtual String GetCaption() const { return m_name; }
	virtual String GetBlockCategory() const { return TXT( "Flow control" ); }
	virtual Bool CanBeAddedToGraph( const CQuestGraph* graph ) const { return true; }

#endif

	RED_INLINE IQuestCondition* GetCondition() { return m_questCondition; }

	// ------------------------------------------------------------------------
	// Block related action execution methods
	// ------------------------------------------------------------------------
	virtual void OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const;
};

BEGIN_CLASS_RTTI( CQuestConditionBlock )
	PARENT_CLASS( CQuestGraphBlock )
	PROPERTY_INLINED( m_questCondition, TXT( "Conditions that needs to be fulfilled" ) )
END_CLASS_RTTI()

