/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "massSpringSolver.h"

Uint32 GConn		= 0;
Uint32 GSprNode	= 0;
Uint32 GTrNode	= 0;
Uint32 GSim		= 0;

static const Float G_SpringKS		= 0.0f;	//23
static const Float G_SpringKD		= 0.0f;	//0.2
static const Float G_Gravity		= 9.81f;	//9.81
static const Float G_NodeMass		= 0.03f;	//0.001
static const Float G_AirK			= 0.0001f;	//0.002

static const Uint32  G_RelaxIter		= 50;
static const Uint32  G_RelaxInnerIter	= 0;
static const Uint32  G_StepsPerSecond	= 500;

static const Uint32  G_AddTrackedNodes= 0; // first node is a tracked node by default

// ** ***************************
//
CMassSpringSolver::CMassSpringSolver		()
	: m_numRelaxIterations( G_RelaxIter )
	, m_numInnerRelaxIterations( G_RelaxInnerIter )
	, m_numIterations( 0 )
	, m_numInternalIterations( 0 )
	, m_stepsPerSecond( G_StepsPerSecond )
{
	m_relaxPerInnerTick		= (Float) m_numInnerRelaxIterations / (Float) m_stepsPerSecond;

	m_relaxInnerTicksFloat	= 0.f;
	m_relaxInnerTicksUint	= 0;;
}

// ** ***************************
//
CMassSpringSolver::~CMassSpringSolver		()
{
}

// ** ***************************
//
void	CMassSpringSolver::Initialize		( const TDynArray< Vector > & initNodes, const Vector & upDelta )
{
	ASSERT( initNodes.Size() > ( G_AddTrackedNodes + 1 ) );

	// Initialize solver parameters

	// Initialize simulation parameters
	InitModelParams();

	// Initialize positions of input nodes (to be used later, during positions interpolation)
    InitPositions( initNodes );

	// Reset model state
	m_model.connections.Clear();
	m_model.trackedNodes.Clear();
	m_model.nodes.Clear();

	// Initialize tracked nodes
	InitTrackedNodes( initNodes, upDelta );

	// Initialize spring nodes
	InitSpringNodes( initNodes );

	// Initialize timestamp
	m_lastTickTime = Red::System::Clock::GetInstance().GetTimer().GetTicks();
}

// ** ***************************
//
void	CMassSpringSolver::InitModelParams		()
{
	// Initialize simulation parameters
	m_model.airK	= G_AirK;
	m_model.sprKS	= G_SpringKS;
	m_model.sprKD	= G_SpringKD;
	m_model.trKS	= G_SpringKS;
	m_model.trKD	= G_SpringKD;
	m_model.g		= Vector( 0.f, 0.f, -G_Gravity, 0.f );
#if 0
	m_model.anchorIndex = 0; // Not used
#endif
	m_model.m		= G_NodeMass;
}

// ** ***************************
//
void    CMassSpringSolver::InitPositions           ( const TDynArray< Vector > & initNodes )
{
    m_prevPositions.Clear();

    for( Uint32 i = 0; i < initNodes.Size(); ++i )
    {
        m_prevPositions.PushBack( initNodes[ i ] );
    }
}

// ** ***************************
// Configuration description:   o - spring node;   * - tracked node;
// 
//  o  o  o  o  o  o  o  o-*  <- tr0
//
// only one tracked node in this configuration
void	CMassSpringSolver::InitTrackedNodes		( const TDynArray< Vector > & initNodes, const Vector & upDelta )
{
	TrackedNode tr0;

	tr0.x = initNodes[ 0 ];
	tr0.v = Vector( 0.f, 0.f, 0.f, 0.f );

	tr0.connection.ks	= m_model.trKS;
	tr0.connection.kd	= m_model.trKD;
	tr0.connection.r	= ( initNodes[ 0 ] - initNodes[ 1 ] ).Mag4();

	m_model.trackedNodes.PushBack( tr0 );

#if 0 // additional tracked nodes
	for( Uint32 i = 0; i < GAddTrackedNodes; ++i )
	{
		TrackedNode tr;

		tr.x = initNodes[ i + 1 ] + upDelta;
		tr.v = Vector( 0.f, 0.f, 0.f, 0.f );

		tr.connection.ks	= m_model.trKS;
		tr.connection.kd	= m_model.trKD;
		tr.connection.r		= upDelta.Mag4();

		m_model.trackedNodes.PushBack( tr );
	}
#endif
}

// ** ***************************
//
void	CMassSpringSolver::InitSpringNodes		( const TDynArray< Vector > & initNodes )
{
	PreInitSpringNodes	( initNodes );
	InitSprConnections	( initNodes );
	PostInitSpringNodes	();
}

// ** ***************************
//
void	CMassSpringSolver::PreInitSpringNodes	( const TDynArray< Vector > & initNodes )
{
	ASSERT( m_model.trackedNodes.Size() == 1 );

	for( Uint32 i = 1; i < initNodes.Size(); ++i )
	{
		SpringNode n;

		// Add main tracked node
		if( i == 1 )
		{
			n.trNodes.PushBack( &m_model.trackedNodes[ 0 ] );
		}

		// Initialize simulation parameters
		n.simulation.x	= initNodes[ i ];
		n.simulation.v	= Vector( 0.f, 0.f, 0.f, 0.f );
		n.simulation.F	= Vector( 0.f, 0.f, 0.f, 0.f );
		n.simulation.a	= Vector( 0.f, 0.f, 0.f, 0.f );
		n.simulation.m	= m_model.m;

#if 0 //no additional tracked nodes here
		// Add upper tracked node (if one exists)
		if( k < m_model.trackedNodes.Size() )
		{
			n.trNodes.PushBack( &m_model.trackedNodes[ k++ ] );
		}
#endif

		m_model.nodes.PushBack( n );
	}
}

// ** ***************************
//
void	CMassSpringSolver::InitSprConnections	( const TDynArray< Vector > & initNodes )
{
	// Initialize connections
	// connection[ i ] connects nodes node[ i ] and node[ i + 1 ]
	for( Uint32 i = 1; i < initNodes.Size() - 1; ++i )
	{
		const Vector & v0 = initNodes[ i ];
		const Vector & v1 = initNodes[ i + 1 ];

		Vector dv = v1 - v0;

		SpringConnection connection;

		connection.kd	= m_model.sprKD;
		connection.ks	= m_model.sprKS;
		connection.r	= dv.Mag4();

		m_model.connections.PushBack( connection );
	}
}

// ** ***************************
//
void	CMassSpringSolver::PostInitSpringNodes	()
{
	// Finish initialization of spring nodes - connections and neighbours
	for( Uint32 i = 0; i < m_model.nodes.Size(); ++i )
	{
		SpringNode & n = m_model.nodes[ i ];

		// Process next neighbour
		if( i < m_model.nodes.Size() - 1 )
		{
			n.connections.PushBack( &m_model.connections[ i ] );
			n.neighbours.PushBack( &m_model.nodes[ i + 1 ] );
		}

		// Process previous neighbour
		if( i > 0 )
		{		
			n.connections.PushBack( &m_model.connections[ i - 1 ] );
			n.neighbours.PushBack( &m_model.nodes[ i - 1 ] );
		}
	}
}

// ** ***************************
//
void	CMassSpringSolver::SetGravity		( const Vector & g )
{
	m_model.g = g;
}

// ** ***************************
//
void	CMassSpringSolver::SetNodeMass		( Float m )
{
	ASSERT( m_model.nodes.Size() > 0 );

	for( Uint32 i = 0; i < m_model.nodes.Size(); ++i )
	{
		SpringNode & n = m_model.nodes[ i ];

		n.simulation.m = m;
	}
}

#if 0
// ** ***************************
//
void	CMassSpringSolver::SetAnchor		( Uint32 anchor )
{
	ASSERT( anchor >= 0 && anchor < m_model.nodes.Size() );

	m_model.anchorIndex = anchor;
}
#endif

// ** ***************************
//
void	CMassSpringSolver::TickSimulation	( TDynArray< Vector > & outputPositions, const TDynArray< Vector > & trackPositions, const Vector & upDelta )
{
    ASSERT( outputPositions.Size() == m_prevPositions.Size() );

    m_numIterations++;

	// Initialize relaxation parameters
	m_relaxInnerTicksFloat	= 0.f;
	m_relaxInnerTicksUint	= 0;

	// Calculate numer of steps and dt for this tick
	Float dt = 0.f;
	Uint32 numSteps = 0;
	UpdateTickParams( dt, numSteps );

    // Initialize tracked nodes position change deltas
	TDynArray< Vector > deltas;
    CalculatePositionDeltas( deltas, trackPositions, numSteps );

    // Perform proper number of internal steps
    TDynArray< Vector > locTrackPositions;
    InitLocTrackPositions( locTrackPositions );

    m_numInternalIterations = 0;

    for( Uint32 i = 0; i < numSteps; ++i )
    {
        // Tick simulation (possibly using some more steps for finer simulation)
        InternalTick	 ( dt );

        // Interpolate loc positions
        UpdateLocTrackPos( locTrackPositions, deltas );

        // Update tracked nodes data (position and velocities)
        UpdateTrackParams( locTrackPositions, upDelta, dt );

        m_numInternalIterations++;
    }

    // Apply constraints (after one whole step)
	for( Uint32 i = 0; i < m_numRelaxIterations; ++i )
    {
        ApplyConstraintsOnce();
    }

	// Update positions for the next frame
    UpdatePrevPositions ( trackPositions );

    // Store calculated result
    outputPositions.ClearFast();

    // Copy calculated positions_
    GetPositions	( outputPositions );
}

// ** ***************************
//
void	CMassSpringSolver::InternalTick		( Float dt )
{
	UpdateVelocityHalfStep	( dt );
    AccumulateForces		();
    ApplyForces				( dt );
	ApplyConstraints		();
}

// ** ***************************
//
void	CMassSpringSolver::GetPositions		( TDynArray< Vector > & positions )
{
	ASSERT( positions.Size() == 0 );

	positions.PushBack( m_model.trackedNodes[ 0 ].x );

	for( Uint32 i = 0; i < m_model.nodes.Size(); ++i )
	{
		positions.PushBack( m_model.nodes[ i ].simulation.x );
	}
}

// ** ***************************
//
void	CMassSpringSolver::UpdateTickParams		( Float & dt, Uint32 & numSteps )
{
	Uint64 tickTime		= Red::System::Clock::GetInstance().GetTimer().GetTicks();
	Uint64 deltaTime	= tickTime - m_lastTickTime;

	dt				= 1.f / Float( m_stepsPerSecond ); //FIXME: calculate during initialization
	Double interval = Double( deltaTime ) / Red::System::Clock::GetInstance().GetTimer().GetFrequency();

	numSteps = (Uint32)( Double( m_stepsPerSecond ) * interval );
	ASSERT( numSteps > 0 );

	LOG_ENGINE( TXT( "Steps: %d  interval: %4.4f" ), numSteps, interval );

	m_lastTickTime		= tickTime;
}

// ** ***************************
//
void    CMassSpringSolver::CalculatePositionDeltas ( TDynArray< Vector > & deltas, const TDynArray< Vector > & curPos, Uint32 numSteps )
{
    Float scl = 1.f / (Float) numSteps;

    // Calculate deltas
    for( Uint32 i = 0; i < m_prevPositions.Size(); ++i )
    {
        deltas.PushBack( ( curPos[ i ] - m_prevPositions[ i ] ) * scl );
    }
}

// ** ***************************
//
void    CMassSpringSolver::InitLocTrackPositions   ( TDynArray< Vector > & locTrackPositions )
{
    for( Uint32 i = 0; i < m_prevPositions.Size(); ++i )
    {
        locTrackPositions.PushBack( m_prevPositions[ i ] );
    }
}

// ** ***************************
//
void    CMassSpringSolver::UpdateLocTrackPos       ( TDynArray< Vector > & locTrackPositions, const TDynArray< Vector > & deltas )
{
    for( Uint32 i = 0; i < locTrackPositions.Size(); ++i )
    {
        locTrackPositions[ i ] += deltas[ i ];
    }
}

// ** ***************************
//
void    CMassSpringSolver::UpdatePrevPositions     ( const TDynArray< Vector > & trackedPositions )
{
    ASSERT( m_prevPositions.Size() == trackedPositions.Size() );

    for( Uint32 i = 0; i < trackedPositions.Size(); ++i )
    {
        m_prevPositions[ i ] = trackedPositions[ i ];
    }
}

// ** ***************************
//
void	CMassSpringSolver::UpdateVelocityHalfStep	( Float dt )
{
	TDynArray< SpringNode > & nodes = m_model.nodes;

	for( Uint32 i = 0; i < nodes.Size(); ++i )
	{
		SimulationData & s = nodes[ i ].simulation;

		// Update half step veolcity
		s.v += s.a * ( dt * 0.5f );

		// Update position
		s.x += ( s.v + s.a * ( dt * 0.5f ) ) * dt;
	}
}

// ** ***************************
// FIXME: iterate over connections first and then add rest of the forces
void	CMassSpringSolver::AccumulateForces		()
{
	TDynArray< SpringNode > & nodes = m_model.nodes;
	Float airK = -m_model.airK;

	for( Uint32 i = 0; i < nodes.Size(); ++i )
	{
		SpringNode & n	= nodes[ i ];

		// Calculate gravitational force - it is already stored in model
		Vector gF  = m_model.g * n.simulation.m;

        // Calculate tracked node spring force
        Vector trF = Vector( 0.f, 0.f, 0.f, 0.f );

		for( Uint32 k = 0; k < n.trNodes.Size(); ++k )
		{
			trF += SpringForce( n, *(n.trNodes[ k ]) );
		}
  
		// Calculate spring forces from connected nodes
		Vector nF( 0.f, 0.f, 0.f, 0.f );

		for( Uint32 k = 0; k < n.neighbours.Size(); ++k )
		{
			nF += SpringForce( n, *(n.neighbours[ k ]), *(n.connections[ k ]) );
		}

        // Calculate air friction
        Vector aF = n.simulation.v * airK;

		// Accumulate forces
		n.simulation.F = gF + aF + trF + nF;
	}
}

// ** ***************************
// Velocity Verlet integrator - see: http://en.wikipedia.org/wiki/Verlet_integration
//
// Equations:
//
// x( t + dt ) = x( t ) + v( t ) * dt  + 1/2 * a( t ) * dt^2
// v( t + dt ) = v( t ) + 1/2 * ( a( t ) + a( t + dt ) ) * dt
//
// Algorithm:
//
// 1. x( t + dt ) = x( t ) + v( t ) * dt  + 1/2 * a( t ) * dt^2
// 2. v( t + dt / 2 ) = v( t ) + a( t ) * dt / 2
// 3. a( t + dt ) = F( t + dt ) / m( t + dt )
// 4. v( t + dt ) = v( t + dt / 2 ) + a( t + dt ) * dt / 2 
//
void	CMassSpringSolver::ApplyForces			( Float dt )
{
	TDynArray< SpringNode > & nodes = m_model.nodes;

	for( Uint32 i = 0; i < nodes.Size(); ++i )
	{
		SimulationData & s = nodes[ i ].simulation;

		// Update acceleration
		s.a = s.F / s.m;

		// Update half step veolcity
		s.v += s.a * ( dt * 0.5f );
	}
}

// ** ***************************
// FIXME: ( x( t + dt ) - x( t - dt )  ) / 2dt may be used to improve accuracy
void	CMassSpringSolver::UpdateTrackParams	( const TDynArray< Vector > & trackPositions, const Vector & upDelta, Float dt )
{
    ASSERT( m_model.nodes.Size() > 1 );
    ASSERT( trackPositions.Size() >= m_model.trackedNodes.Size() );

    // Service first node
    TrackedNode & tr = m_model.trackedNodes[ 0 ];
    tr.v = ( trackPositions[ 0 ] - tr.x ) * ( 1.f / dt );
    tr.x = trackPositions[ 0 ];

    for( Uint32 i = 1; i < m_model.trackedNodes.Size(); ++i )
    {
        TrackedNode & tr = m_model.trackedNodes[ i ];

        tr.v = ( trackPositions[ i - 1 ] + upDelta - tr.x ) * ( 1.f / dt );
        tr.x = trackPositions[ i - 1 ] + upDelta;
    }
}

// ** ***************************
//
void	CMassSpringSolver::ApplyConstraints			()
{
	m_relaxInnerTicksFloat += m_relaxPerInnerTick;

	Uint32 numSteps = (Uint32) floorf( m_relaxInnerTicksFloat ) - m_relaxInnerTicksUint;

	for( Uint32 i = 0; i < numSteps; ++i )
    {
        ApplyConstraintsOnce();
    }

	m_relaxInnerTicksUint += numSteps;
}

// ** ***************************
//
void	CMassSpringSolver::ApplyConstraintsOnce		()
{
	// Apply constraints based on tracked nodes
	for( Uint32 i = 0; i < m_model.nodes.Size(); ++i )
	{
		SpringNode & n = m_model.nodes[ i ];

		if( n.trNodes.Size() > 0 )
		{
			Vector x0 = n.simulation.x;

			for( Uint32 k = 0; k < n.trNodes.Size(); ++k )
			{
				TrackedNode * tr = n.trNodes[ k ];

				Vector x1	= tr->x;
				Float	r	= tr->connection.r;

				Vector dx = x1 - x0;
				Float d   = dx.Mag4();

				dx *= 0.5f * ( r - d ) / d; //maybe 0.5 should be removed from here??
				//dx *= ( r - d ) / d; //maybe 0.5 should be removed from here??

				n.simulation.x -= dx;
			}
		}		
	}

	// Apply inter spring node constraints
    for( Uint32 i = 0; i < m_model.connections.Size(); ++i )
    {
        SimulationData & s0 = m_model.nodes[ i ].simulation;
        SimulationData & s1 = m_model.nodes[ i + 1 ].simulation;

        Float r		= m_model.connections[ i ].r;

        Vector dx	= s1.x - s0.x;
        Float d		= dx.Mag4();

        dx *= 0.5f * ( r - d ) / d;

        s0.x -= dx;
        s1.x += dx;
    }
}

#if 0
// ** ***************************
//
void	CMassSpringSolver::CorrectAnchors		()
{
	SpringNode & n = m_model.nodes[ m_model.anchorIndex ];

	n.simulation.x = n.trackedNode->x;
}
#endif

// ** ***************************
//
Float	CMassSpringSolver::Dist				( const SpringNode & n0, const SpringNode & n1 ) const
{
	Vector v = n1.simulation.x - n0.simulation.x;

	return v.Mag4();
}

// ** ***************************
//
Float	CMassSpringSolver::Dist				( const SpringNode & n0, const TrackedNode & n1 ) const
{
	Vector v = n0.simulation.x - n1.x;

	return v.Mag4();
}

// ** ***************************
//
Vector	CMassSpringSolver::SpringForce			( const SpringNode & n0, const SpringNode & n1, const SpringConnection & connection ) const
{
	Float ks	= connection.ks;
	Float kd	= connection.kd;
	Float r		= connection.r;

	Vector dv	= n0.simulation.v - n1.simulation.v;
	Vector dx	= n0.simulation.x - n1.simulation.x;

	Float d		= dx.Mag4();

	Float mag	= 0.f;

	if( d > 0.0001f )
	{
		Float xdv = Vector::Dot4( dv, dx );

		// Hook's spring law - see http://www.cs.cmu.edu/~baraff/sigcourse/notesc.pdf
		mag = -( ks * ( d - r ) + kd * xdv / d );

		// dx normalization factor (so that dx doesn't have to be normalized again)
		mag /= d;
	}

	return dx * mag;
}

// ** ***************************
// 
Vector	CMassSpringSolver::SpringForce			( const SpringNode & n0, const TrackedNode & n1 ) const
{
	Float ks	= n1.connection.ks;
	Float kd	= n1.connection.kd;
	Float r		= n1.connection.r;

	Vector dv	= n0.simulation.v - n1.v;
	Vector dx	= n0.simulation.x - n1.x;

	Float d		= dx.Mag4();

	Float mag	= 0.f;

	if( d > 0.0001f )
	{
		Float xdv = Vector::Dot4( dv, dx );

		// Hook's spring law - see http://www.cs.cmu.edu/~baraff/sigcourse/notesc.pdf
		mag = -( ks * ( d - r ) + kd * xdv / d );

		// dx normalization factor (so that dx doesn't have to be normalized again)
		mag /= d;
	}

	return dx * mag;
}

// ** ***************************
//
String	CMassSpringSolver::DumpSolverState		( const String & msg, bool logConstants ) const
{
	CMassSpringModelStateDumper d;

	String m = d.DumpMassSpringModel( m_model, logConstants );

	return String::Printf( TXT( "SOLVER [i %d | li %d]  ---- %s ----- \n\n%s" ), m_numIterations, m_numInternalIterations, msg.AsChar(), m.AsChar() );
}

// ****************************************************************************************************** //
// *************************************** DUMPER IMPLEMENTATION **************************************** //
// ****************************************************************************************************** //

// ** ***************************
//
String  CMassSpringModelStateDumper::DumpMassSpringModel( const MassSpringModel & model, bool logConstants ) const
{
	String grav = ToString( model.g );

	String trNodes;

	for( Uint32 i = 0; i < model.trackedNodes.Size(); ++i )
	{
		trNodes += DumpTrackedNode( model.trackedNodes[ i ], logConstants ) + TXT( "\n" );
	}

	String connections;

	for( Uint32 i = 0; i < model.connections.Size(); ++i )
	{
		const SpringNode & n0 = model.nodes[ i ];
		const SpringNode & n1 = model.nodes[ i + 1 ];

		connections += Id( n0 ) + TXT( " <---> **([ ") + DumpConnection( model.connections[ i ] ) + TXT( " ])** <---> ") + Id( n1 ) + TXT( "\n" );
	}

	String nodes;

	for( Uint32 i = 0; i < model.nodes.Size(); ++i )
	{
		nodes += DumpSpringNode( model.nodes[ i ], logConstants ) + TXT( "\n" );
	}

	if( logConstants )
	{
		return String::Printf( TXT( "Model with gravity: %s\n*********Tracked nodes:\n%s\n*********Connections:\n%s\n**********Spring nodes:\n%s" ), grav.AsChar(), trNodes.AsChar(), connections.AsChar(), nodes.AsChar() );
	}

	return String::Printf( TXT( "*********Tracked nodes:\n%s\n**********Spring nodes:\n%s" ), trNodes.AsChar(), nodes.AsChar() );
}

// ** ***************************
//
String	CMassSpringModelStateDumper::DumpSimulation		( const SimulationData & simulation, bool logConstants ) const
{
	String id = Id( simulation );
	String F = ToString( simulation.F );
	String x = ToString( simulation.x );
	String v = ToString( simulation.v );
	String a = ToString( simulation.a );
	Float  m = simulation.m;

	if( logConstants )
	{
		return String::Printf( TXT( "%s F: %s x: %s v: %s a: %s m: %4.4f" ), id.AsChar(), F.AsChar(), x.AsChar(), v.AsChar(), a.AsChar(), m );
	}
	else
	{
		return String::Printf( TXT( "%s F: %s x: %s v: %s a: %s" ), id.AsChar(), F.AsChar(), x.AsChar(), v.AsChar(), a.AsChar() );
	}
}

// ** ***************************
//
String	CMassSpringModelStateDumper::DumpSpringNode		( const SpringNode & node, bool logConstants ) const
{
	String id	= Id( node );
	String sim	= DumpSimulation( node.simulation, logConstants );
	String trId	= TXT( "a" );//Id( *node.trackedNode );

	if( logConstants )
	{
		String curStr = String::Printf( TXT( "%s with trNode: %s \nSimulation:  %s\nConnections:\n" ), id.AsChar(), trId.AsChar(), sim.AsChar() );

		ASSERT( node.connections.Size() == node.neighbours.Size() );

		for( Uint32 i = 0; i < node.connections.Size(); ++i )
		{
			const SpringConnection * conn	= node.connections[ i ];
			const SpringNode * n			= node.neighbours[ i ];

			curStr += TXT( "SELF <---> " ) + Id( *conn ) + TXT( " <---> " ) + Id( *n ) + TXT( "\n" );
		}

		return curStr;
	}
	else
	{
		return String::Printf( TXT( "%s with Simulation: %s" ), id.AsChar(), sim.AsChar() );
	}
}

// ** ***************************
//
String	CMassSpringModelStateDumper::DumpConnection		( const SpringConnection & connection ) const
{
	String id = Id( connection );

	Float ks = connection.ks;
	Float kd = connection.kd;
	Float r  = connection.r;

	return String::Printf( TXT( "%s ks: %4.4f kd: %4.4f r: %4.4f" ), id.AsChar(), ks, kd, r );
}

// ** ***************************
//
String	CMassSpringModelStateDumper::DumpTrackedNode	( const TrackedNode	& node, bool logConstants ) const
{
	String id = Id( node );

	String x = ToString( node.x );
	String v = ToString( node.v );
	String c = DumpConnection( node.connection );

	if( logConstants )
	{
		return String::Printf( TXT( "%s x: %s v: %s connection: %s" ), id.AsChar(), x.AsChar(), v.AsChar(), c.AsChar() );
	}
	else
	{
		return String::Printf( TXT( "%s x: %s v: %s" ), id.AsChar(), x.AsChar(), v.AsChar() );
	}

}
