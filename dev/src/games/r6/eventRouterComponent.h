#pragma once



class CScriptedComponent;

//--------------------------------
//--------------------------------
struct SRouterEntry
{
	CName						m_eventName;
	TDynArray< CScriptedComponent* >	m_awareComponents;	

	~SRouterEntry();
};

//--------------------------------
//--------------------------------
class CEventRouterComponent : public CComponent
{
	DECLARE_ENGINE_CLASS( CEventRouterComponent, CComponent, 0 );

private:
	TDynArray< SRouterEntry >	m_entries;		

public:
	void OnAttachFinished( CWorld* world ) override;
	void RouteEvent( CName eventName );

	~CEventRouterComponent();

private:
	void funcRouteEvent( CScriptStackFrame& stack, void* result );
};
BEGIN_CLASS_RTTI( CEventRouterComponent );
	PARENT_CLASS( CComponent );
	NATIVE_FUNCTION( "I_RouteEvent", funcRouteEvent)
END_CLASS_RTTI();