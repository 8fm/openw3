#include "build.h"
#include "boidNodeData.h"

CBoidNodeData::CBoidNodeData(CBoidInstance *const	boidInstance)
	: m_boidInstance( boidInstance )
	, m_currentAtomicAnimNode( NULL )
	, m_timeOut( 0.0f )
	, m_stateCounter( 0 )
{

}