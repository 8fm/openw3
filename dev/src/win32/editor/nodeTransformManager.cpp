
#include "Build.h"
#include "nodeTransformManager.h"

#include "../../common/engine/world.h"
#include "../../common/physics/physicsWorld.h"
#include "../../common/engine/entity.h"
#include "../../common/engine/clipMap.h"
#include "../../common/physics/physicsWorldUtils.h"
#include "../../common/engine/selectionManager.h"
#include "../../common/engine/nodeHelpers.h"

CNodeTransformManager::CNodeTransformManager( CWorld* world, CSelectionManager* selMan )
	: m_world( world )
	, m_selManager( selMan )
	, m_snapMode( SNAP_ToNothing )
	, m_snapOrgin( SNAP_ByPivot )
{
}

CNodeTransformManager::~CNodeTransformManager()
{
}

void CNodeTransformManager::TransformChangeStart( const TDynArray< CNode* >& nodes )
{
	for ( CNode* node : nodes )
	{
#ifndef NO_EDITOR
		node->EditorOnTransformChangeStart();
#endif
	}
}

Bool CNodeTransformManager::Move( const TDynArray< CNode* >& nodes, 
								  const Vector& delta )
{
	if ( nodes.Empty() )
	{
		return true;
	}

	if ( delta.Mag3() == 0.0f ) 
	{
		return true;
	}

	// Move roots
	for ( CNode* node : nodes )
	{
		#ifndef NO_MARKER_SYSTEMS
			if( node->GetTags().HasTag( CName(TXT("LockedObject")) ) == true)
			{
				continue;
			}
		#endif	// NO_MARKER_SYSTEMS

		Matrix toWorld = GetNodeLocalToWorldMatrix( node );
		Vector newPosition = toWorld.TransformPoint( node->GetPosition() ) + delta;
		newPosition = toWorld.FullInverted().TransformPoint( newPosition );

		CEntity *entity = Cast< CEntity >( node );
		if( m_snapMode != SNAP_ToNothing && entity )
		{
			Float boundingTilt = 0.0f;
			if( m_snapOrgin == SNAP_ByBoundingVolume )
			{
				CEntity* entity = Cast< CEntity >( node );
				if( entity )
				{
					Box box = entity->CalcBoundingBox();
					boundingTilt = box.CalcSize().Z / 2;
				}
			}

			if ( m_snapMode == SNAP_ToTerrainVisual )
			{
				const CClipMap* terrain = m_world->GetTerrain();
				if ( terrain )
				{
					terrain->GetHeightForWorldPosition( newPosition, newPosition.Z );
					newPosition.Z += boundingTilt;
				}
			}
			else 
			{
				Float snapSensitivityDistance = 1.0f; // please play with me

				Vector startVector( newPosition.X, newPosition.Y, newPosition.Z + snapSensitivityDistance );
				Vector stopVector( newPosition.X, newPosition.Y, newPosition.Z - snapSensitivityDistance );

				SPhysicsContactInfo contactInfo;
				CPhysicsEngine::CollisionMask include = GPhysicEngine->GetCollisionTypeBit( CNAME( Terrain ) );
				if ( m_snapMode == SNAP_ToStaticCollision )
				{
					include |= GPhysicEngine->GetCollisionTypeBit( CNAME( Static ) );
				}

				CPhysicsWorld* physicsWorld = nullptr;
				
				if ( m_world->GetPhysicsWorld( physicsWorld ) && physicsWorld->RayCastWithSingleResult( startVector, stopVector, include, 0, contactInfo ) == TRV_Hit )
				{
					newPosition.Z = contactInfo.m_position.Z + boundingTilt;
				}
			}
		}

		DoMoveNode( node, newPosition );
	}

	MarkModified( nodes );

	// Send event
	EDITOR_QUEUE_EVENT( CNAME( SelectionPropertiesChanged ), CreateEventData( m_world ) );
	return true;
}

void CNodeTransformManager::TransformChangeStop(const TDynArray< CNode* >& nodes )
{
	for ( CNode* node : nodes )
	{
#ifndef NO_EDITOR
		node->EditorOnTransformChangeStop();
#endif
	}
}

void CNodeTransformManager::DoMoveNode( CNode* node, const Vector& newPosition )
{
	node->SetPosition( newPosition );

#ifndef NO_EDITOR
	node->EditorOnTransformChanged();
#endif
}

Bool CNodeTransformManager::Rotate( const TDynArray< CNode* >& nodes, 
									const Matrix& rotationMatrix, 
									Bool individually,
									CNode* pivotNode,
									Vector& pivotOffset )
{
	if ( nodes.Empty() )
	{
		return true;
	}

	// Move roots
	for ( CNode* node : nodes )
	{
		#ifndef NO_MARKER_SYSTEMS
		if( node->GetTags().HasTag( CName(TXT("LockedObject")) ) == true)
		{
			continue;
		}
		#endif	// NO_MARKER_SYSTEMS

		// Rotate
		Matrix toWorld = GetNodeLocalToWorldMatrix( node );
		toWorld.SetScale33( Vector( 1.0f, 1.0f, 1.0f ) / toWorld.GetScale33() );
		Matrix newLocalToWorld = node->GetRotation().ToMatrix() * toWorld * rotationMatrix;
		newLocalToWorld = newLocalToWorld * toWorld.FullInverted();
		node->SetRotation( newLocalToWorld.ToEulerAngles() );

		// Move center
		if ( !individually )
		{
			Vector oldPosition = node->GetPosition();
			Vector offsetedPosition = pivotNode ? pivotNode->GetPosition() + pivotOffset : pivotOffset;
			Vector offset = oldPosition - offsetedPosition;
			Vector newPosition = offsetedPosition + rotationMatrix.TransformPoint( offset );
			CEntity *entity = Cast< CEntity >( node );

			// For entities, go through the streaming system
			if ( entity )
			{
				DoMoveNode( entity, newPosition );
			}
			else // otherwise just move the node
			{
				node->SetPosition( newPosition );
			}

			if ( node == pivotNode )
			{
				pivotOffset -= node->GetPosition() - oldPosition;
			}
		}
#ifndef NO_EDITOR
		node->EditorOnTransformChanged();
#endif
	}

	MarkModified( nodes );

	// Send event
	EDITOR_QUEUE_EVENT( CNAME( SelectionPropertiesChanged ), CreateEventData( m_world ) );
	return true;
}

Bool CNodeTransformManager::Rotate( const TDynArray< CNode* >& nodes, 
									const EulerAngles& eulerAngles, 
									Bool individually,
									CNode* pivotNode,
									Vector& pivotOffset )
{
	if ( nodes.Empty() )
	{
		return true;
	}

	// Assemble matrix
	Matrix rotationMatrix = eulerAngles.ToMatrix();

	// Move roots
	for ( CNode* node : nodes )
	{
		#ifndef NO_MARKER_SYSTEMS
			if( node->GetTags().HasTag( CName(TXT("LockedObject")) ) == true)
			{
				continue;
			}
		#endif	// NO_MARKER_SYSTEMS

		// Rotate
		Matrix toWorld = GetNodeLocalToWorldMatrix( node );
		toWorld.SetScale33( Vector( 1.0f, 1.0f, 1.0f ) / toWorld.GetScale33() );
		Matrix newLocalToWorld = node->GetRotation().ToMatrix() * toWorld * rotationMatrix;
		newLocalToWorld = newLocalToWorld * toWorld.FullInverted();
			
		// crappy hack to fix locking due to precision errors
		EulerAngles newEulerAngles;
		if ( newLocalToWorld.GetRow(1).A[2] > 0.99995f )
		{
			newEulerAngles.Roll  =  RAD2DEG( atan2f( newLocalToWorld.GetRow(2).A[0], newLocalToWorld.GetRow(0).A[0] ) );
			newEulerAngles.Pitch =  90.0f;
			newEulerAngles.Yaw	  =  0.0f;
		}
		else if ( newLocalToWorld.GetRow(1).A[2] < -0.99995f )
		{
			newEulerAngles.Roll  =  RAD2DEG( atan2f( newLocalToWorld.GetRow(2).A[0], newLocalToWorld.GetRow(0).A[0] ) );
			newEulerAngles.Pitch = -90.0f;
			newEulerAngles.Yaw	  =  0.0f;
		}
		else
		{
			newEulerAngles.Roll  = -RAD2DEG( atan2f( newLocalToWorld.GetRow(0).A[2], newLocalToWorld.GetRow(2).A[2] ) );
			newEulerAngles.Pitch =  RAD2DEG( asin( newLocalToWorld.GetRow(1).A[2] ) );
			newEulerAngles.Yaw	  = -RAD2DEG( atan2f( newLocalToWorld.GetRow(1).A[0], newLocalToWorld.GetRow(1).A[1] ) );
		}

		node->SetRotation( newEulerAngles );

		// Move center
		if ( !individually )
		{
			Vector oldPosition = node->GetPosition();
			Vector offsetedPosition = pivotNode ? pivotNode->GetPosition() + pivotOffset : pivotOffset;
			Vector offset = oldPosition - offsetedPosition;
			Vector newPosition = offsetedPosition + rotationMatrix.TransformPoint( offset );
			CEntity *entity = Cast< CEntity >( node );

			// For entities, go through the streaming system
			if ( entity )
			{
				DoMoveNode( entity, newPosition );
			}
			else // otherwise just move the node
			{
				node->SetPosition( newPosition );
			}

			if ( node == pivotNode )
			{
				pivotOffset -= node->GetPosition() - oldPosition;
			}
		}
#ifndef NO_EDITOR
		node->EditorOnTransformChanged();
#endif
	}

	MarkModified( nodes );

	// Send event
	EDITOR_QUEUE_EVENT( CNAME( SelectionPropertiesChanged ), CreateEventData( m_world ) );
	return true;
}

Bool CNodeTransformManager::Scale( const TDynArray< CNode* >& nodes, 
								   const Vector& delta, 
								   const Matrix& space,
								   Bool individually,
								   CNode* pivotNode,
								   Vector& pivotOffset,
								   ERPWidgetSpace widgetSpace )
{
	if ( nodes.Empty() )
	{
		return true;
	}

	// Calculate space inversion
	Matrix invSpace = space.Inverted();
	Float eps = 0.0001f;

	Vector scaleInWidgetSpace = invSpace.TransformVector( delta );
	Bool enlarge = ( widgetSpace == RPWS_Local ? scaleInWidgetSpace.Lower3() >= -eps : delta.Lower3() >= -eps );

	Vector newScale = scaleInWidgetSpace.Abs() * 0.025f;
	newScale += enlarge ? Vector::ONES : -Vector::ONES;

	Bool uniform = ( Abs( newScale.X - newScale.Y ) <= eps && Abs( newScale.X - newScale.Z ) < eps );

	// Move roots
	for ( CNode* node : nodes )
	{
		#ifndef NO_MARKER_SYSTEMS
			if( node->GetTags().HasTag( CName(TXT("LockedObject")) ) == true)
			{
				continue;
			}
		#endif	// NO_MARKER_SYSTEMS

		Matrix toWorld = node->GetRotation().ToMatrix() * GetNodeLocalToWorldMatrix( node );
		Matrix toWorldInv = toWorld.Inverted();

		// Scale
		if ( !uniform )
		{
			if ( widgetSpace == RPWS_Global )
			{
				Vector scaleX = toWorldInv.TransformVector( Vector( delta.X, 0, 0 ) );
				Vector scaleY = toWorldInv.TransformVector( Vector( 0, delta.Y, 0 ) );
				Vector scaleZ = toWorldInv.TransformVector( Vector( 0, 0, delta.Z ) );

				newScale = ( scaleX.Abs() + scaleY.Abs() + scaleZ.Abs() ) * 0.025f;
			}
			else
			{
				newScale = toWorldInv.TransformVector( delta ) * 0.025f;
				newScale = newScale.Abs();
			}
			// we need to update scaling 'directory'
			newScale += enlarge ? Vector::ONES : -Vector::ONES;
		}

		node->SetScale( node->GetScale() * newScale.Abs() );

		if( !individually )
		{
			// Move center
			Vector oldPosition = node->GetPosition();
			Vector offsetedPosition = pivotNode ? pivotNode->GetPosition() + pivotOffset : pivotOffset;
			Vector offsetLocal = toWorldInv.TransformVector( oldPosition - offsetedPosition );
			Vector offsetNew = toWorld.TransformVector( offsetLocal * newScale.Abs() );
			node->SetPosition( offsetedPosition + offsetNew );

			if ( node == pivotNode )
			{
				pivotOffset -= node->GetPosition() - oldPosition;
			}
		}
#ifndef NO_EDITOR
		node->EditorOnTransformChanged();
#endif
	}

	MarkModified( nodes );

	// Send event
	EDITOR_QUEUE_EVENT( CNAME( SelectionPropertiesChanged ), CreateEventData( m_world ) );
	return true;
}

// -------------------------------

void CNodeTransformManager::TransformSelectionStart()
{
	TransformChangeStart( m_selManager->GetSelectedRoots() );
}

Bool CNodeTransformManager::MoveSelection( const Vector& delta )
{
	Bool result = Move( m_selManager->GetSelectedRoots(), delta );
	m_selManager->RefreshPivot();
	return result;
}

Bool CNodeTransformManager::RotateSelection( const Matrix& rotationMatrix, 
											 Bool individually )
{
	Vector offset = m_selManager->GetPivotOffset();
	Bool result = Rotate( m_selManager->GetSelectedRoots(), rotationMatrix, individually, m_selManager->GetPivot(), offset );
	m_selManager->SetPivotOffset( offset );
	return result;
}

Bool CNodeTransformManager::RotateSelection( const EulerAngles& eulerAngles, 
											 Bool individually )
{
	Vector offset = m_selManager->GetPivotOffset();
	Bool result = Rotate( m_selManager->GetSelectedRoots(), eulerAngles, individually, m_selManager->GetPivot(), offset );
	m_selManager->SetPivotOffset( offset );
	return result;
}

Bool CNodeTransformManager::ScaleSelection( const Vector& delta, 
											const Matrix& space, 
											ERPWidgetSpace widgetSpace, 
											Bool individually )
{
	Vector offset = m_selManager->GetPivotOffset();
	Bool result = Scale( m_selManager->GetSelectedRoots(), delta, space, individually, m_selManager->GetPivot(), offset, widgetSpace );
	m_selManager->SetPivotOffset( offset );
	return result;
}

void CNodeTransformManager::TransformSelectionStop()
{
	TransformChangeStop( m_selManager->GetSelectedRoots() );
}

Matrix CNodeTransformManager::CalculatePivotSpace( ERPWidgetSpace widgetSpace )
{
	Matrix pivotSpace;

	if ( m_selManager->GetPivot() )
	{
		switch ( widgetSpace )
		{
		case RPWS_Foreign:
			pivotSpace = m_foreignRotation.ToMatrix();
			pivotSpace.SetTranslation( m_selManager->GetPivotPosition() );
			break;

		case RPWS_Local:
			pivotSpace =  m_selManager->GetPivot()->GetLocalToWorld();
			pivotSpace.V[0].Normalize3();
			pivotSpace.V[1].Normalize3();
			pivotSpace.V[2].Normalize3();
			pivotSpace.SetTranslation( m_selManager->GetPivotPosition() );
			break;

		case RPWS_Global:
			pivotSpace.SetIdentity();
			pivotSpace.SetTranslation( m_selManager->GetPivotPosition() );
			break;
		}
	}
	else
	{
		pivotSpace.SetIdentity();
	}

	return pivotSpace;
}

Bool CNodeTransformManager::MarkModified( const TDynArray< CNode* >& nodes )
{
	for ( CNode* node : nodes )
	{
		if ( !node->MarkModified() )
		{
			return false;
		}
	}

	return true;
}

