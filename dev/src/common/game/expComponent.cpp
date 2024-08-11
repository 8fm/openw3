
#include "build.h"
#include "expComponent.h"
#include "expCooking.h"
#include "expManager.h"
#include "gameWorld.h"
#include "../physics/physicsWorld.h"
#include "../physics/physicsWorldUtils.h"
#include "../engine/renderFrame.h"
#include "../engine/pathlibCookerData.h"

IMPLEMENT_ENGINE_CLASS( CExplorationComponent );

CExplorationComponent::CExplorationComponent()
	: m_start( -0.5f, 0.f, 0.f )
	, m_end( 0.5f, 0.f, 0.f )
	, m_explorationId( ET_Jump )
	, m_internalExploration( false )
{

}

void CExplorationComponent::OnAttached( CWorld* world )
{
	TBaseClass::OnAttached( world );

	PC_SCOPE_PIX( CExplorationComponent_OnAttached );
	
	world->GetEditorFragmentsFilter().RegisterEditorFragment( this, SHOW_Exploration );

	CGameWorld* gameWorld = Cast< CGameWorld > ( world );
	if ( gameWorld && !m_internalExploration )
	{
		ASSERT( gameWorld->GetExpManager() );
		gameWorld->GetExpManager()->AddExplorationByComponent( this );
	}
}

void CExplorationComponent::OnDetached( CWorld* world )
{
	CGameWorld* gameWorld = Cast< CGameWorld > ( world );
	if ( gameWorld && !m_internalExploration && gameWorld->GetExpManager() )
	{
		gameWorld->GetExpManager()->RemoveExplorationByComponent( this );
	}

	world->GetEditorFragmentsFilter().UnregisterEditorFragment( this, SHOW_Exploration );

	TBaseClass::OnDetached( world );
}

void CExplorationComponent::OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flags )
{
	TBaseClass::OnGenerateEditorFragments( frame, flags );

	if ( flags == SHOW_Exploration )
	{
		Vector p1, p2;
		GetEdgeWS( p1, p2 );

		frame->AddDebugFatLine( p1, p2, Color::LIGHT_MAGENTA, 0.005f );
		frame->AddDebugSphere( p1, 0.02f, Matrix::IDENTITY, Color::BLACK );
		frame->AddDebugSphere( p2, 0.02f, Matrix::IDENTITY, Color::BLACK );
		
		Vector n;
		GetNormal( n );

		Vector mp = ( p1 + p2 ) / 2.f;
		frame->AddDebugLine( mp, mp + n, Color::GREEN, true );

		CPhysicsWorld* physicsWorld = nullptr;
		if ( ( GetId() == ET_Ledge || GetId() == ET_Fence || GetId() == ET_Fence_OneSided ) &&
			 GetWorld() && GetWorld()->GetPhysicsWorld( physicsWorld ) )
		{
			for(Float i = 0.0f; i < 1.0f; i += 0.1f)
			{
				CPhysicsEngine::CollisionMask include = GPhysicEngine->GetCollisionTypeBit( CNAME( Static ) ) | GPhysicEngine->GetCollisionTypeBit( CNAME( Terrain ) ) | GPhysicEngine->GetCollisionTypeBit( CNAME( Destructible ) ) | GPhysicEngine->GetCollisionTypeBit( CNAME( RigidBody ) ) ;
				CPhysicsEngine::CollisionMask exclude = GPhysicEngine->GetCollisionTypeBit( CNAME( Debris ) );
				Vector startWS = p1 + (p2 - p1) * i + n * 1.0f;
				Vector startWSLifted = startWS + Vector( 0.0f, 0.0f, 1.0f );
				Vector endWS = startWS + Vector( 0.0f, 0.0f, -5.0f );
				SPhysicsContactInfo contactInfo;
				int expSize = 2;
				if( physicsWorld->RayCastWithSingleResult( startWSLifted, endWS, include, exclude, contactInfo ) == TRV_Hit )
				{
					Float height = Max( 0.0f, startWS.Z - contactInfo.m_position.Z );
					if (height < 0.70f)
					{
						expSize = 0;
					}
					if (height < 1.50f)
					{
						expSize = 1;
					}
					frame->AddDebugLine( startWS, contactInfo.m_position, expSize == 0? Color(255,100,100,150) : ( expSize == 1? Color(255,200,100,150) : Color(100,255,100,150) ), true, true );
				}
				else
				{
					frame->AddDebugLine( startWS, endWS, Color(25,150,25,75), true, true );
				}
			}
		}

		if ( GetId() == ET_Ladder )
		{
			Vector startWS = p1;
			Vector endWS = p2;
			startWS += n * 0.5f;
			endWS += n * 0.5f;
			frame->AddDebugLine( startWS, endWS, Color(25,70,255,75), true, true );
		}

		//Matrix mat;
		//mat.BuildFromDirectionVector( n );

		//frame->AddDebugAxis( mp, mat, 0.5f );

		//const Matrix& l2w = GetLocalToWorld();
		//frame->AddDebugSphere( l2w.GetTranslation(), 0.55f, Matrix::IDENTITY, expColor );
	}
}

void CExplorationComponent::GetMatWS( Matrix& mat ) const
{
	mat = GetLocalToWorld();
}

void CExplorationComponent::GetParentMatWS( Matrix& mat ) const
{
	mat = GetLocalToWorld();
	if ( GetParent() )
	{
		if ( CNode* parentAsNode = Cast< CNode >( GetParent() ) )
		{
			mat = parentAsNode->GetLocalToWorld();
		}
	}
}

void CExplorationComponent::GetEdgeWS( Vector& p1, Vector& p2 ) const
{
	const Matrix& mat = GetLocalToWorld();
	p1 = mat.TransformPoint( m_start );
	p2 = mat.TransformPoint( m_end );
}

void CExplorationComponent::GetNormal( Vector& n ) const
{
	n = GetLocalToWorld().GetRow( 1 ).Normalized3();
}

Int32 CExplorationComponent::GetId() const
{
	return (Int32)m_explorationId;
}

CObject* CExplorationComponent::GetObjectForEvents() const
{
	if ( !m_componentForEvents.Empty() )
	{
		CComponent* comp = GetEntity()->FindComponent( m_componentForEvents );
		if ( comp )
		{
			return comp;
		}
	}

	return GetEntity();
}

#ifndef NO_EDITOR
void CExplorationComponent::OnNavigationCook( CWorld* world, CNavigationCookingContext* context )
{
	context->Get< CExplorationCookingContext >()->OnExploration( this );
}
#endif // NO_EDITOR

/*Bool CExplorationComponent::FastAreaTest( const CEntity* testedEntity ) const
{
	const Box box = testedEntity->CalcBoundingBox();
	const CEntity* entity = GetEntity();

	Bool hasAreas = false;

	for ( ComponentIterator< CAreaComponent > it( entity ); it; ++it )
	{
		hasAreas = true;

		const CAreaComponent* c = *it;

		if ( c->TestBoxOverlap( box ) )
		{
			return true;
		}
	}

	return !hasAreas;
}*/
