/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#pragma once

extern Uint32 GConn;
extern Uint32 GSprNode;
extern Uint32 GTrNode;
extern Uint32 GSim;

// ! Debug helper - obj counter

template< typename T >
Uint32 IncCounter()
{
	return 1012327892;
}

// ! Debug helper - obj counter
template< typename T >
struct Counter
{
	Counter()
	{
		// FIXME: bullshit
		objId = IncCounter< T >();		
	}

	~Counter	()
	{
		//--objCreated;
	}

	Uint32	GetId	() const
	{
		return objId;
	}

private:

	Uint32		objId;
//	static Uint32	objCreated;
};

// ! Simulation data stored withing simulated nodes
struct SimulationData : public Counter< SimulationData >
{
	Vector F;	//!< Current net force - updated first

	Vector x;	//!< Current position - updated second
	Vector v;	//!< Current velocity - updated fourth
	Vector a;	//!< Current acceleration - updated third

	Float  m;	//!< Mass - most probably won't change during simulation but at some point this feature may become handy
};

// ! Spring connection - describes connection parameters
struct SpringConnection : public Counter< SpringConnection >
{
	Float ks;	//!< Spring stiffness
	Float kd;	//!< Damping parameter
	Float r;	//!< Rest length
};

// ! Node tracked by simulated nodes
struct TrackedNode : public Counter< TrackedNode >
{
	Vector x;	//!< Current position - updated externally (not simulated)
	Vector v;	//!< Current velocity - updated based on position change

	SpringConnection connection; //!< Tracked node connection data
};

// ! Spring node
struct SpringNode : public Counter< SpringNode > 
{
	TDynArray< SpringNode * >		neighbours;				//!< Neighbouring nodes
	TDynArray< SpringConnection * >	connections;			//!< Connections to neighbours
	TDynArray< TrackedNode * >		trNodes;				//!< Tracked nodes for this one

	SimulationData					simulation;				//!< Simulation data for current node - used during integration
};

// ! Mass spring model
struct MassSpringModel
{
	TDynArray< SpringConnection >	connections;	//!< Connection between spring nodes in the system
	TDynArray< SpringNode >			nodes;			//!< Spring nodes in the system
	TDynArray< TrackedNode >		trackedNodes;	//!< Tracked nodes in the system

#if 0
	Uint32							anchorIndex;	//!< Anchor node index (we assume one only)
#endif

	Vector							g;				//!< Gravity vector

    Float                           sprKS;          //!< Spring node KS
    Float                           sprKD;          //!< Spring node KD
    Float                           trKS;           //!< Traced node KS
    Float                           trKD;           //!< Traced node KD
    Float                           m;              //!< Spring node mass
    Float                           airK;           //!< Air friction constant
};

// ! Debug helper
class CMassSpringModelStateDumper
{
public:

	String  DumpMassSpringModel	( const MassSpringModel & model, bool logConstants ) const;
	String	DumpSimulation		( const SimulationData & simulation, bool logConstants ) const;
	String	DumpSpringNode		( const SpringNode & node, bool logConstants ) const;
	String	DumpConnection		( const SpringConnection & connection ) const;
	String	DumpTrackedNode		( const TrackedNode	& node, bool logConstants ) const;

private:

	String	Id	( const SimulationData & simulation ) const
	{
		return String::Printf( TXT( "SimulationData_%d" ), simulation.GetId() );
	}

	String	Id	( const SpringNode & node ) const
	{
		return String::Printf( TXT( "SpringNode_%d" ), node.GetId() );
	}

	String	Id	( const SpringConnection & connection ) const
	{
		return String::Printf( TXT( "SpringConnection_%d" ), connection.GetId() );
	}

	String	Id	( const TrackedNode	& node ) const
	{
		return String::Printf( TXT( "TrackedNode_%d" ), node.GetId() );
	}

	String ToString( const Vector & v ) const
	{
		return TXT( "[" ) + ::ToString( v ) + TXT( "]" );
	}
};

template<>
Uint32 IncCounter< SimulationData >()
{
	return GSim++;
}

template<>
Uint32 IncCounter< SpringConnection >()
{
	return GConn++;
}

template<>
Uint32 IncCounter< TrackedNode >()
{
	return GTrNode++;
}

template<>
Uint32 IncCounter< SpringNode >()
{
	return GSprNode++;
}


// ! Mass spring model implementation - first iteration
class CMassSpringSolver
{
private:

	MassSpringModel		m_model;

	Vector				m_trNodesDelta;
	TDynArray< Vector > m_prevPositions;

	Uint32				m_numIterations;
	Uint32				m_numRelaxIterations;
	Uint32				m_numInnerRelaxIterations;
	Uint32				m_numInternalIterations;
	Uint32				m_stepsPerSecond;

	Uint64				m_lastTickTime;

	Float				m_relaxInnerTicksFloat;
	Float				m_relaxPerInnerTick;
	Uint32				m_relaxInnerTicksUint;

public:

	CMassSpringSolver		();
	~CMassSpringSolver		();

	void	Initialize		( const TDynArray< Vector > & initNodes, const Vector & upDelta );

	Vector	DefaultUpDelta	() const
	{
		return Vector( 0.05f, 0.f, 0.f, 0.f ); // approx. 5 cm
	}

private:

	void	InitModelParams		();
    void    InitPositions		( const TDynArray< Vector > & initNodes );
	void	InitTrackedNodes	( const TDynArray< Vector > & initNodes, const Vector & upDelta );
	void	InitSpringNodes		( const TDynArray< Vector > & initNodes );

	void	PreInitSpringNodes	( const TDynArray< Vector > & initNodes );
	void	InitSprConnections	( const TDynArray< Vector > & initNodes );
	void	PostInitSpringNodes	();

public:

	void	SetGravity		( const Vector & g );
	void	SetNodeMass		( Float m );
#if 0
	void	SetAnchor		( Uint32 anchor );
#endif

	void	TickSimulation	( TDynArray< Vector > & outputPositions, const TDynArray< Vector > & trackPositions, const Vector & upDelta );

	Uint32	GetNumIterations() const
	{	
		return m_numIterations;
	}

	void	IncNumIterations()
	{
		++m_numIterations;
	}

	// ! Debugging method
	String	DumpSolverState	( const String & msg, bool logConstants ) const;

private:

	void	InternalTick			( Float dt );
	void	GetPositions			( TDynArray< Vector > & positions );

	void	UpdateTickParams		( Float & dt, Uint32 & numSteps );

	void    CalculatePositionDeltas ( TDynArray< Vector > & deltas, const TDynArray< Vector > & curPos, Uint32 numSteps );
    void    InitLocTrackPositions   ( TDynArray< Vector > & locTrackPositions );
    void    UpdateLocTrackPos       ( TDynArray< Vector > & locTrackPositions, const TDynArray< Vector > & delta );
    void    UpdatePrevPositions     ( const TDynArray< Vector > & trackedPositions );

	void	UpdateVelocityHalfStep	( Float dt );
	void	AccumulateForces		();
	void	ApplyForces				( Float dt );
	void	UpdateTrackParams		( const TDynArray< Vector > & trackPositions, const Vector & upDelta, Float dt );
	void	ApplyConstraintsOnce	();
	void	ApplyConstraints		();

#if 0
	void	CorrectAnchors			();
#endif

	Float	Dist					( const SpringNode & n0, const SpringNode & n1 ) const;
	Float	Dist					( const SpringNode & n0, const TrackedNode & n1 ) const;

	Vector	SpringForce				( const SpringNode & n0, const SpringNode & n1, const SpringConnection & connection ) const;
	Vector	SpringForce				( const SpringNode & n0, const TrackedNode & n1 ) const;
};

// FIXME: add those ODE, particle integrators - system will only generate necessary data and provide proper answer to queries
// FIXME: but thje integration itself will be carried out by integrator class
