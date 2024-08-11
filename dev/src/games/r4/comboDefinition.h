
#pragma once

#include "comboAspect.h"

class CComboDefinition : public CObject
{
	DECLARE_ENGINE_CLASS( CComboDefinition, CObject, 0 );

	TDynArray< CComboAspect* >	m_aspects;

public:
	CComboAspect*	CreateComboAspect( const CName& aspectName );
	Bool			DeleteComboAspect( const CName& aspectName );
	CComboAspect*	FindComboAspect( const CName& aspectName ) const;

	void DebugDumpToLog();

private:
	void funcPlayAttack( CScriptStackFrame& stack, void* result );
	void funcCreateComboAspect( CScriptStackFrame& stack, void* result );
	void funcDeleteComboAspect( CScriptStackFrame& stack, void* result );
	void funcFindComboAspect( CScriptStackFrame& stack, void* result );
	void funcDebugDumpToLog( CScriptStackFrame& stack, void* result );

private:
	Int32 FindAspectIndex( const CName& aspectName ) const;
};

BEGIN_CLASS_RTTI( CComboDefinition );
	PARENT_CLASS( CObject );
	PROPERTY( m_aspects );
	NATIVE_FUNCTION( "CreateComboAspect", funcCreateComboAspect );
	NATIVE_FUNCTION( "DeleteComboAspect", funcDeleteComboAspect );
	NATIVE_FUNCTION( "FindComboAspect", funcFindComboAspect );
END_CLASS_RTTI();
