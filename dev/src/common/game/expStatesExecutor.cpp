
#include "build.h"
#include "expStatesExecutor.h"
#include "expBaseExecutor.h"
#include "expSlideExecutor.h"
#include "expToPointSliderExecutor.h"
#include "expStepsExecutor.h"

#include "../../common/engine/animatedComponent.h"

const Float ExpPreLoopPostExecutor::DISTANCE_TO_SWITCH_TO_POST = 2.25f;

ExpPreLoopPostExecutor::ExpPreLoopPostExecutor( const IExploration* e, const IExplorationDesc* desc, const ExecutorSetup& setup )
	: m_state( IS_Loop )
	, m_preExe( NULL )
	, m_loopExe( NULL )
	, m_postExe( NULL )
    , m_postExeL( NULL )
    , m_postExeR( NULL )
    , m_preExeL( NULL )
    , m_preExeR( NULL )
	, m_exploration( e )
	, m_explorationDesc( desc )
	, m_entity( setup.m_entity )	
	, m_distanceToSwitchToPost( DISTANCE_TO_SWITCH_TO_POST )
{

}

ExpPreLoopPostExecutor::~ExpPreLoopPostExecutor()
{
    delete m_preExeL;
    delete m_preExeR;
	delete m_loopExe;
    delete m_postExeL;
    delete m_postExeR;
}

void ExpPreLoopPostExecutor::ConnectPreLeft( ExpToPointSliderExecutor* exe )
{
	ASSERT( !m_preExe );
	m_preExeL = exe;
}

void ExpPreLoopPostExecutor::ConnectPostLeft( ExpToPointSliderExecutor* exe )
{
	ASSERT( !m_postExe );
	m_postExeL = exe;
}

void ExpPreLoopPostExecutor::ConnectPreRight( ExpToPointSliderExecutor* exe )
{
    ASSERT( !m_preExe );
    m_preExeR = exe;
}

void ExpPreLoopPostExecutor::ConnectPostRight( ExpToPointSliderExecutor* exe )
{
    ASSERT( !m_postExe );
    m_postExeR = exe;
}


void ExpPreLoopPostExecutor::ConnectLoop( IExpExecutor* exe )
{
	ASSERT( !m_loopExe );
	m_loopExe = exe;
}

void ExpPreLoopPostExecutor::Update( const ExpExecutorContext& context, ExpExecutorUpdateResult& result )
{
	switch ( m_state )
	{
	case IS_Loop:
        {
            result.m_finishPoint = CanGoToPreOrPost();

            m_loopExe->Update( context, result );

			if ( result.m_finished)
			{
				result.m_finished = false;

				if ( result.m_finishPoint == -1 )
				{
					SlideToPrePoint();

                    if(result.m_side == 0)
                    {
                        m_preExe = m_preExeL;
                    }
                    else
                    {
                        m_preExe = m_preExeR;
                    }

					m_preExe->SyncAnim( result.m_timeRest );
                    m_preExe->InitializeSlide();

					m_state = IS_Pre;
				}
				else if ( result.m_finishPoint == 1 )
				{
					SlideToPostPoint();

                    if(result.m_side == 0)
                    {
                        m_postExe = m_postExeL;
                    }
                    else
                    {
                        m_postExe = m_postExeR;
                    }

                    m_postExe->SyncAnim( result.m_timeRest );
                    m_postExe->InitializeSlide();

					m_state = IS_Post;
				}
			}

			break;
		}


	case IS_Pre:
		{
			m_preExe->Update( context, result );		
			break;
		}


	case IS_Post:
		{
			m_postExe->Update( context, result );
			break;
		}
	}
}

void ExpPreLoopPostExecutor::GenerateDebugFragments( CRenderFrame* frame )
{
	switch ( m_state )
	{
	case IS_Pre:
		{
			m_preExe->GenerateDebugFragments( frame );
			break;
		}

	case IS_Loop:
		{
			m_loopExe->GenerateDebugFragments( frame );
			break;
		}

	case IS_Post:
		{
			m_postExe->GenerateDebugFragments( frame );
			break;
		}
	}
}

void ExpPreLoopPostExecutor::CalculateAnimationZToSwitchToPost()
{
	if( !m_postExeL )
		return;
	const CSkeletalAnimationSetEntry* animEntry = m_postExeL->GetAnimationEntry();
	CSkeletalAnimation* animation = animEntry ? animEntry->GetAnimation() : nullptr;
	if ( animation && animation->HasExtractedMotion() )
	{
		Float duration = animation->GetDuration();
		Matrix movement;
		animation->GetMovementAtTime( duration, movement );
		m_distanceToSwitchToPost = movement.GetTranslation().Z;		
	}
}

Int32 ExpPreLoopPostExecutor::CanGoToPreOrPost() const
{
	if ( !m_preExeL && !m_postExeL && !m_preExeR && !m_postExeR )
	{
		return 0;
	}

	const Vector p = m_entity->GetWorldPosition();

	Vector p1, p2;
	m_exploration->GetEdgeWS( p1, p2 );

	// Set p2 to the last step
	Float granularity;
	m_explorationDesc->UseEdgeGranularity( granularity );
	if( granularity > 0.0f )
	{
		Vector	targetPosition	= p;
		Int32	targetStep		= m_exploration->SetPointOnClosestGrain( targetPosition, granularity );
		Int32	totalSteps		= m_exploration->GetTotalNumberOfGrains( granularity );
		if( targetStep > totalSteps - 4 ) // minus the steps required to leave the ladder
		{
			return 1;
		}
		if( targetStep <= 0 )
		{
			return -1;
		}
		return 0;
	}

	Vector d = p2 - p1;
	const Float len = d.Normalize3();

	const Vector v( p.X - p1.X, p.Y - p1.Y, p.Z - p1.Z );

	const Float pOnEdgeActualDist = Vector::Dot3( v, d );
	const Float pOnEdgeActualDistToEnd = len - pOnEdgeActualDist;
	const Float pOnEdge = pOnEdgeActualDist / len;

	const Vector pointOnEdge = Vector::Interpolate( p1, p2, pOnEdge );

	Float p1o, p2o;
	m_explorationDesc->GetEdgePointsOffset( p1o, p2o );

	const Float p1o2 = p1o * p1o;
	const Float p2o2 = p2o * p2o;

	if ( pOnEdgeActualDist <= 0.4f || pointOnEdge.DistanceSquaredTo( p1 ) <= p1o2 )
	{
		return -1;
	}
	else if ( pOnEdgeActualDistToEnd <= m_distanceToSwitchToPost /* || pointOnEdge.DistanceSquaredTo( p2 ) <= p2o2 */ )
	{
		return 1;
	}

	return 0;
}

void ExpPreLoopPostExecutor::SlideToPrePoint()
{

}

void ExpPreLoopPostExecutor::SlideToPostPoint()
{

}



///////////////////////////////////////////////////////////



ExpPrePostExecutor::ExpPrePostExecutor( const IExploration* e, const IExplorationDesc* desc, const ExecutorSetup& setup )
    : m_state( IS_Start )
    , m_preExe( NULL )
    , m_postExe( NULL )
    , m_exploration( e )
    , m_explorationDesc( desc )
    , m_entity( setup.m_entity )
{

}

ExpPrePostExecutor::~ExpPrePostExecutor()
{
    delete m_preExe;
    delete m_postExe;
}

void ExpPrePostExecutor::ConnectPre( ExpSlideExecutor* exe )
{
    ASSERT( !m_preExe );
    m_preExe = exe;
}

void ExpPrePostExecutor::ConnectPost( ExpSlideExecutor* exe )
{
    ASSERT( !m_postExe );
    m_postExe = exe;
}

void ExpPrePostExecutor::Update( const ExpExecutorContext& context, ExpExecutorUpdateResult& result )
{
    switch ( m_state )
    {
    
    case IS_Start:
        {
            if(CalcSide())
            {
                m_state = IS_Pre;
            }
            else
            {
                m_state = IS_Post;
            }
            break;
        }

    case IS_Pre:
        {
            m_preExe->Update( context, result );		
            break;
        }


    case IS_Post:
        {
            m_postExe->Update( context, result );
            break;
        }
    }
}

void ExpPrePostExecutor::GenerateDebugFragments( CRenderFrame* frame )
{
    switch ( m_state )
    {
    case IS_Pre:
        {
            m_preExe->GenerateDebugFragments( frame );
            break;
        }

    case IS_Post:
        {
            m_postExe->GenerateDebugFragments( frame );
            break;
        }
    }
}

void ExpPrePostExecutor::SlideToPrePoint()
{

}

void ExpPrePostExecutor::SlideToPostPoint()
{

}

Bool ExpPrePostExecutor::CalcSide() const
{
    Vector p1, p2, n;
    m_exploration->GetEdgeWS( p1, p2 );
    const Vector p = ( p1 + p2 ) / 2.f;

    const Vector& ep = m_entity->GetWorldPositionRef();
    m_exploration->GetNormal( n );

    const Float dot2 = n.Dot2( ep - p );
    return dot2 > 0.f;
}