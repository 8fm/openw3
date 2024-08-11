/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "debugSlider.h"
#include "inputKeys.h"
#include "inputBufferedInputEvent.h"
#include "renderFrame.h"

#ifndef NO_DEBUG_PAGES

IDebugSlider::IDebugSlider( IDebugCheckBox* parent, const String& name )
	: IDebugCheckBox( parent, name, false, false )
{
	
}

void IDebugSlider::OnRender( CRenderFrame* frame, Uint32 x, Uint32 &y, Uint32 &counter, const RenderOptions& options )
{
	//grab stats
	Float min = GetMin();
	Float max = GetMax();
	Float value = GetValue();
	Float range = max - min;
	Float percent = ( value - min ) / range;

	Bool isSelected = this == options.m_selected;

	{
		static Int32 width = 200;
		static Int32 height = 12;
		static Int32 offsetX = 150;
		static Int32 offsetY = -9;
		static Color green(0,120,0);
		static Color red(120,0,0);
		static Color gray(60,60,60);

		// Draw background bar
		Int32 barWidth = width;
		if ( isSelected ) frame->AddDebugRect( x+offsetX, y+offsetY, barWidth, height, red );

		// Draw background bar
		barWidth = Clamp< Int32 >( (Int32)( percent * width ), 0, width );
		frame->AddDebugRect( x+offsetX, y+offsetY, barWidth, height, isSelected ? green : gray );

		// Draw frame
		frame->AddDebugFrame( x+offsetX, y+offsetY, width, height, gray );

		// Draw text
		frame->AddDebugScreenText( x+offsetX+60, y, String::Printf(TXT("(%.1f)    (%1.2f)   (%.1f)"), min, value, max ), isSelected ? Color::YELLOW : Color::WHITE );
	}

	IDebugCheckBox::OnRender( frame, x, y, counter, options );
}

Bool IDebugSlider::OnInput( enum EInputKey key, enum EInputAction action, Float data )
{
	// Navigate
	if ( ( key == IK_Right || key == IK_Pad_DigitRight ) && action == IACT_Press )
	{
		OnValueInc();
		return true;
	}
	else if ( ( key == IK_Left || key == IK_Pad_DigitLeft ) && action == IACT_Press )
	{
		OnValueDec();
		return true;
	}

	if ( IDebugCheckBox::OnInput( key, action, data ) )
	{
		return true;
	}

	// Not handled
	return false;
}

IDebugSlider::~IDebugSlider()
{
	
}

#endif
