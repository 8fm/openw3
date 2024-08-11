#include "build.h"
#include "physicsCharacterVirtualController.h"
#include "renderFrame.h"
#include "component.h"
//#include "physicsCharacterWrapper.h"

//////////////////////////////////////////////////////////////////////////

CVirtualCharacterController::CVirtualCharacterController( const CName& name, const CName& boneName, const Vector& localOffset,
														  const Float height, const Float radius, class CPhysicsCharacterWrapper* parent )
    : m_name( name )
	, m_boneName( boneName )
	, m_boneIndex( -1 )
    , m_localOffset( localOffset )
	, m_globalPosition( Vector::ZEROS )
	, m_height( height )
	, m_physicalRadius( radius )
    , m_virtualRadius( radius )
	, m_parentController( parent )
	, m_collisions( false )
	, m_collisionResponse( false )
	, m_enabled( false )
	, m_localOffsetInModelSpace( false )
	, m_collisionGrabber( false )
	, m_collisionGrabberGroupMask( 0 )
	, m_targetable( false )
	, m_collisionsEventName( CName::NONE )
	, m_additionalRaycastCheck( Vector::ZEROS )
	, m_additionalRaycastCheckEventName( CName::NONE )
{
}

//////////////////////////////////////////////////////////////////////////

void CVirtualCharacterController::OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flags )
{
#ifndef NO_EDITOR

	if( SHOW_PhysXTraceVisualization == flags )
	{
		FixedCapsule caps( GetGlobalPosition(), m_physicalRadius, m_height);
		Color color( Color::BLACK );
		if ( m_collisionResponse )
		{
			color = Color::GREEN;
		}
		else if ( m_collisionGrabber )
		{
			color = Color::GRAY;
		}
		else if ( m_enabled )
		{
			color = Color::WHITE;
		}
		frame->AddDebugCapsule( caps, Matrix::IDENTITY, color );
	}

#endif // ndef NO_EDITOR
}

//////////////////////////////////////////////////////////////////////////

// parentMatrix is either a bone or parent entity's matrix
void CVirtualCharacterController::UpdateGlobalPosition( const Matrix& parentMatrixWS, const Matrix& acMatrix )
{
	Vector offset = m_localOffset;

#ifdef USE_PHYSX
	// use pitch adjustment only if not attached to a bone
	if ( m_boneName == CName::NONE )
	{
		if ( m_parentController != nullptr )
		{
			// > internal pitch
			const Float pitch = m_parentController->GetVirtualControllerPitch();
			offset.Y *= cosf( pitch );
			offset.Z += m_localOffset.Y * sinf( pitch );
		}
	}
#endif

	// compute final position
	if ( m_localOffsetInModelSpace )
	{
		// apply ac rotation
		Vector offsetInAC = acMatrix.TransformPoint( offset ) - acMatrix.GetTranslationRef();
		m_globalPosition = parentMatrixWS.GetTranslationRef() + offsetInAC;
	}
	else
	{
		m_globalPosition = parentMatrixWS.TransformPoint( offset );
	}
}

//////////////////////////////////////////////////////////////////////////

Vector CVirtualCharacterController::GetGlobalPosition() const
{
	return m_globalPosition;
}

void CVirtualCharacterController::GetGlobalBoundingBox( Box& box ) const
{
	const Float radius = GetCurrentRadius();
	box.Min = m_globalPosition - Vector( radius, radius, 0 );
	box.Max = m_globalPosition + Vector( radius, radius, m_height );
}

//////////////////////////////////////////////////////////////////////////
