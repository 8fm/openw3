/**
* Copyright ©2010 CD Projekt Red. All Rights Reserved.
*/

#pragma once

///////////////////////////////////////////////////////////////////////////////

class IPerformableAction : public CObject
{
	DECLARE_ENGINE_ABSTRACT_CLASS( IPerformableAction, CObject );
public:
	static void PerformAll( const TDynArray< IPerformableAction* >& actionsToPerform, CEntity* parent );

	virtual ~IPerformableAction() {}

	virtual void Perform( CEntity* parent );
	virtual void Perform( CEntity* parent, CNode* node );
	virtual void Perform( CEntity* parent, float value );

private:
	void funcTrigger( CScriptStackFrame& stack, void* result );
	void funcTriggerArgNode( CScriptStackFrame& stack, void* result );
	void funcTriggerArgFloat( CScriptStackFrame& stack, void* result );
};

BEGIN_ABSTRACT_CLASS_RTTI( IPerformableAction );
	PARENT_CLASS( CObject );
	NATIVE_FUNCTION( "Trigger",			funcTrigger );
	NATIVE_FUNCTION( "TriggerArgNode",	funcTriggerArgNode );
	NATIVE_FUNCTION( "TriggerArgFloat",	funcTriggerArgFloat );
END_CLASS_RTTI();

///////////////////////////////////////////////////////////////////////////////

class CScriptedAction : public IPerformableAction
{
	DECLARE_ENGINE_CLASS( CScriptedAction, IPerformableAction, 0 );

public:

	virtual void Perform( CEntity* parent ) override;
	virtual void Perform( CEntity* parent, CNode* node ) override;
	virtual void Perform( CEntity* parent, float value ) override;

};
BEGIN_CLASS_RTTI( CScriptedAction );
	PARENT_CLASS( IPerformableAction );
END_CLASS_RTTI();

///////////////////////////////////////////////////////////////////////////////
