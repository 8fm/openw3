/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behaviorGraphDampValueNode.h"
#include "behaviorGraphInstance.h"
#include "behaviorGraphSocket.h"
#include "../core/instanceDataLayoutCompiler.h"
#include "../core/mathUtils.h"
#include "../engine/graphConnectionRebuilder.h"
#include "behaviorProfiler.h"

RED_DEFINE_STATIC_NAME( SmoothTime )

// TEMP E3
class DefaultDiffPolicy
{
public:
	static Float CalcDiff( Float a, Float b )
	{
		return a - b;
	}
};

class AngularDiffPolicy
{
public:
	static Float CalcDiff( Float a, Float b )
	{
		return EulerAngles::AngleDistance( b, a );
	}
};

class CyclicDiffPolicy
{
public:
	static Float CalcDiff( Float a, Float b )
	{
		return MathUtils::CyclicDistance( b, a );
	}
};

template< typename DiffPolicy >
class OldSpring : public Red::System::NonCopyable
{
private:
	const Float m_springFactor;
	const Float	m_dampingFactor;

public:
	OldSpring( Float factor )
		: m_springFactor( factor )
		, m_dampingFactor( 2.f * MSqrt( factor ) )
	{};

	void Update( Float& curr, Float& vel, Float dest, Float dt )
	{
		const Float step = 0.005f;

		if ( dt > step )
		{
			// This is slow...
			Int32 count = (Int32)(dt/step);

			dt -= count * step;

			for ( Int32 i=0; i<count; ++i )
			{
				InternalUpdate( curr, vel, dest, step );
			}

			ASSERT( dt >= 0.f );
			InternalUpdate( curr, vel, dest, dt );
		}
		else
		{
			InternalUpdate( curr, vel, dest, dt );
		}
	}

	RED_INLINE void InternalUpdate( Float& curr, Float& vel, Float dest, Float dt )
	{
		const Float diff = DiffPolicy::CalcDiff( curr, dest );
		const Float springAccel = -(  diff * m_springFactor ) - vel * m_dampingFactor;

		vel += springAccel * dt;
		curr += vel * dt;
	}
};
// end of TEMP E3

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphDampValueNode );

CBehaviorGraphDampValueNode::CBehaviorGraphDampValueNode()
	: m_increaseSpeed( 1.0f )
	, m_decreaseSpeed( 1.0f )
	, m_startFromDefault( false )
	, m_defaultValue( 1.f )
{
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphDampValueNode::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );

	CreateSocket( CBehaviorGraphVariableInputSocketSpawnInfo( CNAME( Input ) ) );
	CreateSocket( CBehaviorGraphVariableOutputSocketSpawnInfo( CNAME( Output ) ) );

	CreateSocket( CBehaviorGraphVariableInputSocketSpawnInfo( CNAME( Default_val ), false ) );
	CreateSocket( CBehaviorGraphVariableInputSocketSpawnInfo( CNAME( Inc_speed ), false ) );
	CreateSocket( CBehaviorGraphVariableInputSocketSpawnInfo( CNAME( Dec_speed ), false ) );
}

#endif

void CBehaviorGraphDampValueNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	BEH_NODE_UPDATE( Damp );

	TBaseClass::OnUpdate( context, instance, timeDelta );

	Float increaseSpeed, decreaseSpeed;
	Float dampedValue = instance[ i_value ];

	if ( m_cachedIncSpeedNode )
	{
		m_cachedIncSpeedNode->Update( context, instance, timeDelta );
		increaseSpeed = m_cachedIncSpeedNode->GetValue( instance );
	}
	else
	{
		increaseSpeed = m_increaseSpeed;
	}

	if ( m_cachedDecSpeedNode )
	{
		m_cachedDecSpeedNode->Update( context, instance, timeDelta );
		decreaseSpeed = m_cachedDecSpeedNode->GetValue( instance );
	}
	else
	{
		decreaseSpeed = m_decreaseSpeed;
	}

	Float currValue = m_cachedInputNode ? m_cachedInputNode->GetValue( instance ) : 0.0f;
	Float diff = currValue - dampedValue;

	if ( diff == 0 )
		return;

	Float changeSpeed = increaseSpeed;
	Float dir = 1.0f;
	Float lowerBound = -NumericLimits<Float>::Max();
	Float upperBound = currValue;
	if ( diff < 0.0f )
	{
		changeSpeed = decreaseSpeed;
		dir = -1.0f;
		lowerBound = currValue;
		upperBound = NumericLimits<Float>::Max();
	}

	if ( m_absolute )
	{
		const Float absDir = Sgn< Float >( dampedValue ) * dir; 
		changeSpeed = (absDir < 0.f) ? decreaseSpeed : increaseSpeed;
	}

	if ( changeSpeed == 0.f )
	{
		// changeSpeed == Zero means we have to change the value immediately to the currValue.
		// But! We can't do that when m_absolute is true and we're crossing zero,
		// because the User might want to use different speed after crossing 0.
		const Bool doWeChangeTheSign = Sgn< Float >( currValue ) * Sgn< Float >( dampedValue ) < 0.f;
		dampedValue = ( m_absolute && doWeChangeTheSign && increaseSpeed != decreaseSpeed ) ? 0.f : currValue;
	}
	else
	{
		dampedValue += dir * changeSpeed * timeDelta; 
		dampedValue = Clamp( dampedValue, lowerBound, upperBound );
	}

	instance[ i_value ] = dampedValue;
}

void CBehaviorGraphDampValueNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnActivated( instance );

	if( m_cachedDefaultValNode )
	{
		m_cachedDefaultValNode->Activate( instance );
	}

	if ( m_cachedIncSpeedNode )
	{
		m_cachedIncSpeedNode->Activate( instance );
	}

	if ( m_cachedDecSpeedNode )
	{
		m_cachedDecSpeedNode->Activate( instance );
	}

	// Reset value
	if ( !m_startFromDefault )
	{
		instance[ i_value ] = m_cachedInputNode ? m_cachedInputNode->GetValue( instance ) : 0.0f;
	}
	else
	{
		instance[ i_value ] = m_cachedDefaultValNode ? m_cachedDefaultValNode->GetValue( instance ) : m_defaultValue;
	}
}

void CBehaviorGraphDampValueNode::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnDeactivated( instance );

	if( m_cachedDefaultValNode )
	{
		m_cachedDefaultValNode->Deactivate( instance );
	}

	if ( m_cachedIncSpeedNode )
	{
		m_cachedIncSpeedNode->Deactivate( instance );
	}

	if ( m_cachedDecSpeedNode )
	{
		m_cachedDecSpeedNode->Deactivate( instance );
	}
}

void CBehaviorGraphDampValueNode::ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const
{
	TBaseClass::ProcessActivationAlpha( instance, alpha );

	if( m_cachedDefaultValNode )
	{
		m_cachedDefaultValNode->ProcessActivationAlpha( instance, alpha );
	}

	if ( m_cachedIncSpeedNode )
	{
		m_cachedIncSpeedNode->ProcessActivationAlpha( instance, alpha );
	}

	if ( m_cachedDecSpeedNode )
	{
		m_cachedDecSpeedNode->ProcessActivationAlpha( instance, alpha );
	}
}

void CBehaviorGraphDampValueNode::CacheConnections()
{
	// Pass to base class
	TBaseClass::CacheConnections();

	// Cache connections
	m_cachedDefaultValNode = CacheValueBlock( TXT("Default value") );
	m_cachedIncSpeedNode = CacheValueBlock( TXT("Inc speed") );
	m_cachedDecSpeedNode = CacheValueBlock( TXT("Dec speed") );
}

///////////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphDampAngularValueNode );

CBehaviorGraphDampAngularValueNode::CBehaviorGraphDampAngularValueNode()
    : m_speed( 1.f ),
    m_isDegree(  false )
{

}

void CBehaviorGraphDampAngularValueNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
    BEH_NODE_UPDATE( DampAngular );

    TBaseClass::OnUpdate( context, instance, timeDelta );

    // Node value
    Float& value = instance[ i_value ];

    // Input value
    Float inputValue = m_cachedInputNode ? m_cachedInputNode->GetValue( instance ) : 0.0f;

    Float dist = EulerAngles::AngleDistance( m_isDegree ? value : value * 180,  m_isDegree ? inputValue : inputValue * 180);
    dist = m_isDegree ? dist : dist / 180.0f;

    Float deltaValue = ( m_speed * timeDelta );

    if( value == inputValue || Abs( dist ) < deltaValue - 0.00001f )
    {
        value = inputValue;
        return;
    }

    if( dist < 0 )
    {
        value = value - deltaValue;
    }
    else
    {
        value = value + deltaValue;
    }

    float limit =  m_isDegree ? 180.0f : 1.0f;

    if( value < -limit ) 
    {
        value += ( limit * 2.0f );
    }
    else if( value > limit )
    {
        value -= ( limit * 2.0f );
    }
}

///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphDampAngularValueNodeDiff );

CBehaviorGraphDampAngularValueNodeDiff::CBehaviorGraphDampAngularValueNodeDiff()
    : m_speed( 1.f ),
    m_isDegree(  false )
{

}

void CBehaviorGraphDampAngularValueNodeDiff::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
    BEH_NODE_UPDATE( DampAngularDiff );

    TBaseClass::OnUpdate( context, instance, timeDelta );

    // Node value
    Float& value = instance[ i_value ];

    // Input value
    Float inputValue= m_cachedInputNode ? m_cachedInputNode->GetValue( instance ) : 0.0f;

    Float dist = EulerAngles::AngleDistance( m_isDegree ? value : value * 180,  m_isDegree ? inputValue : inputValue * 180);
    dist = m_isDegree ? dist : dist / 180.0f;

    Float absDist = Abs( dist );

    Float deltaValue = ( m_speed * timeDelta * absDist );

    if( value == inputValue || absDist < deltaValue - 0.00001f)
    {
        value = inputValue;
        return;
    }

    if( dist < 0 )
    {
        value = value - deltaValue;
    }
    else
    {
        value = value + deltaValue;
    }

    float limit =  m_isDegree ? 180.0f : 1.0f;

    if( value < -limit ) 
    {
        value += ( limit * 2.0f );
    }
    else if( value > limit )
    {
        value -= ( limit * 2.0f );
    }
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphSpringDampValueNode );

CBehaviorGraphSpringDampValueNode::CBehaviorGraphSpringDampValueNode()
	: m_factor( 10.f )
	, m_scale( 1.f )
{
}

void CBehaviorGraphSpringDampValueNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_vel;
}

void CBehaviorGraphSpringDampValueNode::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnInitInstance( instance );

	InternalReset( instance );
}

void CBehaviorGraphSpringDampValueNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	BEH_NODE_UPDATE( SpringDampValue );

	TBaseClass::OnUpdate( context, instance, timeDelta );

	if ( m_cachedInputNode )
	{
		//TFloatCriticalDamp damper( m_factor );
		OldSpring< DefaultDiffPolicy > damper( m_factor );
		damper.Update( instance[ i_value ], instance[ i_vel ], m_scale * m_cachedInputNode->GetValue( instance ), timeDelta );
	}
}

void CBehaviorGraphSpringDampValueNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnActivated( instance );

	if( m_forceInputValueOnActivate )
	{
		instance[ i_value ] = m_cachedInputNode ? m_cachedInputNode->GetValue( instance ) : 0.0f;
	}
}

void CBehaviorGraphSpringDampValueNode::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	InternalReset( instance );

	TBaseClass::OnDeactivated( instance );
}

void CBehaviorGraphSpringDampValueNode::OnReset( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnReset( instance );

	InternalReset( instance );
}

void CBehaviorGraphSpringDampValueNode::InternalReset( CBehaviorGraphInstance& instance ) const
{
	instance[ i_vel ] = 0.f;
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphOptSpringDampValueNode );

CBehaviorGraphOptSpringDampValueNode::CBehaviorGraphOptSpringDampValueNode()
	: m_smoothTime( 1.f )
	, m_scale( 1.f )
	, m_maxSpeed( 0.f )
	, m_maxDiff( 0.f )
	, m_defaultValue( 0.f )
	, m_forceDefaultValueOnActivate( false )
{
}

void CBehaviorGraphOptSpringDampValueNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_vel;
}

void CBehaviorGraphOptSpringDampValueNode::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnInitInstance( instance );

	InternalReset( instance );
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphOptSpringDampValueNode::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );

	TBaseClass::OnRebuildSockets();

	CreateSocket( CBehaviorGraphVariableInputSocketSpawnInfo( CNAME( SmoothTime ), false ) );
}

#endif

void CBehaviorGraphOptSpringDampValueNode::CacheConnections()
{
	TBaseClass::CacheConnections();

	m_cachedSmoothTimeNode = CacheValueBlock( CNAME( SmoothTime ) );
}

void CBehaviorGraphOptSpringDampValueNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	BEH_NODE_UPDATE( SpringDampValueOpt );

	TBaseClass::OnUpdate( context, instance, timeDelta );

	if ( m_cachedSmoothTimeNode )
	{
		m_cachedSmoothTimeNode->Update( context, instance, timeDelta );
	}

	if ( m_cachedInputNode )
	{
		const Float inputValue = m_scale * m_cachedInputNode->GetValue( instance );
		Float& value = instance[ i_value ];
		Float& vel = instance[ i_vel ];

		const Bool useMaxSpeed = m_maxSpeed > 0.f;
		const Bool useMaxDiff = m_maxDiff > 0.f;

		const Float smoothTime = m_cachedSmoothTimeNode ? m_cachedSmoothTimeNode->GetValue( instance ) : m_smoothTime;

		if ( useMaxSpeed && useMaxDiff )
		{
			BehaviorUtils::SmoothCriticalDamp_MVMD( value, vel, timeDelta, inputValue, m_maxSpeed, m_maxDiff, smoothTime );
		}
		else if ( useMaxSpeed )
		{
			BehaviorUtils::SmoothCriticalDamp_MV( value, vel, timeDelta, inputValue, m_maxSpeed, smoothTime );
		}
		else if ( useMaxDiff )
		{
			BehaviorUtils::SmoothCriticalDamp_MD( value, vel, timeDelta, inputValue, m_maxDiff, smoothTime );
		}
		else
		{
			BehaviorUtils::SmoothCriticalDamp( value, vel, timeDelta, inputValue, smoothTime );
		}
	}
}

void CBehaviorGraphOptSpringDampValueNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnActivated( instance );

	if ( m_forceInputValueOnActivate )
	{
		instance[ i_value ] = m_cachedInputNode ? m_cachedInputNode->GetValue( instance ) : 0.0f;
	}
	else if ( m_forceDefaultValueOnActivate )
	{
		instance[ i_value ] = m_defaultValue;
	}

	if ( m_cachedSmoothTimeNode )
	{
		m_cachedSmoothTimeNode->Activate( instance );
	}
}

void CBehaviorGraphOptSpringDampValueNode::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	if ( m_cachedSmoothTimeNode )
	{
		m_cachedSmoothTimeNode->Deactivate( instance );
	}

	InternalReset( instance );

	TBaseClass::OnDeactivated( instance );
}

void CBehaviorGraphOptSpringDampValueNode::ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const
{
	TBaseClass::ProcessActivationAlpha( instance, alpha );

	if ( m_cachedSmoothTimeNode )
	{
		m_cachedSmoothTimeNode->ProcessActivationAlpha( instance, alpha );
	}
}

void CBehaviorGraphOptSpringDampValueNode::OnReset( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnReset( instance );

	InternalReset( instance );
}

void CBehaviorGraphOptSpringDampValueNode::InternalReset( CBehaviorGraphInstance& instance ) const
{
	instance[ i_vel ] = 0.f;
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphSpringAngularDampValueNode );

CBehaviorGraphSpringAngularDampValueNode::CBehaviorGraphSpringAngularDampValueNode()
	: CBehaviorGraphSpringDampValueNode()
	, m_isDegree(  false )
{
}

void CBehaviorGraphSpringAngularDampValueNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	BEH_NODE_UPDATE( SpringAngularDampValue );

	CBehaviorGraphValueBaseNode::OnUpdate( context, instance, timeDelta );

	if ( m_cachedInputNode )
	{
		Float& val = instance[ i_value ];
		Float& vel = instance[ i_vel ];
		const Float dest = m_scale * m_cachedInputNode->GetValue( instance );

		if( m_isDegree )
		{
			TFloatAngleCriticalDamp damper( m_factor );
			//OldSpring< AngularDiffPolicy > damper( m_factor );
			damper.Update( val, vel, dest, timeDelta );
		}
		else
		{
			TFloatCyclicCriticalDamp damper( m_factor );
			//OldSpring< CyclicDiffPolicy > damper( m_factor );
			damper.Update( val, vel, dest, timeDelta );
		}
	}
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_RTTI_ENUM( EBehaviorCustomDampType );
IMPLEMENT_ENGINE_CLASS( CBehaviorGraphCustomDampValueNode );

CBehaviorGraphCustomDampValueNode::CBehaviorGraphCustomDampValueNode()
	: m_type( BGCDT_DirectionalAcc )
	, m_directionalAcc_MaxAccDiffFromZero( 50.f )
	, m_directionalAcc_MaxAccDiffToZero( 200.f )
	, m_filterLowPass_RC( 0.5f )
{

}

void CBehaviorGraphCustomDampValueNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_varA;
	compiler << i_varB;
}

void CBehaviorGraphCustomDampValueNode::ResetAllVariables( CBehaviorGraphInstance& instance ) const
{
	instance[ i_varA ] = 0.f;
	instance[ i_varB ] = 0.f;
}

void CBehaviorGraphCustomDampValueNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	BEH_NODE_UPDATE( CustomDampValue );

	TBaseClass::OnUpdate( context, instance, timeDelta );

	const Float inputVal = m_cachedInputNode ? m_cachedInputNode->GetValue( instance ) : 0.f;
	Float& value = instance[ i_value ];

	switch ( m_type )
	{
	case BGCDT_DirectionalAcc:
		{
			value = CalcDirectionalAcc( instance, inputVal, timeDelta );
			return;
		}

	case BGCDT_FilterLowPass:
		{
			value = CalcFilterLowPass( instance, inputVal, timeDelta );
			return;
		}

	default:
		ASSERT( 0 );
	}
}

Float CBehaviorGraphCustomDampValueNode::GetValue( CBehaviorGraphInstance& instance ) const
{
	const Float val = instance[ i_value ];
	return val; // Place for break point
}

namespace
{
	Float SafeDiv( Float a, Float b )
	{
		return MAbs( b ) > NumericLimits< Float >::Epsilon() ? a / b : 0.f; 
	}
}

Float CBehaviorGraphCustomDampValueNode::CalcDirectionalAcc( CBehaviorGraphInstance& instance, Float inputVal, Float timeDelta ) const
{
	Float prevVal = instance[ i_value ];

	Bool goToZero = false;
	if ( ( prevVal < 0.f && inputVal > prevVal ) || ( prevVal > 0.f && inputVal < prevVal ) )
	{
		goToZero = true;
	}

	//const Float accClamp = goToZero ? m_directionalAcc_MaxAccDiffToZero : m_directionalAcc_MaxAccDiffFromZero;
	const Float accClamp = m_directionalAcc_MaxAccDiffFromZero;

	const Float valDiff = inputVal - prevVal;
	const Float currVel = SafeDiv( valDiff, timeDelta );
	const Float currAcc = SafeDiv( currVel, timeDelta );

	const Float acc = Clamp( currAcc, -accClamp, accClamp );

	Float valueChange = acc * timeDelta * timeDelta;
	
	/*if ( accClamp > 0.f && goToZero && MAbs( valueChange ) > MAbs( prevVal ) )
	{
		const Float toZeroDist = MAbs( prevVal );
		const Float fromZeroDist = MAbs( prevVal + valueChange );
		const Float timeToRecalc = MSqrt( fromZeroDist / accClamp );

		timeDelta = timeToRecalc;
		//...
	}*/

	const Float finalValue = prevVal + valueChange;

	return finalValue;
}

Float CBehaviorGraphCustomDampValueNode::CalcFilterLowPass( CBehaviorGraphInstance& instance, Float inputVal, Float timeDelta ) const
{
	Float value = instance[ i_value ];

	if ( timeDelta > 0.f )
	{
		const Float alpha = timeDelta / ( m_filterLowPass_RC + timeDelta );
		value = alpha * inputVal + ( 1.f-alpha ) * value;
	}

	return value;
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

String CBehaviorGraphCustomDampValueNode::GetCaption() const 
{ 
	switch ( m_type )
	{
	case BGCDT_DirectionalAcc:
		return TXT("Directional acc damp"); 

	case BGCDT_FilterLowPass:
		return TXT("Filter low pass"); 

	default:
		ASSERT( 0 );
		return TXT("Custom damp"); 
	}
}

#endif

void CBehaviorGraphCustomDampValueNode::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnInitInstance( instance );

	ResetAllVariables( instance );
}

void CBehaviorGraphCustomDampValueNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnActivated( instance );

	ResetAllVariables( instance );
}

void CBehaviorGraphCustomDampValueNode::OnReset( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnReset( instance );

	ResetAllVariables( instance );
}
