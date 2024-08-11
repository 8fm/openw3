/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#include "debugCheckBox.h"

class CRenderFrame;

#ifndef NO_DEBUG_PAGES

class CDebugTextBox : public IDebugCheckBox
{
protected:
	String	m_text;
	Uint32	m_separation;
	Uint32	m_boxSize;

public:
	CDebugTextBox( IDebugCheckBox* parent, const String& name, const String& text = TXT("") )
		: IDebugCheckBox( parent, name, false, false )
		, m_text( text )
		, m_separation( 100 )
		, m_boxSize( 50 ) {};
	virtual ~CDebugTextBox() {};

	virtual void OnRender( CRenderFrame* frame, Uint32 x, Uint32 &y, Uint32 &counter, const RenderOptions& options );

	virtual Bool OnInput( enum EInputKey key, enum EInputAction action, Float data );

	RED_INLINE const String& GetText() { return m_text; }

	RED_INLINE void SetText( const String& text ) { m_text.Set( text.AsChar() ); }

	//! Sets the separation of the name and text box
	RED_INLINE void SetSeparation( Uint32 val ) { m_separation = val; }

	//! Gets the separation of the name and text box
	RED_INLINE Uint32 GetSeparation() const { return m_separation; }

	//! Sets the size of text box
	RED_INLINE void SetBoxSize( Uint32 val ) { m_boxSize = val; }

	//! Gets the size of text box
	RED_INLINE Uint32 GetBoxSize() const { return m_boxSize; }
};

#endif