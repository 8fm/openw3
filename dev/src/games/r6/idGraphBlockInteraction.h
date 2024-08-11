/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#pragma once

#include "idBasicBlocks.h"


//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
enum EIDBlockInteractionMode
{
	IDBIM_StartInteraction		= 0	,
	IDBIM_StopInteraction			,
	IDBIM_EnableInteraction			,
	IDBIM_DisableInteraction		,
};
BEGIN_ENUM_RTTI( EIDBlockInteractionMode )
	ENUM_OPTION( IDBIM_StartInteraction )
	ENUM_OPTION( IDBIM_StopInteraction )
	ENUM_OPTION( IDBIM_EnableInteraction )
	ENUM_OPTION( IDBIM_DisableInteraction )
END_ENUM_RTTI()

//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
class CIDGraphBlockInteraction : public CIDGraphBlock
{
	DECLARE_ENGINE_CLASS( CIDGraphBlockInteraction, CIDGraphBlock, 0 );

protected:
	EIDBlockInteractionMode		m_mode;
	EntityHandle				m_entity;
	CName						m_interactionComponent;
	CIDGraphSocket*				m_output;

public:
	CIDGraphBlockInteraction();
	
	//! Get the name of the block
	virtual CName GetDefaultName() const { return CNAME( Interaction ); }
	virtual String GetCaption()		const;

	//! Rebuild block layout
	virtual void OnRebuildSockets();

	//! Get title bar color
	virtual Color GetTitleColor()	const;

	//! Get client color
	virtual Color GetClientColor()	const;

	virtual const CIDGraphBlock* Evaluate( CIDTopicInstance* topicInstance, Float& timeDelta )	const;

	//! Get block shape
	virtual EGraphBlockShape GetBlockShape()	const;

private:
	String const	GetInteractionDisplayText	( )		const;
	void const		StartInteraction			( )		const;
	void const		StopInteraction				( )		const;
	void const		SetInteractionEnabled		( Bool enabled )	const;
};

BEGIN_CLASS_RTTI( CIDGraphBlockInteraction )
	PARENT_CLASS( CIDGraphBlock )
	PROPERTY_EDIT( m_mode, TXT("") )
	PROPERTY_EDIT( m_entity, TXT("") )
	PROPERTY_EDIT( m_interactionComponent, TXT("") )
	PROPERTY( m_output )
END_CLASS_RTTI()
