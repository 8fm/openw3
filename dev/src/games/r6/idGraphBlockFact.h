/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#pragma once

#include "idBasicBlocks.h"

//------------------------------------------------------------------------------------------------------------------
// Graph block
//------------------------------------------------------------------------------------------------------------------
class CIDGraphBlockFact : public CIDGraphBlock
{
	DECLARE_ENGINE_CLASS( CIDGraphBlockFact, CIDGraphBlock, 0 );

protected:
	String				m_factID;
	Int32				m_value;
	Int32				m_validFor;
	CIDGraphSocket*		m_output;

public:
	CIDGraphBlockFact();

	//! Get the name of the block
	virtual CName GetDefaultName() const { return CNAME( Fact ); }
	virtual String GetCaption() const;

	//! Rebuild block layout
	virtual void OnRebuildSockets();

	//! Get title bar color
	virtual Color GetTitleColor() const;

	virtual const CIDGraphBlock* Evaluate( CIDTopicInstance* topicInstance, Float& timeDelta ) const;

	//! Get block shape
	virtual EGraphBlockShape GetBlockShape() const;
};

BEGIN_CLASS_RTTI( CIDGraphBlockFact )
	PARENT_CLASS( CIDGraphBlock )
	PROPERTY_EDIT( m_factID, TXT("FactDB id.") )
	PROPERTY_EDIT( m_value, TXT("FactDB value.") )
	PROPERTY_EDIT_RANGE( m_validFor, TXT("How many seconds it will remain valid. -1 means: infinite."), -1, 10000000 ) // property ranges are stored as float so we can't use INT_MAX
	PROPERTY( m_output )
END_CLASS_RTTI()
