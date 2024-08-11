/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "scriptedRenderFrame.h"
#include "../../common/engine/renderFrame.h"

IMPLEMENT_ENGINE_CLASS( CScriptedRenderFrame );

CScriptedRenderFrame::CScriptedRenderFrame()
	: m_frame( nullptr )
{}

CScriptedRenderFrame::CScriptedRenderFrame( CRenderFrame* frame )
	: m_frame( frame )
{}

void CScriptedRenderFrame::funcDrawText( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String, text, String::EMPTY );
	GET_PARAMETER( Vector, position, Vector::ZEROS );
	GET_PARAMETER( Color, color, Color::WHITE );
	FINISH_PARAMETERS;

	if ( m_frame != nullptr )
	{
		m_frame->AddDebugText( position, text, 0, 0, true, color );
	}

	RETURN_VOID();
}

void CScriptedRenderFrame::funcDrawSphere( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, position, Vector::ZEROS );
	GET_PARAMETER( Float, radius, 0.0f );
	GET_PARAMETER( Color, color, Color::WHITE );
	FINISH_PARAMETERS;

	if ( m_frame != nullptr )
	{
		m_frame->AddDebugSphere( position, radius, Matrix::IDENTITY, color );
	}

	RETURN_VOID();
}

void CScriptedRenderFrame::funcDrawLine( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, start, Vector::ZEROS );
	GET_PARAMETER( Vector, end, Vector::ZEROS );
	GET_PARAMETER( Color, color, Color::WHITE );
	FINISH_PARAMETERS;

	if ( m_frame != nullptr )
	{
		m_frame->AddDebugLine( start, end, color );
	}

	RETURN_VOID();
}