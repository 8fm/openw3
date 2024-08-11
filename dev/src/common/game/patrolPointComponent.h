#pragma once

#include "wanderPointComponent.h"

class CPatrolPointComponent : public CWanderPointComponent
{
	DECLARE_ENGINE_CLASS( CPatrolPointComponent, CWanderPointComponent, 0 )

public:
	CPatrolPointComponent()
		: CWanderPointComponent()
	{ 
		InitializeMarkersPP(); 
	}

	void ChangeLinks( Bool generate, Bool twoSideRemoval ) override {}
protected:

	static IRenderResource*		m_markerValidPP;
	static IRenderResource*		m_markerInvalidPP;
	static IRenderResource*		m_markerNoMeshPP; 
	static IRenderResource*		m_markerSelectionPP;

	void InitializeMarkersPP();

	virtual IRenderResource* GetMarkerValid(){ return m_markerValidPP; }
	virtual IRenderResource* GetMarkerInvalid(){ return m_markerInvalidPP; }
	virtual IRenderResource* GetMarkerNoMesh(){ return m_markerNoMeshPP; }
	virtual IRenderResource* GetMarkerSelection(){ return m_markerSelectionPP; }
};

BEGIN_CLASS_RTTI( CPatrolPointComponent );
	PARENT_CLASS( CWanderPointComponent );	
END_CLASS_RTTI();