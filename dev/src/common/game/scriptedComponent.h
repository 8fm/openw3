#pragma once

class CScriptedComponent : public CComponent
{
	DECLARE_ENGINE_CLASS( CScriptedComponent, CComponent, 0 );

	struct SIntrestingEventEntry
	{
		CName	m_eventName;
		CName	m_eventToCallOnComponent;
		Bool	m_reportToScripts;
	};

	TDynArray< SIntrestingEventEntry >	m_interestingEvents;
	Bool								m_useUpdateTransform;

public:
	CScriptedComponent();

	virtual void OnAttached( CWorld* world ) override;
	virtual void OnAttachFinished( CWorld* world ) override;
	virtual void OnDetached( CWorld* world ) override;
	virtual bool UsesAutoUpdateTransform() override { return m_useUpdateTransform; }

protected:
	virtual void OnEventOccure( CName eventName, CObject* param ){}; 

public:
	RED_INLINE const TDynArray< SIntrestingEventEntry >& GetInterestingEvents(){ return m_interestingEvents; }

	void ProcessEvent( CName eventName, CObject* param );	
	void ListenToEvent( CName eventName, CName scriptFunctionToCall, Bool reportToScripts );

private:
	void funcListenToEvent( CScriptStackFrame& stack, void* result );
	void funcUseUpdateTransform( CScriptStackFrame& stack, void* result );
};

BEGIN_CLASS_RTTI( CScriptedComponent );
	PARENT_CLASS( CComponent );
	NATIVE_FUNCTION( "I_ListenToEvent", funcListenToEvent );
	NATIVE_FUNCTION( "UseUpdateTransform", funcUseUpdateTransform );
END_CLASS_RTTI();
