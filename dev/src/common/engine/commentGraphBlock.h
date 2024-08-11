/**
 * Copyright © 2012 CD Projekt Red. All Rights Reserved.
 */

#include "graphHelperBlock.h"

#pragma once

class CCommentGraphBlock : public CGraphHelperBlock
{
	DECLARE_ENGINE_CLASS( CCommentGraphBlock, CGraphHelperBlock, 0 );

	String		m_commentGraphBlockText;
	Color		m_titleColor;
	Vector		m_size;

public:
	CCommentGraphBlock() : m_titleColor(128,128,128) {}

	virtual String GetCaption() const { return m_commentGraphBlockText; }

	virtual String GetBlockName() const { return String( TXT("Comment block") ); }

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual String GetBlockCategory() const { return TXT("Helper tools"); }

	virtual bool IsResizable() const { return true; }

	virtual Bool ShouldCaptionBeInConstantSize() const { return true; }

	virtual EGraphBlockDepthGroup GetBlockDepthGroup() const { return GBDG_Comment; }

	virtual Bool IsMovingOverlayingBlocks() const { return true; }

	virtual Bool CanFreezeContent() const { return false; }

	virtual Bool IsInnerAreaTransparent() const { return true; }

	virtual Color GetTitleColor() const { return m_titleColor; }

	virtual const Vector GetSize() const { return m_size; }

	virtual void SetSize( Vector& newSize ) { m_size = newSize; }

	virtual Bool IsDraggedByClickOnInnerArea() const { return false; }
#endif
};

BEGIN_CLASS_RTTI( CCommentGraphBlock);
	PARENT_CLASS( CGraphHelperBlock );
	#ifndef NO_EDITOR_GRAPH_SUPPORT
	PROPERTY_NOT_COOKED( m_size )
	PROPERTY_EDIT_NOT_COOKED( m_commentGraphBlockText, TXT("Comment text") )
	PROPERTY_EDIT_NOT_COOKED( m_titleColor, TXT("Color of title background") )
	#endif
END_CLASS_RTTI();

