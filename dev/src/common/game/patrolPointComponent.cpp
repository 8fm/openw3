#include "build.h"
#include "patrolPointComponent.h"

IMPLEMENT_ENGINE_CLASS( CPatrolPointComponent );

IRenderResource*	CPatrolPointComponent::m_markerValidPP;
IRenderResource*	CPatrolPointComponent::m_markerInvalidPP;
IRenderResource*	CPatrolPointComponent::m_markerNoMeshPP; 
IRenderResource*	CPatrolPointComponent::m_markerSelectionPP;


void CPatrolPointComponent::InitializeMarkersPP()
{
	struct InitOnce
	{
		InitOnce( )
		{		
			m_markerValidPP = CWayPointComponent::CreateAgentMesh( 0.2f, 1.8f, Color::LIGHT_BLUE );
			m_markerInvalidPP = CWayPointComponent::CreateAgentMesh( 0.2f, 1.8f, Color::RED );
			m_markerNoMeshPP = CWayPointComponent::CreateAgentMesh( 0.2f, 1.8f, Color::GRAY );
			m_markerSelectionPP = CWayPointComponent::CreateAgentMesh( 0.24f, 1.8f, Color::WHITE, true );
		}
	};
	static InitOnce initOncePP;
}
