/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "fxTrackItemCameraShake.h"
#include "fxTrackGroup.h"
#include "game.h"
#include "world.h"
#include "entity.h"
#include "component.h"

IMPLEMENT_ENGINE_CLASS( CFXTrackItemCameraShake );
RED_DEFINE_STATIC_NAME( ExecuteCameraShake );


/// Runtime player for particles
class CFXTrackItemCameraShakePlayData : public IFXTrackItemPlayData
{
public:
	const CFXTrackItemCameraShake*	m_track;
	Float							m_effectFullForceRadius;
	Float							m_effectMaxRadius;
	Int32								m_shakeNum;
	THandle< CComponent >			m_positioningComponent;

public:
	CFXTrackItemCameraShakePlayData( const CFXTrackItemCameraShake* trackItem, CComponent* comp, Int32 shakeNum, Float effectFullForceRadius, Float effectMaxRadius )
		: IFXTrackItemPlayData( GGame->GetActiveWorld() ? SafeCast<CNode>(GGame->GetActiveWorld()->GetCameraDirector()->GetTopmostCameraObject().Get()) : NULL, trackItem )
		, m_positioningComponent( comp )
		, m_track( trackItem )
		, m_shakeNum( shakeNum )
		, m_effectFullForceRadius( effectFullForceRadius )
		, m_effectMaxRadius( effectMaxRadius )
	{
	};

	~CFXTrackItemCameraShakePlayData()
	{
	}

	virtual void OnStop() {}

	virtual void OnTick( const CFXState& fxState, Float timeDelta )
	{
		CEntity* ent = fxState.GetEntity();

		Vector position;

		CComponent *positioningComponent = m_positioningComponent.Get();
		
		if ( positioningComponent )
		{
			position = positioningComponent->GetWorldPosition();
		}
		else
		{
			position = ent->GetWorldPosition();
		}

		Float distance = position.DistanceTo( GGame->GetActiveWorld()->GetCameraDirector()->GetCameraPosition() );

		if ( distance > m_effectMaxRadius )
		{
			return;
		}

		Float distanceMultiplier = 1.0f;

		if ( distance >= m_effectFullForceRadius )
		{
			distanceMultiplier = 1.0f - (distance - m_effectFullForceRadius) / ( m_effectMaxRadius - m_effectFullForceRadius);
		}

		if( m_node )
			CallFunction( m_node, CNAME( ExecuteCameraShake ), m_shakeNum, distanceMultiplier * m_track->GetCurveValue( fxState.GetCurrentTime() ) );
	}
};




CFXTrackItemCameraShake::CFXTrackItemCameraShake()
	: CFXTrackItemCurveBase( 1 )
	, m_effectFullForceRadius ( 3.0f )
	, m_effectMaxRadius ( 10.0f )
{

}

IFXTrackItemPlayData* CFXTrackItemCameraShake::OnStart( CFXState& fxState ) const
{
	// Fill choice control with values
	CEnum* en = SRTTI::GetInstance().FindEnum( CNAME( ECameraShake ) );

	ASSERT( en && TXT("Unable to find enum ECameraShake") );
	if ( !en )
	{
		return NULL;
	}
	Int32 val = -1;
	Bool result = en->FindValue( m_shakeType, val );

	ASSERT( result && TXT("Cannot find ECameraShake shake type") );
	if ( !result )
	{
		return NULL;
	}

	CComponent *trackComponent = GetTrack()->GetTrackGroup()->GetAffectedComponent( fxState );

	if ( GGame->GetActiveWorld()->GetCameraDirector()->GetTopmostCameraObject() )
	{
		CFXTrackItemCameraShakePlayData *ppd = new CFXTrackItemCameraShakePlayData( this, trackComponent, val, m_effectFullForceRadius, m_effectMaxRadius );
		return ppd;
	}
	else
	{
		// Cannot shake camera if there is no camera
		return NULL;
	}
}
