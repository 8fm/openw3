/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

class CScriptedRenderFrame : public CObject
{
	DECLARE_ENGINE_CLASS( CScriptedRenderFrame, CObject, 0 );

public:

	CScriptedRenderFrame();
	CScriptedRenderFrame( CRenderFrame* frame );

	RED_FORCE_INLINE void Set( class CRenderFrame* frame )
	{
		m_frame = frame;
	}

private:

	class CRenderFrame*	m_frame;

	void funcDrawText( CScriptStackFrame& stack, void* result );
	void funcDrawSphere( CScriptStackFrame& stack, void* result );
	void funcDrawLine( CScriptStackFrame& stack, void* result );
};

BEGIN_CLASS_RTTI( CScriptedRenderFrame );
	PARENT_CLASS( CObject );
	NATIVE_FUNCTION( "DrawText", funcDrawText );
	NATIVE_FUNCTION( "DrawSphere", funcDrawSphere );
	NATIVE_FUNCTION( "DrawLine", funcDrawLine );
END_CLASS_RTTI();
