#pragma once


class CSelfUpdatingComponent : public CScriptedComponent
{
	DECLARE_ENGINE_CLASS( CSelfUpdatingComponent, CScriptedComponent, 0 );

private:
	Bool		m_tickedByDefault;
	ETickGroup	m_tickGroup;

protected:
	Bool		m_scriptNeedsTicking;
	Bool		m_tickedNow;

public:
	CSelfUpdatingComponent();

	virtual void OnAttached( CWorld* world );
	virtual void OnDetached( CWorld* world );

	void AddToTickGroups( CWorld* world );
	void RemoveFromTickGroups( CWorld* world );
	virtual void CustomTick( Float deltaTime );
	virtual void DecideIfItHasToTickScript();

public:
	void StartTicking();
	void StopTicking();
	Bool GetIsTicking()		{ return m_tickedNow; };

private:	
	void TickScript( Float timeDelta );

	void OnTick( Float timeDelta );
	void OnTickPrePhysics( Float timeDelta );
	void OnTickPrePhysicsPost( Float timeDelta );
	void OnTickPostPhysics( Float timeDelta );
	void OnTickPostPhysicsPost( Float timeDelta );
	void OnTickPostUpdateTransform( Float timeDelta );

	void funcStartTicking( CScriptStackFrame& stack, void* result );
	void funcStopTicking( CScriptStackFrame& stack, void* result );
	void funcGetIsTicking( CScriptStackFrame& stack, void* result );
};


BEGIN_CLASS_RTTI( CSelfUpdatingComponent );
	PARENT_CLASS( CScriptedComponent );

	PROPERTY_INLINED( m_tickGroup		, TXT("When is it ticked") );
	PROPERTY_INLINED( m_tickedByDefault	, TXT("Ticked from the start") );

	NATIVE_FUNCTION( "StartTicking"	, funcStartTicking);
	NATIVE_FUNCTION( "StopTicking"	, funcStopTicking);
	NATIVE_FUNCTION( "GetIsTicking"	, funcGetIsTicking);

END_CLASS_RTTI();