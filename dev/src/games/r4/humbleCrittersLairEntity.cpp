#include "build.h"
#include "humbleCritterslairEntity.h"
#include "humbleCrittersAI.h"
#include "r4BoidSpecies.h"
#include "humbleCritterAlgorithmData.h"
#include "humbleCritterUpdateJob.h"
#include "../../common/engine/freeCamera.h"

#include "../../common/game/swarmAlgorithmData.h"
#include "../../common/game/boidInstance.h"
#include "../../common/game/boidSpecies.h"
#include "../../common/core/feedback.h"
#include "../../common/engine/renderFrame.h"

//#define SWARM_DEBUG_GRID

///////////////////////////////////////////////////////////////
IMPLEMENT_ENGINE_CLASS( CHumbleCrittersLairEntity );

CHumbleCrittersLairEntity::CHumbleCrittersLairEntity()
	: m_breakCounter( 8 )
{
}

void CHumbleCrittersLairEntity::OnAttachFinished( CWorld* world )
{
	if ( m_params == NULL && GCommonGame->GetBoidSpecies() )
	{
		const CBoidLairParams*const params = GCommonGame->GetBoidSpecies()->GetParamsByName( m_boidSpeciesName );
		if ( params )
		{
			m_params		= params->As< CHumbleCritterLairParams >();
		}
		if ( m_params == NULL )
		{
			m_params = &CHumbleCritterLairParams::sm_defaultParams;
		}
	}
	CSwarmLairEntity::OnAttachFinished( world );
}

volatile CHumbleCrittersAlgorithmData* CHumbleCrittersLairEntity::GetAlgorithmData() const volatile
{
	return static_cast< volatile CHumbleCrittersAlgorithmData* >( m_swarmAlgorithmData );
}
CSwarmUpdateJob* CHumbleCrittersLairEntity::NewUpdateJob()
{
	return new ( CTask::Root ) CHumbleCrittersUpdateJob( this, m_memberLists[ m_currentStateIndex ], m_memberLists[ m_currentStateIndex+1 == MEMBER_STATES_LISTS ? 0 : m_currentStateIndex+1 ], m_swarmAlgorithmData );
}
CSwarmAlgorithmData* CHumbleCrittersLairEntity::NewAlgorithmData()
{
	ASSERT(m_params->IsValid(), TXT("Boid params m_isValid should be true !"));
	return new CHumbleCrittersAlgorithmData( this,  *static_cast<const CHumbleCritterLairParams *>(m_params) );
}	
void CHumbleCrittersLairEntity::NoticeFireInCone( const Vector& position, const Vector2& coneDir, Float coneHalfAngle, Float coneRange )
{
	m_fireInConeInfo.m_isPending	= true;
	m_fireInConeInfo.m_origin		= position.AsVector3();
	m_fireInConeInfo.m_dir			= coneDir;
	m_fireInConeInfo.m_halfAngle	= coneHalfAngle;
	m_fireInConeInfo.m_range		= coneRange;
}

void CHumbleCrittersLairEntity::OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flag )
{
	if (m_swarmAlgorithmData)
	{
		CSwarmLairEntity::OnGenerateEditorFragments( frame, flag );

		if ( GGame->IsFreeCameraEnabled() == false )
		{
			return;
		}
		const Vector3 &cameraPosition	= GGame->GetFreeCamera().GetPosition();
		
		CHumbleCrittersAlgorithmData *swarmAlgorithmData	= static_cast<CHumbleCrittersAlgorithmData *>(const_cast<CSwarmAlgorithmData *>( m_swarmAlgorithmData ));
		const SwarmEnviromentData* envData					= swarmAlgorithmData->GetEnviroment();
		for ( Uint32 x = 0; x < (Uint32)envData->GetWidth(); ++x )
		{
			for ( Uint32 y = 0; y < (Uint32)envData->GetHeight(); ++y )
			{
				const SwarmEnviromentCelData & cellData = envData->GetChannel( x, y );
				Matrix matrix;
				matrix.SetIdentity();
				Vector2 cellPosition = envData->GetCelWorldPositionFromCoordinates( x, y );
				Vector3 position( cellPosition.X, cellPosition.Y, 0.0f );

				if ( ( position - Vector3(cameraPosition.X, cameraPosition.Y, 0.0f ) ).SquareMag() < SWARM_DEBUG_DISTANCE * SWARM_DEBUG_DISTANCE )
				{			
					position.Z = cellData.m_z;
					matrix.SetTranslation( position );
					if ( cellData.m_flags & CDF_BLOCKED )
					{
						frame->AddDebugCircle( position, 0.25f,  Matrix::IDENTITY, Color::RED, 5, false );
					}
					else
					{
						frame->AddDebugCircle( position, 0.25f,  Matrix::IDENTITY, Color::WHITE, 5, false );
					}
				}
			}
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////
//					CHumbleCritterLairParams
/////////////////////////////////////////////////////////////////////////////////

const CHumbleCritterLairParams CHumbleCritterLairParams::sm_defaultParams = CHumbleCritterLairParams(false);


CHumbleCritterLairParams::CHumbleCritterLairParams(Bool isValid)
	: CSwarmLairParams( E_TYPE, CNAME( HumbleCritterLair ), isValid )
	, m_maxVelocity( 0.5f )
	, m_turnRatio( 90.f )
	, m_individualRandomization( 0.3f )
	, m_actorsRangeDesired( 5.f )
	, m_actorsRangeMax( 10.f )
	, m_actorsGravity( -2.f )
	//, m_actorsBreakGravity( -1.f )
	, m_fireRangeMin( 1.f )
	, m_fireRangeMax( 9.f )
	, m_fireRepultion( 1.25f )
	, m_mutualUndesiredDensity( 5.f )
	, m_mutualRepultion( 0.75f )
	, m_hungerRatio( 11.2211f )
	, m_eatingTime( 3.f )
	, m_hungerGravity( 0.5f )
	, m_wanderTime( 4.5f )
	, m_wanderGravity( 0.1f )
	, m_wallsDistance( 4.f )
	, m_wallsRepulsion( 3.f )
	, m_panicSpeedBoost( 2.f )
	, m_panicActorsGravity( -2.f )
	, m_actorRangeMultWhenPanic( 3.f )
	, m_burnResistance( 0.5f )
	, m_postPanicDeathChance( 0.33333f )
	, m_attackRange( 2.0f )
	, m_hasAttackBehavior( true )
	, m_testLocation_hack( true )
	, m_walkSideway(false)
{

}

Bool CHumbleCritterLairParams::VirtualCopyTo(CBoidLairParams* const params)const
{
	CHumbleCritterLairParams *const humbleParams = params->As<CHumbleCritterLairParams>();
	if (humbleParams)
	{
		*humbleParams = *this;
		return true;
	}
	return false;
}

Bool CHumbleCritterLairParams::ParseXmlAttribute(const SCustomNodeAttribute & att)
{
	if ( att.m_attributeName == CNAME( maxVelocity ) )
	{
		if (att.GetValueAsFloat( m_maxVelocity ) == false)
		{
			GFeedback->ShowError(TXT("Boid XML Error: maxVelocity is not defined as a float"));
			return false;
		}
	}
	else if ( att.m_attributeName == CNAME( turnRatio ) )
	{
		if (att.GetValueAsFloat( m_turnRatio ) == false)
		{
			GFeedback->ShowError(TXT("Boid XML Error: turnRatio is not defined as a float"));
			return false;
		}	
	}
	else if (att.m_attributeName == CNAME( individualRandomization ))
	{
		if (att.GetValueAsFloat( m_individualRandomization ) == false)
		{
			GFeedback->ShowError(TXT("Boid XML Error: individualRandomization is not defined as a float"));
			return false;
		}	
	}
	else if (att.m_attributeName == CNAME( actorsRangeDesired ))
	{
		if (att.GetValueAsFloat( m_actorsRangeDesired ) == false)
		{
			GFeedback->ShowError(TXT("Boid XML Error: actorsRangeDesired is not defined as a float"));
			return false;
		}	
	}
	else if (att.m_attributeName == CNAME( actorsRangeMax ))
	{
		if (att.GetValueAsFloat( m_actorsRangeMax ) == false)
		{
			GFeedback->ShowError(TXT("Boid XML Error: actorsRangeMax is not defined as a float"));
			return false;
		}	
	}
	else if (att.m_attributeName == CNAME( actorsGravity ))
	{
		if (att.GetValueAsFloat( m_actorsGravity ) == false)
		{
			GFeedback->ShowError(TXT("Boid XML Error: actorsGravity is not defined as a float"));
			return false;
		}	
	}
	else if (att.m_attributeName == CNAME( fireRangeMin ))
	{
		if (att.GetValueAsFloat(m_fireRangeMin) == false)
		{
			GFeedback->ShowError(TXT("Boid XML Error: fireRangeMin is not defined as a float"));
			return false;
		}	
	}
	else if (att.m_attributeName == CNAME( fireRangeMax ))
	{
		if (att.GetValueAsFloat( m_fireRangeMax ) == false)
		{
			GFeedback->ShowError(TXT("Boid XML Error: fireRangeMax is not defined as a float"));
			return false;
		}	
	}
	else if (att.m_attributeName == CNAME( fireRepultion ))
	{
		if (att.GetValueAsFloat( m_fireRepultion ) == false)
		{
			GFeedback->ShowError(TXT("Boid XML Error: fireRepultion is not defined as a float"));
			return false;
		}	
	}
	else if (att.m_attributeName == CNAME( mutualUndesiredDensity ))
	{
		if (att.GetValueAsFloat( m_mutualUndesiredDensity ) == false)
		{
			GFeedback->ShowError(TXT("Boid XML Error: mutualUndesiredDensity is not defined as a float"));
			return false;
		}	
	}
	else if (att.m_attributeName == CNAME( mutualRepultion ))
	{
		if (att.GetValueAsFloat( m_mutualRepultion ) == false)
		{
			GFeedback->ShowError(TXT("Boid XML Error: mutualRepultion is not defined as a float"));
			return false;
		}	
	}
	else if (att.m_attributeName == CNAME( hungerRatio ))
	{
		if (att.GetValueAsFloat( m_hungerRatio ) == false)
		{
			GFeedback->ShowError(TXT("Boid XML Error: hungerRatio is not defined as a float"));
			return false;
		}	
	}
	else if (att.m_attributeName == CNAME( eatingTime ))
	{
		if (att.GetValueAsFloat( m_eatingTime ) == false)
		{
			GFeedback->ShowError(TXT("Boid XML Error: eatingTime is not defined as a float"));
			return false;
		}	
	}
	else if (att.m_attributeName == CNAME( hungerGravity ))
	{
		if (att.GetValueAsFloat( m_hungerGravity ) == false)
		{
			GFeedback->ShowError(TXT("Boid XML Error: hungerGravity is not defined as a float"));
			return false;
		}	
	}
	else if (att.m_attributeName == CNAME( wanderTime ))
	{
		if (att.GetValueAsFloat( m_wanderTime ) == false)
		{
			GFeedback->ShowError(TXT("Boid XML Error: wanderTime is not defined as a float"));
			return false;
		}	
	}
	else if (att.m_attributeName == CNAME( wanderGravity ))
	{
		if (att.GetValueAsFloat( m_wanderGravity ) == false)
		{
			GFeedback->ShowError(TXT("Boid XML Error: wanderGravity is not defined as a float"));
			return false;
		}	
	}
	else if (att.m_attributeName == CNAME( wallsDistance ))
	{
		if (att.GetValueAsFloat( m_wallsDistance ) == false)
		{
			GFeedback->ShowError(TXT("Boid XML Error: wallsDistance is not defined as a float"));
			return false;
		}	
	}
	else if (att.m_attributeName == CNAME( wallsRepulsion ))
	{
		if (att.GetValueAsFloat( m_wallsRepulsion ) == false)
		{
			GFeedback->ShowError(TXT("Boid XML Error: wallsRepulsion is not defined as a float"));
			return false;
		}	
	}
	else if (att.m_attributeName == CNAME( panicSpeedBoost ))
	{
		if (att.GetValueAsFloat( m_panicSpeedBoost ) == false)
		{
			GFeedback->ShowError(TXT("Boid XML Error: panicSpeedBoost is not defined as a float"));
			return false;
		}	
	}
	else if (att.m_attributeName == CNAME( panicActorsGravity ))
	{
		if (att.GetValueAsFloat( m_panicActorsGravity ) == false)
		{
			GFeedback->ShowError(TXT("Boid XML Error: panicActorsGravity is not defined as a float"));
			return false;
		}	
	}
	else if (att.m_attributeName == CNAME( actorRangeMultWhenPanic ))
	{
		if (att.GetValueAsFloat( m_actorRangeMultWhenPanic ) == false)
		{
			GFeedback->ShowError(TXT("Boid XML Error: actorRangeMultWhenPanic is not defined as a float"));
			return false;
		}	
	}
	else if (att.m_attributeName == CNAME( burnResistance ))
	{
		if (att.GetValueAsFloat( m_burnResistance ) == false)
		{
			GFeedback->ShowError(TXT("Boid XML Error: burnResistance is not defined as a float"));
			return false;
		}	
	}
	else if (att.m_attributeName == CNAME( postPanicDeathChance ))
	{
		if (att.GetValueAsFloat( m_postPanicDeathChance ) == false)
		{
			GFeedback->ShowError(TXT("Boid XML Error: postPanicDeathChance is not defined as a float"));
			return false;
		}	
	}
	else if (att.m_attributeName == CNAME(attackRange))
	{
		if (att.GetValueAsFloat( m_attackRange ) == false)
		{
			GFeedback->ShowError(TXT("Boid XML Error: attackRange is not defined as a float"));
			return false;
		}
	}
	else if (att.m_attributeName == CNAME(hasAttackBehaviour))
	{
		if (att.GetValueAsBool( m_hasAttackBehavior ) == false)
		{
			GFeedback->ShowError(TXT("Boid XML Error: hasAttackBehaviour is not defined as a bool"));
			return false;
		}
	}
	else if (att.m_attributeName == CNAME(testLocation_hack))
	{
		if (att.GetValueAsBool( m_testLocation_hack ) == false)
		{
			GFeedback->ShowError(TXT("Boid XML Error: m_testLocation_hack is not defined as a bool"));
			return false;
		}
	}
	else if (att.m_attributeName == CNAME(walkSideway))
	{
		if (att.GetValueAsBool( m_walkSideway ) == false)
		{
			GFeedback->ShowError(TXT("Boid XML Error: walkSideway is not defined as a bool"));
			return false;
		}
	}
	else
	{
		return CSwarmLairParams::ParseXmlAttribute(att);
	}
	return true;
}