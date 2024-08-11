/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#pragma once

class CQuestFactsDBChangingBlock : public CQuestGraphBlock
{
	DECLARE_ENGINE_CLASS( CQuestFactsDBChangingBlock, CQuestGraphBlock, 0 )

private:
	String		m_factID;
	Int32		m_value;
	Bool		m_setExactValue;


public:

	CQuestFactsDBChangingBlock() { m_name = TXT("FactsDB Change"); }

	//! CObject interface
	virtual void OnSerialize( IFile& file ) { TBaseClass::OnSerialize( file ); }

#ifndef NO_EDITOR_GRAPH_SUPPORT

	//! CGraphBlock interface
	virtual void OnRebuildSockets();
	virtual EGraphBlockShape GetBlockShape() const { return GBS_Slanted; }
	virtual Color GetClientColor() const { return Color( 192, 80, 77 ); }
	virtual String GetBlockCategory() const { return TXT( "Game systems control" ); }
	virtual Bool CanBeAddedToGraph( const CQuestGraph* graph ) const { return true; }

#endif

	// Returns the ID of the modified fact.
	RED_INLINE const String& GetFactId() const { return m_factID; }

	// ------------------------------------------------------------------------
	// Block related action execution methods
	// ------------------------------------------------------------------------
	virtual void OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const;

};

BEGIN_CLASS_RTTI( CQuestFactsDBChangingBlock )
	PARENT_CLASS( CQuestGraphBlock )
	PROPERTY_EDIT( m_factID, TXT( "ID of the fact to insert." ) )
	PROPERTY_EDIT( m_value, TXT( "Value we want to insert for that fact." ) )
	PROPERTY_EDIT( m_setExactValue, TXT( "Set exact value. If false, add value to the existing one." ) )
END_CLASS_RTTI()