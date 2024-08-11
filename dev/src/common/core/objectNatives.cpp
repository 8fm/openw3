/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "object.h"
#include "scriptStackFrame.h"

void CObject::funcGetParent( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_OBJECT( GetParent() );
}

void CObject::funcClone( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle<CObject>, parent, NULL );
	FINISH_PARAMETERS;
	
	CObject* newObj = Clone( parent.Get() );
	newObj->SetFlag( OF_ScriptCreated );

	RETURN_OBJECT( newObj );
}

void CObject::funcIsIn( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< CObject >, object, NULL );
	FINISH_PARAMETERS;
	RETURN_BOOL( IsContained( object.Get() ) );
}
