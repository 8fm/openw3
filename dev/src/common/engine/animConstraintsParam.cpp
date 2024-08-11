
#include "build.h"
#include "animConstraintsParam.h"
#include "behaviorGraph.h"

IMPLEMENT_ENGINE_CLASS( CAnimConstraintsParam );

CAnimConstraintsParam::CAnimConstraintsParam()
	: m_graph( NULL )
{

}

const CBehaviorGraph* CAnimConstraintsParam::GetConstraintGraph() const
{
	return m_graph.Get();
}
