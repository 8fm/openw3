
#include "expIntarface.h"

#pragma once

class CExplorationComponent : public CComponent, public IExploration
{
	DECLARE_ENGINE_CLASS( CExplorationComponent, CComponent, 0 );

	friend class CCookedExploration;

protected:
	EExplorationType	m_explorationId;

	Vector				m_start;
	Vector				m_end;

	String				m_componentForEvents;
	Bool				m_internalExploration;

public:
	CExplorationComponent();

	virtual void OnAttached( CWorld* world );
	virtual void OnDetached( CWorld* world );

	virtual void OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flags );

#ifndef NO_EDITOR
	virtual void OnNavigationCook( CWorld* world, CNavigationCookingContext* context ) override;
#endif

public: // IExploration
	virtual void GetMatWS( Matrix & mat ) const;
	virtual void GetParentMatWS( Matrix& mat ) const;
	virtual void GetEdgeWS( Vector& p1, Vector& p2 ) const;
	virtual void GetNormal( Vector& n ) const;
	virtual Int32 GetId() const;
	virtual CObject* GetObjectForEvents() const;
};

BEGIN_CLASS_RTTI( CExplorationComponent )
	PARENT_CLASS( CComponent );
	PROPERTY_EDIT( m_explorationId, TXT("") );
	PROPERTY_EDIT( m_start, TXT("Start") );
	PROPERTY_EDIT( m_end, TXT("End") );
	PROPERTY_EDIT( m_componentForEvents, TXT("'Empty' means entity will be object for exploration events") );
	PROPERTY_EDIT( m_internalExploration, TXT("If yes this exploration will not be added to exploration manager") );
END_CLASS_RTTI();
