#include "build.h"
#include "moveSteeringBehavior.h"

#include "movementCommandBuffer.h"
#include "movementGoal.h"
#include "moveSteeringNode.h"

#include "../core/factory.h"
#include "../core/instanceDataLayoutCompiler.h"


///////////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CMoveSteeringBehavior );

///////////////////////////////////////////////////////////////////////////////

CMoveSteeringBehavior::CMoveSteeringBehavior()
: m_root( NULL )
{
}

void CMoveSteeringBehavior::CalculateMovement( IMovementCommandBuffer& comm, InstanceBuffer& data, Float timeDelta, ISteeringBehaviorListener* listener ) const
{
	if ( listener )
	{
		listener->OnFrameStart( comm.GetGoal() );
	}

	if ( m_root )
	{
		PC_SCOPE( GSteeringBehavior );

		m_root->CalculateSteering( comm, data, timeDelta, listener );
	}
}

void CMoveSteeringBehavior::OnPostLoad()
{
	TBaseClass::OnPostLoad();
	CompileDataLayout();
}

void CMoveSteeringBehavior::OnStructureModified()
{
	MarkModified();
	CompileDataLayout();
}

void CMoveSteeringBehavior::CompileDataLayout()
{
	// compile the data layout
	InstanceDataLayoutCompiler compiler( m_dataLayout );
	if ( m_root )
	{
		m_root->OnBuildDataLayout( compiler );
		m_dataLayout.ChangeLayout( compiler );
	}
}

void CMoveSteeringBehavior::Activate( CMovingAgentComponent* owner, InstanceBuffer* data )
{
	if ( m_root )
	{
		m_root->OnGraphActivation( *owner, *data );
	}
}

void CMoveSteeringBehavior::Deactivate( CMovingAgentComponent* owner, InstanceBuffer* data )
{
	if ( m_root )
	{
		m_root->OnGraphDeactivation( *owner, *data );
	}
}

InstanceBuffer* CMoveSteeringBehavior::CreateRealtimeDataInstance( CMovingAgentComponent* owner ) 
{
	InstanceBuffer* data = m_dataLayout.CreateBuffer( owner, TXT( "CMoveSteeringBehaviorInstance" ) );

	if ( m_root )
	{
		m_root->OnInitData( *owner, *data );
	}

	return data;
}

void CMoveSteeringBehavior::ReleaseRealtimeDataInstance( CMovingAgentComponent* owner, InstanceBuffer* data )
{
	// initialize the data buffer
	if ( m_root )
	{
		m_root->OnDeinitData( *owner, *data );
	}
	data->Release();
}

void CMoveSteeringBehavior::GenerateDebugFragments( const CMovingAgentComponent& agent, CRenderFrame* frame ) const
{
	if ( m_root )
	{
		m_root->GenerateDebugFragments( agent, frame );
	}
}

///////////////////////////////////////////////////////////////////////////////

class CMoveSteeringBehaviorFactory : public IFactory
{
	DECLARE_ENGINE_CLASS( CMoveSteeringBehaviorFactory, IFactory, 0 );

public:
	CMoveSteeringBehaviorFactory()
	{
		m_resourceClass = ClassID< CMoveSteeringBehavior >();
	}

	virtual CResource* DoCreate( const FactoryOptions& options );
};

BEGIN_CLASS_RTTI( CMoveSteeringBehaviorFactory )
	PARENT_CLASS( IFactory )
END_CLASS_RTTI()

IMPLEMENT_ENGINE_CLASS( CMoveSteeringBehaviorFactory );

CResource* CMoveSteeringBehaviorFactory::DoCreate( const FactoryOptions& options )
{
	CMoveSteeringBehavior* steeringBehavior = ::CreateObject< CMoveSteeringBehavior >( options.m_parentObject );
	return steeringBehavior;
}
