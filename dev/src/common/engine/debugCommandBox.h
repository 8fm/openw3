
/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#include "debugCheckBox.h"
#include "inputBufferedInputEvent.h"

#ifndef NO_DEBUG_PAGES

class IDebugCommandBox : public IDebugCheckBox
{
protected:
	Float				m_timer;
	Float				m_timeMax;

public:
	IDebugCommandBox( IDebugCheckBox* parent, const String& name )
		: IDebugCheckBox( parent, name, false, false )
		, m_timer( 0.f )
		, m_timeMax( 1.f )
	{

	};

	virtual void OnTick( Float timeDelta )
	{
		IDebugCheckBox::OnTick( timeDelta );

		if ( m_timer > 0.f )
		{
			m_timer = Max( 0.f, m_timer - timeDelta );
		}
	}

	virtual void OnRender( CRenderFrame* frame, Uint32 x, Uint32 &y, Uint32 &counter, const RenderOptions& options );

	virtual Bool OnInput( enum EInputKey key, enum EInputAction action, Float data )
	{
		if ( action == IACT_Press && ( key == IK_Enter || key == IK_Pad_A_CROSS ) )
		{
			Process();
			m_timer = m_timeMax;
			return true;
		}

		return IDebugCheckBox::OnInput( key, action, data );
	}

	virtual void Process() = 0;

	virtual String GetComment() const { return TXT("Done"); }
};

#endif
