/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "graphHelperBlock.h"

class CDescriptionGraphBlock : public CGraphHelperBlock
{
	DECLARE_ENGINE_CLASS( CDescriptionGraphBlock, CGraphHelperBlock, 0 );

	String		m_caption;
	String		m_descriptionText;
	Vector		m_size;

public:
	CDescriptionGraphBlock() 
		: m_caption( TXT("Note") ) 
		, m_size( 130, 80, 0 )
	{}

	virtual String GetCaption() const { return m_caption; }

	virtual String GetBlockName() const { return String( TXT("Description note") ); }

	virtual String GetDescriptionText() const { return m_descriptionText; }

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual String GetBlockCategory() const { return TXT("Helper tools"); }

	virtual bool IsResizable() const { return true; }

	virtual Bool ShouldCaptionBeInConstantSize() const { return false; }

	virtual EGraphBlockDepthGroup GetBlockDepthGroup() const { return GBDG_Background; }

	virtual Bool IsMovingOverlayingBlocks() const { return false; }

	virtual Bool CanFreezeContent() const { return false; }

	virtual Bool IsInnerAreaTransparent() const { return false; }

	virtual Color GetTitleColor() const { return Color(70,128,70); }

	virtual const Vector GetSize() const { return m_size; }

	virtual void SetSize( Vector& newSize ) { m_size = newSize; }

	virtual Bool IsDraggedByClickOnInnerArea() const { return true; }
#endif
};

BEGIN_CLASS_RTTI( CDescriptionGraphBlock );
	PARENT_CLASS( CGraphHelperBlock );
	PROPERTY( m_size )
	PROPERTY_EDIT( m_caption, TXT("Caption text") )
	PROPERTY_EDIT( m_descriptionText, TXT("Description text") )
END_CLASS_RTTI( );
