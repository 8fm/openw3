/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#pragma once

#include "idBasicBlocks.h"

//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
enum EIDFocusMode
{
	IDFM_GetFocus		= 0	,
	IDFM_LoseFocus			,
	IDFM_RecalcFocus		,
};
BEGIN_ENUM_RTTI( EIDFocusMode )
	ENUM_OPTION( IDFM_GetFocus )
	ENUM_OPTION( IDFM_LoseFocus )
	ENUM_OPTION( IDFM_RecalcFocus )
END_ENUM_RTTI()


//------------------------------------------------------------------------------------------------------------------
// Block that allows to get the HUD focus, lose it or recalculate who should have it
//------------------------------------------------------------------------------------------------------------------
class CIDGraphBlockRequestFocus : public CIDGraphBlock
{
	DECLARE_ENGINE_CLASS( CIDGraphBlockRequestFocus, CIDGraphBlock, 0 )

private:
	EIDFocusMode		m_focus;

public:
	CIDGraphBlockRequestFocus() {};

	virtual void OnRebuildSockets();

	//! Get the name of the block
	virtual CName GetDefaultName() const { return CNAME( RequestFocus ); }
	virtual String GetCaption() const;

	//! Get title bar color
	virtual Color GetTitleColor() const;

	//! Get client color
	virtual Color GetClientColor() const;

	//! Get block shape
	virtual EGraphBlockShape GetBlockShape() const;

	virtual const CIDGraphBlock* Evaluate( CIDTopicInstance* topicInstance, Float& timeDelta ) const;
};

BEGIN_CLASS_RTTI( CIDGraphBlockRequestFocus );
	PARENT_CLASS( CIDGraphBlock );
	PROPERTY_EDIT( m_focus, TXT("Get, lose or recalc focus") );
END_CLASS_RTTI();
