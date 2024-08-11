/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "fxTrackItemMovement.h"
#include "fxTrackGroup.h"
#include "fxTrackItem.h"
#include "component.h"
#include "fxDefinition.h"


IMPLEMENT_ENGINE_CLASS( CFXTrackItemMovement );

CFXTrackItemMovement::CFXTrackItemMovement()
: CFXTrackItemCurveBase( 3 )
{
}

/// Runtime player for movement
class CFXTrackItemMovementPlayData : public IFXTrackItemPlayData
{
public:
	const CFXTrackItemMovement*		m_trackItem;		//!< Data
	Vector							m_basePos;			//!< Base position of the moved component

public:
	CFXTrackItemMovementPlayData( const CFXTrackItemMovement* trackItem, CComponent* component )
		: IFXTrackItemPlayData( component , trackItem )
		, m_trackItem( trackItem )
	{
		ASSERT( component );
		m_basePos = component->GetPosition();
	};

	~CFXTrackItemMovementPlayData()
	{
		CComponent* component = (CComponent*)( m_node );
		if ( component )
		{
			component->SetPosition( m_basePos );
		}
	}

	virtual void OnStop() {}

	virtual void OnTick( const CFXState& fxState, Float timeDelta )
	{
		CComponent* component = (CComponent*)( m_node );
		if ( component )
		{
			// Evaluate curve value
			const Vector val = m_trackItem->GetVectorFromCurve( fxState.GetCurrentTime() );
			component->SetPosition( m_basePos + val );
		}
	}
};

IFXTrackItemPlayData* CFXTrackItemMovement::OnStart( CFXState& fxState ) const
{
	CComponent* component = GetTrack()->GetTrackGroup()->GetAffectedComponent( fxState );
	if ( !component )
	{
		return NULL;
	}

	// Done
	return new CFXTrackItemMovementPlayData( this, component );
}

void CFXTrackItemMovement::SetName( const String& name )
{
}

String CFXTrackItemMovement::GetName() const
{
	return TXT("Movement");
}
