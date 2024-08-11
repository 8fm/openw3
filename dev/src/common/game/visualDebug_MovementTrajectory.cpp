
#include "build.h"
#include "visualDebug_MovementTrajectory.h"
#include "../engine/gameTimeManager.h"
#include "../engine/renderFrame.h"

IMPLEMENT_ENGINE_CLASS( CVisualDebug_MovementTrajectory );

#define MOVEMENT_TRAJECTORY_DEBUG_STEP_SPLIT	0.05f
#define MOVEMENT_TRAJECTORY_DEBUG_STEPS			80

CVisualDebug_MovementTrajectory::CVisualDebug_MovementTrajectory() 
	: m_isAdded( false )
	, m_movingAgentComponent( NULL ) 
{

}

void CVisualDebug_MovementTrajectory::Init( CActor* entity )
{
	m_entity = entity;

	if ( m_isAdded )
	{
		if ( m_entity && m_entity->GetVisualDebug() )
		{
			m_entity->GetVisualDebug()->RemoveObject( this );
			m_isAdded = false;
		}
	}
	if ( !m_isAdded && m_entity && m_entity->GetVisualDebug() )
	{
		ComponentIterator<CMovingAgentComponent> it( m_entity );
		m_movingAgentComponent = *it;
		m_entity->GetVisualDebug()->AddObject( this );
		m_isAdded = true;
	}
}

void CVisualDebug_MovementTrajectory::Reset()
{
	if ( m_isAdded && m_entity && m_entity->GetVisualDebug() )
	{
		m_entity->GetVisualDebug()->RemoveObject( this );
	}
}

void CVisualDebug_MovementTrajectory::funcReset( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	Reset();
}

void CVisualDebug_MovementTrajectory::funcInit( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< CActor >, entity, THandle< CActor >( NULL ) );
	FINISH_PARAMETERS;

	Init( entity.Get() );
}

void CVisualDebug_MovementTrajectory::Render( CRenderFrame* frame, const Matrix& matrix )
{
	if ( !m_movingAgentComponent )
	{
		return;
	}

	m_posHistory[0] = m_posHistory[1];
	m_posHistory[1] = m_entity->GetLocalToWorld();

	Vector pos = m_posHistory[1].GetTranslation();
	Vector deltaPos = pos - m_posHistory[0].GetTranslation();

	const Float d = deltaPos.Normalize3();
	const Float dt = GGame->GetTimeManager()->GetLastTickTime();

	if ( d!=0 && m_posHistory[0].GetRow(1) != Vector::ZEROS && dt > 0 )
	{
		deltaPos.Z = 0;
		deltaPos /= dt;

		EulerAngles deltaRotation = m_posHistory[1].ToEulerAnglesFull() - m_posHistory[0].ToEulerAnglesFull();
		if ( Abs( deltaRotation.Yaw ) > 180 )
		{
			if ( deltaRotation.Yaw < 0 )
			{
				deltaRotation.Yaw = 360 + deltaRotation.Yaw;
			}
			else
			{
				deltaRotation.Yaw = - 360 + deltaRotation.Yaw;
			}
		}

		Vector nextPos;
		Float heading = m_posHistory[1].ToEulerAngles().Yaw;
		const Float speed = d/dt;
		Vector dp;

		for ( Int32 i=0; i < MOVEMENT_TRAJECTORY_DEBUG_STEPS; ++i )
		{
			dp = EulerAngles::YawToVector2( heading ) * speed * MOVEMENT_TRAJECTORY_DEBUG_STEP_SPLIT;
			nextPos = pos + dp;

			frame->AddDebugLine( pos, nextPos, Color::LIGHT_BLUE, true );

			pos = nextPos;
			heading += deltaRotation.Yaw * MOVEMENT_TRAJECTORY_DEBUG_STEP_SPLIT / dt;
		}
	}
}
