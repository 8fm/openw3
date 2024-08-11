#include "build.h"
#include "behTreeSailorFollowPath.h"
#include "../../common/game/behTreeInstance.h"
#include "../../common/game/BoatComponent.h"
#include "../../common/engine/tagManager.h"
#include "../../common/engine/pathComponent.h"


//////////////////////////////////////////////////////////////////////////
// CFollowPathUtils
Bool CFollowPathUtils::ComputeTargetAndHeading( const Vector &actorPos, CName pathName, Bool startFromBeginning, Bool upThePath, THandle< CPathComponent > & pathComponentHandle, Vector & outStartingPoint, Float & outHeading )
{
	CPathComponent* pathComponent = pathComponentHandle.Get();
	if ( !pathComponent )
	{
		CNode* node = GGame->GetActiveWorld()->GetTagManager()->GetTaggedNode( pathName );
		if ( !node )
		{
			// invalid tag
			return false;
		}
		if ( node->IsA< CPathComponent >() )
		{
			pathComponent = static_cast< CPathComponent* >( node );
		}
		else
		{
			CEntity* entity = Cast< CEntity >( node );
			if ( entity )
			{
				pathComponent = entity->FindComponent< CPathComponent >();
			}
		}

		if ( !pathComponent )
		{
			// no path component in given node
			return false;
		}
		pathComponentHandle = pathComponent;
	}

	SMultiCurve& curve = pathComponent->GetCurve();

	if ( curve.Size() == 0 )
	{
		return false;
	}

	Vector startingPoint;
	Bool hasNextPoint = false;
	Vector2 heading;

	if ( startFromBeginning )
	{
		if ( upThePath )
		{
			curve.GetAbsoluteControlPointPosition( 0, startingPoint );
			if ( curve.Size() > 1 )
			{
				Vector worldPoint;
				curve.GetAbsoluteControlPointPosition( 1, worldPoint );

				hasNextPoint = true;
				heading = worldPoint.AsVector2() - startingPoint.AsVector2();
			}
		}
		else
		{
			curve.GetAbsoluteControlPointPosition( curve.Size() - 1, startingPoint );
			if ( curve.Size() > 1 )
			{
				Vector worldPoint;
				curve.GetAbsoluteControlPointPosition( curve.Size() - 2, worldPoint );

				hasNextPoint = true;
				heading = worldPoint.AsVector2() - startingPoint.AsVector2();
			}
		}
	}
	else
	{
		Int32 edgeIdx;
		Float edgeAlpha;
		Bool isEndpoint = false;

		Vector closestPoint = pathComponent->GetClosestPointOnPath( actorPos, edgeIdx, edgeAlpha );
		Vector nextPoint = pathComponent->GetNextPointOnPath( edgeIdx, edgeAlpha, upThePath ? 0.1f : -0.1f, isEndpoint );
		if ( isEndpoint )
		{
			// move backward if
			Vector prevPoint =  pathComponent->GetNextPointOnPath( edgeIdx, edgeAlpha, upThePath ? -0.1f : 0.1f, isEndpoint );
			heading = closestPoint.AsVector2() - prevPoint.AsVector2();
		}
		else
		{
			heading = nextPoint.AsVector2() - closestPoint.AsVector2();
		}

		startingPoint = closestPoint.AsVector3();
	}
	outStartingPoint = startingPoint;
	if ( hasNextPoint )
	{
		outHeading = EulerAngles::YawFromXY( heading.X, heading.Y );
	}
	else
	{
		outHeading = 0.f;
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////
// CBehTreeNodeAtomicSailorMoveToPathDefinition
IBehTreeNodeInstance* CBehTreeNodeAtomicSailorMoveToPathDefinition::SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const
{
    return new Instance( *this, owner, context, parent );
}

//////////////////////////////////////////////////////////////////////////
// CBehTreeNodeAtomicSailorMoveToPathInstance
CBehTreeNodeAtomicSailorMoveToPathInstance::CBehTreeNodeAtomicSailorMoveToPathInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
        : CBehTreeNodeAtomicActionInstance( def, owner, context, parent ) 
		, m_pathTag( def.m_pathTag.GetVal( context ) )
		, m_pathComponent()
		, m_upThePath( def.m_upThePath.GetVal( context ) )
		, m_startFromBeginning( def.m_startFromBeginning.GetVal( context ) )
		, m_boatTag( def.m_boatTag.GetVal( context ) )
		, m_boatComponent()
		, m_pathReached( false )
{
	SBehTreeEvenListeningData e;
	e.m_eventName = CNAME( BoatReachedEndOfPath );
	e.m_eventType = SBehTreeEvenListeningData::TYPE_GAMEPLAY;
	context.AddEventListener( e, this );
}
void CBehTreeNodeAtomicSailorMoveToPathInstance::OnDestruction()
{
	SBehTreeEvenListeningData e;
	e.m_eventName = CNAME( BoatReachedEndOfPath );
	e.m_eventType = SBehTreeEvenListeningData::TYPE_GAMEPLAY;
	m_owner->RemoveEventListener( e, this );

	Super::OnDestruction();
}

Bool CBehTreeNodeAtomicSailorMoveToPathInstance::OnListenedEvent( CBehTreeEvent& e )
{
	if ( e.m_eventName == CNAME( BoatReachedEndOfPath ) )
	{
		m_pathReached = true;
	}
	return false;
}

Bool CBehTreeNodeAtomicSailorMoveToPathInstance::UpdateTarget()
{
	// [ Step ] Get the boat
	CEntity* boatEntity = (CEntity*)GGame->GetActiveWorld()->GetTagManager()->GetTaggedNode( m_boatTag );
    if( boatEntity == nullptr)
    {
        return false;
    }
	CBoatComponent* boatComponent = boatEntity->FindComponent< CBoatComponent >( );
    if( boatComponent == nullptr )
    {
        return false;
    }
	m_boatComponent = boatComponent;

	// [ Step ] Compute starting point of the path
	Vector startingPoint;
	Float startingPointHeading;
	if ( CFollowPathUtils::ComputeTargetAndHeading( m_owner->GetActor()->GetWorldPositionRef(), m_pathTag, m_startFromBeginning, m_upThePath, m_pathComponent, startingPoint, startingPointHeading ) == false )
	{
		return false;
	}
  
	// [ Step ] Order Boat to go to that position
	Matrix boatLocalToWorld;
    boatComponent->GetLocalToWorld( boatLocalToWorld );

    Vector targetHeading = boatLocalToWorld.GetAxisY();

    // Get destination and end heading
    SMultiCurve* multiCurve = nullptr;
    multiCurve = &m_pathComponent.Get()->GetCurve();
    if( multiCurve != nullptr )
    {
        Matrix ltw = Matrix::IDENTITY;
        if( multiCurve->GetParent() != nullptr )
            multiCurve->GetParent()->GetLocalToWorld( ltw );

        multiCurve->GetPosition( 0.0f, startingPoint );
        multiCurve->CalculateTangentFromCurveDirection( 0.0f, targetHeading );
        
        // Transform to global space
        startingPoint = ltw.TransformPoint( startingPoint );

        ltw.SetTranslation(Vector::ZEROS);
        targetHeading = ltw.TransformPoint( targetHeading );
    }

    // Trigger path following
    boatComponent->PathFindingMoveToLocation( startingPoint, targetHeading );
	return true;
}

Bool CBehTreeNodeAtomicSailorMoveToPathInstance::Activate()
{
	if ( UpdateTarget() == false )
	{
		DebugNotifyActivationFail();
		return false;
	}

	m_pathReached = false;
	return CBehTreeNodeAtomicActionInstance::Activate();
}

void CBehTreeNodeAtomicSailorMoveToPathInstance::Update()
{
	if ( m_startFromBeginning == false )
	{
		UpdateTarget();
	}
	CBoatComponent* boatComponent = m_boatComponent.Get();
	if ( boatComponent == nullptr )
	{
		Complete( BTTO_FAILED );
		return;
	}
	if ( m_pathReached )
	{
		Complete( BTTO_SUCCESS );
		return;
	}
	CBehTreeNodeAtomicActionInstance::Update();
}



//////////////////////////////////////////////////////////////////////////
// CBehTreeNodeAtomicSailorFollowPathDefinition
IBehTreeNodeInstance* CBehTreeNodeAtomicSailorFollowPathDefinition::SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const
{
    return new Instance( *this, owner, context, parent );
}

//////////////////////////////////////////////////////////////////////////
// CBehTreeNodeAtomicSailorFollowPathInstance
CBehTreeNodeAtomicSailorFollowPathInstance::CBehTreeNodeAtomicSailorFollowPathInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
    : CBehTreeNodeAtomicActionInstance( def, owner, context, parent ) 
	, m_pathTag( def.m_pathTag.GetVal( context ) )
	, m_pathComponent()
	, m_upThePath( def.m_upThePath.GetVal( context ) )
	, m_startFromBeginning( def.m_startFromBeginning.GetVal( context ) )
	, m_boatTag( def.m_boatTag.GetVal( context ) )
	, m_boatComponent()
	, m_pathReached( false )
{
	SBehTreeEvenListeningData e;
	e.m_eventName = CNAME( BoatReachedEndOfPath );
	e.m_eventType = SBehTreeEvenListeningData::TYPE_GAMEPLAY;
	context.AddEventListener( e, this );
}
void CBehTreeNodeAtomicSailorFollowPathInstance::OnDestruction()
{
	SBehTreeEvenListeningData e;
	e.m_eventName = CNAME( BoatReachedEndOfPath );
	e.m_eventType = SBehTreeEvenListeningData::TYPE_GAMEPLAY;
	m_owner->RemoveEventListener( e, this );

	Super::OnDestruction();
}

Bool CBehTreeNodeAtomicSailorFollowPathInstance::OnListenedEvent( CBehTreeEvent& e )
{
	if ( e.m_eventName == CNAME( BoatReachedEndOfPath ) )
	{
		m_pathReached = true;
	}
	return false;
}

Bool CBehTreeNodeAtomicSailorFollowPathInstance::Activate()
{
	// [ Step ] Get the boat
	CEntity* boatEntity = (CEntity*)GGame->GetActiveWorld()->GetTagManager()->GetTaggedNode( m_boatTag );
    if( boatEntity == nullptr)
    {
		DebugNotifyActivationFail();
        return false;
    }

	CBoatComponent* boatComponent = boatEntity->FindComponent< CBoatComponent >( );
    if( boatComponent == nullptr )
    {
		DebugNotifyActivationFail();
        return false;
    }

	m_boatComponent = boatComponent;

	// [ Step ] Compute starting point of the path
	Vector startingPoint;
	Float startingPointHeading;
	if ( CFollowPathUtils::ComputeTargetAndHeading( m_owner->GetActor()->GetWorldPositionRef(), m_pathTag, m_startFromBeginning, m_upThePath, m_pathComponent, startingPoint, startingPointHeading ) == false )
	{
		DebugNotifyActivationFail();
		return false;
	}
  
	// [ Step ] Order Boat to follow the path
	SMultiCurve* multiCurve = nullptr;
    multiCurve = &m_pathComponent.Get()->GetCurve();
        
	boatComponent->PathFindingFollowCurve( multiCurve );
	
	m_pathReached = false;
	return CBehTreeNodeAtomicActionInstance::Activate();
}

void CBehTreeNodeAtomicSailorFollowPathInstance::Update()
{
	CBoatComponent* boatComponent = m_boatComponent.Get();
	if ( boatComponent == nullptr )
	{
		Complete( BTTO_FAILED );
		return;
	}
	if ( m_pathReached )
	{
		Complete( BTTO_SUCCESS );
		return;
	}
	CBehTreeNodeAtomicActionInstance::Update();
}
