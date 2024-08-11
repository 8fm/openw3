/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#include "debugCheckBox.h"

#ifndef NO_DEBUG_PAGES

/// Slider
class IDebugSlider : public IDebugCheckBox
{
public:
	IDebugSlider( IDebugCheckBox* parent, const String& name );
	virtual ~IDebugSlider();

	virtual Float GetValue() const = 0;
	virtual Float GetMin() const = 0;
	virtual Float GetMax() const = 0;

public:
	//! Render
	virtual void OnRender( CRenderFrame* frame, Uint32 x, Uint32 &y, Uint32 &counter, const RenderOptions& options );

	//! Handle input
	virtual Bool OnInput( enum EInputKey key, enum EInputAction action, Float data );

protected:
	virtual void OnValueInc() = 0;
	virtual void OnValueDec() = 0;
};

#endif
