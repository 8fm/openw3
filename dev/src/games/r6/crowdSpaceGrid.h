/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#pragma once

#include "crowdAgent.h"

#define NUM_GRID_CELLS_XY 200
#define GRID_CELL_SIZE 2.f
#define CELL_BASE_ARRAY_SIZE 6
#define GRID_EPSILON 0.0001f

class CCrowdArea;  
class CCrowdDebuggerCanvas;

class CCrowdSpace_Grid
{
	// temporary
	friend class CCrowdDebuggerCanvas;

//------------------------------------------------------------------------------------------------------------------
// Helper structs
//------------------------------------------------------------------------------------------------------------------
private:
	struct SGridCellForAgents
	{
		TAgentIndex			m_firstAgent;	

		struct Iterator : public Red::System::NonCopyable
		{
			const TAgentIndex*	m_nextAgents;
			TAgentIndex			m_currentAgent;

			RED_INLINE Iterator( const CCrowdSpace_Grid* self, Uint32 x, Uint32 y );
			RED_INLINE Bool			IsValid() const			{ return IS_VALID_AGENT_INDEX( m_currentAgent ); }
			RED_INLINE void			Next()					{ ASSERT( IsValid() ); m_currentAgent = m_nextAgents[ m_currentAgent ]; }
			RED_INLINE TAgentIndex	Get() const				{ return m_currentAgent; }
			RED_INLINE				operator Bool () const	{ return IsValid(); }
			RED_INLINE void			operator++ ()			{ Next(); }
			RED_INLINE TAgentIndex	operator*()	const		{ return Get(); }
		};
	};

	struct SGridCellForAreas
	{
		TAreaIndex	m_indexOrOffset;	
		Int16		m_numAreas;

		RED_INLINE void Clear()								{ m_indexOrOffset = INVALID_AREA_INDEX; m_numAreas = 0; }

		struct Iterator : public Red::System::NonCopyable
		{
			const TAreaIndex*	m_data;
			Int16				m_numLeft;

			RED_INLINE Iterator( const CCrowdSpace_Grid* self, Uint32 x, Uint32 y );
			RED_INLINE Bool			IsValid() const			{ return m_numLeft > 0; }
			RED_INLINE void			Next()					{ ASSERT( IsValid() ); ++m_data; --m_numLeft; }
			RED_INLINE TAreaIndex		Get() const				{ return *m_data; }
			RED_INLINE				operator Bool () const	{ return IsValid(); }
			RED_INLINE void			operator++ ()			{ Next(); }
			RED_INLINE TAreaIndex		operator*()	const		{ return Get(); }
		};
	};

	struct SGridCellForObstacles
	{
		TObstacleIndex				m_firstObstacle;	

		struct Iterator : public Red::System::NonCopyable
		{
			const TObstacleIndex*	m_nextObstacles;
			TObstacleIndex			m_currentObstacle;

			RED_INLINE Iterator( const CCrowdSpace_Grid* self, Uint32 x, Uint32 y );
			RED_INLINE Bool			IsValid() const			{ return IS_VALID_OBSTACLE_INDEX( m_currentObstacle ); }
			RED_INLINE void			Next()					{ ASSERT( IsValid() ); m_currentObstacle = m_nextObstacles[ m_currentObstacle ]; }
			RED_INLINE TObstacleIndex	Get() const				{ return m_currentObstacle; }
			RED_INLINE				operator Bool () const	{ return IsValid(); }
			RED_INLINE void			operator++ ()			{ Next(); }
			RED_INLINE TObstacleIndex	operator*()	const		{ return Get(); }
		};
	};

	struct SGridCrawler : public Red::System::NonCopyable
	{
		enum EDir { DOWN, RIGHT, UP, LEFT, MAXDIR };
		
		Int32	X, Y;

		RED_INLINE SGridCrawler()								{}
		RED_INLINE SGridCrawler( Int32 x, Int32 y )			: X( x ), Y( y ) { ASSERT( x < NUM_GRID_CELLS_XY && y < NUM_GRID_CELLS_XY ); }
		RED_INLINE Bool IsValidCell() const					{ return X >= 0 && X < NUM_GRID_CELLS_XY && Y >= 0 && Y < NUM_GRID_CELLS_XY; }
		RED_INLINE EDir NextDir( EDir dir ) const				{ EDir d = static_cast< EDir > ( dir + 1 ); return ( d >= MAXDIR ) ? DOWN : d; } 
		RED_INLINE Int32 EDirToXDiff( EDir dir ) const		{ switch( dir ) { case RIGHT: return 1; case LEFT: return -1; } return 0; } 
		RED_INLINE Int32 EDirToYDiff( EDir dir ) const		{ switch( dir ) { case UP: return 1; case DOWN: return -1; } return 0; } 
		RED_INLINE EDir XDiffToEDir( Int32 xdiff ) const		{ ASSERT( xdiff != 0 ); return ( xdiff < 0 ) ? LEFT : RIGHT; }
		RED_INLINE EDir YDiffToEDir( Int32 ydiff ) const		{ ASSERT( ydiff != 0 ); return ( ydiff < 0 ) ? DOWN : UP; }
	};

	struct SGridCrawlerCCW : public SGridCrawler
	{		
		Uint32	m_cellLimit;
		Uint32	m_currCell;
		EDir	m_dir;
		Uint32	m_dirLimit;
		Uint32	m_currDirLimit;

		RED_INLINE SGridCrawlerCCW( Int32 x, Int32 y, Uint32 size = NUM_GRID_CELLS_XY );
		RED_INLINE Bool	IsValid() const						{ return m_currCell < m_cellLimit; }		
		RED_INLINE		operator Bool () const				{ return IsValid(); }
		RED_INLINE void	operator++ ()						{ Next(); }
		RED_INLINE void	Next();
	};

	struct SGridCrawlerInOrder : public SGridCrawler
	{
		Int32	m_startX, m_startY;
		Int32	m_sizeX, m_sizeY;

		RED_INLINE SGridCrawlerInOrder( Int32 x, Int32 y, Int32 sizeX, Int32 sizeY );
		RED_INLINE SGridCrawlerInOrder( const Box2& relativeBox );
		RED_INLINE Bool	IsValid() const						{ return X < m_startX + m_sizeX && Y < m_startY + m_sizeY; }
		RED_INLINE		operator Bool () const				{ return IsValid(); }
		RED_INLINE void	operator++ ()						{ Next(); }
		RED_INLINE void	Next();
	};

	struct SGridCrawlerRay : public SGridCrawler
	{
		Int32		m_currX, m_currY;
		Int32		m_endX, m_endY;
		EDir		m_dir;
		SCrowdRay2	m_ray;
		Uint8		m_dirsToCheckLeft;
		Bool		m_valid			: 1;

		RED_INLINE SGridCrawlerRay( const SCrowdRay2& ray, const Vector& aabbMin );
		RED_INLINE		operator Bool () const				{ return m_valid; }
		RED_INLINE void	operator++ ()						{ Next(); }
		RED_INLINE void	Next();
		RED_INLINE Bool	IsRayOnCell( Int32 cellX, Int32 cellY ) const;
		RED_INLINE Bool	IsRayCloseToCell( Int32 cellX, Int32 cellY ) const;
		RED_INLINE EDir	CalculateCaretDir() const;
		RED_INLINE Bool	IsLastCell() const					{ return m_endX == m_currX && m_endY == m_currY; }
	};

	struct SSortedByDistanceInplaceAgentsArray
	{
		CCrowdManager*		m_manager;
		TAgentIndex*		m_array;
		Vector2				m_referencePosition;
		TAgentIndex			m_maxSize;
		TAgentIndex			m_currSize;

		RED_INLINE SSortedByDistanceInplaceAgentsArray( CCrowdManager* manager, const Vector2& pos, TAgentIndex max, TAgentIndex* array );
		RED_INLINE Bool Insert( TAgentIndex agent );
		RED_INLINE Bool IsFull() const { return m_currSize == m_maxSize; }
		RED_INLINE TAgentIndex CurrSize() const { return m_currSize; }
	};

//------------------------------------------------------------------------------------------------------------------
// Internal data
//------------------------------------------------------------------------------------------------------------------
private:
	// System data
	CCrowdManager*				m_manager;
	Box2						m_aabb;
	Bool						m_ready;

	// Agents data
	TAgentIndex					m_numAgents;
	TAgentIndex					m_nextAgents[ MAX_CROWD_AGENTS ];								// 2 * 1000 = 2000 bytes				= ~2 kb
	SGridCellForAgents			m_gridForAgents[ NUM_GRID_CELLS_XY ][ NUM_GRID_CELLS_XY ];		// 2 * 200 * 200 = 80000 bytes			= ~80 kb

	#ifndef NO_GET_AGENTS_AREA
		// Areas data
		TDynArray< TAreaIndex >		m_areasData;												// depends... around...					~ few kb 
		SGridCellForAreas			m_gridForAreas[ NUM_GRID_CELLS_XY ][ NUM_GRID_CELLS_XY ];	// 4 * 200 * 200 = 160000 bytes			= ~160 kb
	#endif
	
	// Obstacles data
	TDynArray< TObstacleIndex >	m_obstaclesData;												// depends... 4 bytes * numObstacles... ~ few kb
	SGridCellForObstacles		m_gridForObstacles[ NUM_GRID_CELLS_XY ][ NUM_GRID_CELLS_XY ];	// 4 * 200 * 200 = 160000 bytes			= ~160 kb


//------------------------------------------------------------------------------------------------------------------
// Internal helper inline methods 
//------------------------------------------------------------------------------------------------------------------
private:
	RED_INLINE Box2 GetGridArea() const { return m_aabb; }
	RED_INLINE Vector2 GetAgentPos2( TAgentIndex agent ) const;
	RED_INLINE void CellCoords( const Vector2& pos, Uint32& x, Uint32& y ) const;
	RED_INLINE Box2 RelativeBox2( const Box2& absoluteBox ) const;
	RED_INLINE TAgentIndex CountAgentsInCell( Uint32 x, Uint32 y ) const;
	RED_INLINE Uint32 EstimateCrawlerSizeForNAgents( TAgentIndex n ) const;
	RED_INLINE Vector2 GetObstacleStart2( TObstacleIndex idx ) const;
	RED_INLINE Vector2 GetObstacleEnd2( TObstacleIndex idx ) const;
	RED_INLINE TAgentIndex GetNumAgents() const { return m_numAgents; }
	RED_INLINE Float GetAgentZ( TAgentIndex agent ) const;
	RED_INLINE Box CalcAgentBox( TAgentIndex agent ) const;  
	RED_INLINE Bool FrustumTest( TAgentIndex agent, const CFrustum& frustum ) const { return 0 != frustum.TestBox( CalcAgentBox( agent ) ); }
	RED_INLINE Bool IsAgentActive( TAgentIndex agent ) const;

//------------------------------------------------------------------------------------------------------------------
// Public interface 
//------------------------------------------------------------------------------------------------------------------
public:
	CCrowdSpace_Grid();

	// Called by crowd manager
	void OnInit( CCrowdManager* manager );

	// Called by crowd manager
	void Reset();

	// Called by crowd manager
	void UpdateSpace();

public:
	// Get all agents within the frustum (maximum: maxAgents). Returns number of agents in frustum.
	TAgentIndex GetAgentsInFrustum( const CFrustum& frustum, TAgentIndex maxAgents, TAgentIndex* result ) const;

	// Writes indices of a maximum n agents nearest to specified point. Returns actual number of agents found.
	TAgentIndex GetNNearestAgents( const Vector2& pos, TAgentIndex n, TAgentIndex* result ) const;

	// Writes indices of a maximum n agents nearest to specified point, but no futher away than radius. Returns actual number of agents found.
	TAgentIndex GetNearestAgentsWithinRadius( const Vector2& pos, Float radius, TAgentIndex maxAgents, TAgentIndex* result ) const;	

	// Writes indices of a maximum n agents being within a radius from some point. Returns actual number of agents found.
	TAgentIndex GetAnyAgentsWithinRadius( const Vector2& pos, Float radius, TAgentIndex maxAgents, TAgentIndex* result ) const;	

	// Calculates number of agents currently residing within an area
	TAgentIndex GetAgentsCountWithinArea( const CCrowdArea* area ) const;	

	#ifndef NO_GET_AGENTS_AREA
		// Get the CrowdArea of a specific agent
		TAreaIndex GetAgentsAreaIndex( const SCrowdAgent& agent ) const;
		const CCrowdArea* GetAgentsArea( const SCrowdAgent& agent ) const;
	#endif

	// Raycast - test 2D ray against the crowd. Returns true if any crowd agent was hit.
	Bool Ray2Test_Any( const SCrowdRay2& ray ) const;

	// Raycast - test 2D ray against the crowd. Returns true if any crowd agent was hit, outputs index of an hit agent 
	// to hitAgent (index of an agent closest to ray.m_start who was hit by that ray). 
	Bool Ray2Test_Closest( const SCrowdRay2& ray, TAgentIndex& hitAgent ) const;

	// Raycast - test 2D ray against the crowd. Returns number of outputed agents. Outputs each hit agents index 
	// in order of a distance to ray.m_start, until maxAgents limit is reached (or all hit agents are outputed).
	TAgentIndex Ray2Test_NClosest( const SCrowdRay2& ray, TAgentIndex maxAgents, TAgentIndex* outputArray ) const;

	// TODO: implement 3D versions:

	// Raycast - test 3D ray against the crowd. Returns true if any crowd agent was hit.
	// NOT IMPLEMENTED YET
	Bool Ray3Test_Any( const SCrowdRay3& ray ) const;

	// Raycast - test 3D ray against the crowd. Returns true if any crowd agent was hit, outputs index of an hit agent 
	// to hitAgent (index of an agent closest to ray.m_start who was hit by that ray). 
	// NOT IMPLEMENTED YET
	Bool Ray3Test_Closest( const SCrowdRay3& ray, TAgentIndex& hitAgent ) const;

	// Raycast - test 3D ray against the crowd. Returns number of outputed agents. Outputs each hit agents index 
	// in order of a distance to ray.m_start, until maxAgents limit is reached (or all hit agents are outputed).
	// NOT IMPLEMENTED YET
	TAgentIndex Ray3Test_NClosest( const SCrowdRay3& ray, TAgentIndex maxAgents, TAgentIndex* outputArray ) const;

	// Writes indices of a maximum maxObstacles obstacles near to specified point, but no futher away than radius. Returns actual number of obstacles found. 
	// The result is NOT sorted, so if you provide too small result buffer, it might happen that closest obstacles aren't there.
	TObstacleIndex GetObstaclesWithinRadius( const Vector2& pos, Float radius, TObstacleIndex maxObstacles, TObstacleIndex* result ) const;	
};
