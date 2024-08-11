/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "binaryStorage.h"


///////////////////////////////////////////////////////////////////////////////

// A storage allowing to quickly query for nearby ( immobile ) nodes
class CNodesBinaryStorage : public CObject, public CQuadTreeStorage< CNode, THandle< CNode > >
{
	DECLARE_ENGINE_CLASS( CNodesBinaryStorage, CObject, 0 );
	using CObject::operator delete;

private:
	void funcInitializeFromTag( CScriptStackFrame& stack, void* result );
	void funcInitializeWithNodes( CScriptStackFrame& stack, void* result );
	void funcGetClosestToNode( CScriptStackFrame& stack, void* result );
	void funcGetClosestToPosition( CScriptStackFrame& stack, void* result );
};

BEGIN_CLASS_RTTI( CNodesBinaryStorage );
	PARENT_CLASS( CObject );
	NATIVE_FUNCTION( "InitializeFromTag", funcInitializeFromTag );
	NATIVE_FUNCTION( "InitializeWithNodes", funcInitializeWithNodes);
	NATIVE_FUNCTION( "GetClosestToNode", funcGetClosestToNode );
	NATIVE_FUNCTION( "GetClosestToPosition", funcGetClosestToPosition );
END_CLASS_RTTI();
///////////////////////////////////////////////////////////////////////////////
