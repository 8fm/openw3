/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#pragma once

#include "idBasicBlocks.h"

enum EDialogDisplayMode;

//------------------------------------------------------------------------------------------------------------------
// Graph block
//------------------------------------------------------------------------------------------------------------------
class CIDGraphBlockComunicatorSwitch : public CIDGraphBlock
{
	DECLARE_ENGINE_CLASS( CIDGraphBlockComunicatorSwitch, CIDGraphBlock, 0 );

protected:
	EDialogDisplayMode	m_communicatorMode;
	CIDGraphSocket*		m_output;

public:
	CIDGraphBlockComunicatorSwitch();
	
	//! Get the name of the block
	virtual CName GetDefaultName() const { return CNAME( CommunicatorSwitch ); }
	virtual String GetCaption() const;

	//! Rebuild block layout
	virtual void OnRebuildSockets();

	//! Get title bar color
	virtual Color GetTitleColor() const;

	//! Get client color
	virtual Color GetClientColor() const;

	virtual const CIDGraphBlock* Evaluate( CIDTopicInstance* topicInstance, Float& timeDelta ) const;

	//! Get block shape
	virtual EGraphBlockShape GetBlockShape() const;
};

BEGIN_CLASS_RTTI( CIDGraphBlockComunicatorSwitch )
	PARENT_CLASS( CIDGraphBlock )
	PROPERTY_EDIT( m_communicatorMode, TXT("") )
	PROPERTY( m_output )
END_CLASS_RTTI()
