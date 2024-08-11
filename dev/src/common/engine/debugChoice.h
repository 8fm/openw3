/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#include "debugCheckBox.h"

#ifndef NO_DEBUG_PAGES

/// Choice
class IDebugChoice : public IDebugCheckBox
{
public:
	IDebugChoice( IDebugCheckBox* parent, const String& name );
	virtual ~IDebugChoice();

public:
	//! Render
	virtual void OnRender( CRenderFrame* frame, Uint32 x, Uint32 &y, Uint32 &counter, const RenderOptions& options );

	//! Handle input
	virtual Bool OnInput( enum EInputKey key, enum EInputAction action, Float data );

	virtual String GetSelection() const = 0;

	//! Sets the separation of the name and selection
	RED_INLINE void SetSeparation( Uint32 val ) { m_separation = val; }

	//! Gets the separation of the name and selection
	RED_INLINE Uint32 GetSeparation() const { return m_separation; }

protected:
	virtual void OnNext() = 0;
	virtual void OnPrev() = 0;

	Uint32 m_separation;
};

class CDebugStringChoice : public IDebugChoice
{
	Uint32							m_index;
	TDynArray< String, MC_Debug >	m_list;

public:
	CDebugStringChoice( IDebugCheckBox* parent, const String& name, const TDynArray< String, MC_Debug >& list );

	Bool SetSelection( const String& str );
	virtual String GetSelection() const;

protected:
	virtual void OnNext();
	virtual void OnPrev();
};

#endif
