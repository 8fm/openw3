#include "build.h"
#include "swarmEnvironment.h"
#include "humbleCritterAlgorithmData.h"

CSwarmFloodFill::CSwarmFloodFill( SwarmEnviromentData* enviroment, CPathLibWorld* pathLibWorld, Float cellRadius, CAreaComponent* areaBoundings )
		: m_priorityQueue()
		, m_enviroment( enviroment )
		, m_pathLibWorld( pathLibWorld )
		, m_areaBoundings( areaBoundings )
		, m_cellRadius( cellRadius )
{
}

void CSwarmFloodFill::Process(  )
{
	while ( !m_priorityQueue.Empty() )
	{
		const CSwarmHeadData heapData = m_priorityQueue.Front();
		m_priorityQueue.PopHeap();

		PathLib::AreaId areaId		= PathLib::INVALID_AREA_ID;
	
		// [STEP] Pop a cell, making sure it is a copy not a reference because Heap will corrupt reference if Algo pushes stuff inside via ProcessNeighbourCell
		const Vector2 cellWorldPosition2D		= m_enviroment->GetCelWorldPositionFromCoordinates( heapData.m_coord.X, heapData.m_coord.Y );
		const SwarmEnviromentCelData& cellData	= m_enviroment->GetChannel( heapData.m_coord.X, heapData.m_coord.Y );
		const Vector3 cellWorldPosition3D( cellWorldPosition2D.X, cellWorldPosition2D.Y, cellData.m_z );

		// [STEP] Process all neighbours of this cell
		const CCoord coordRight( heapData.m_coord.X + 1, heapData.m_coord.Y );
		ProcessNeighbourCell( coordRight, cellWorldPosition3D, areaId, heapData.m_floodSourcePosition2D, heapData.m_lastValidHeight );
		const CCoord coordLeft( heapData.m_coord.X - 1, heapData.m_coord.Y );
		ProcessNeighbourCell( coordLeft, cellWorldPosition3D, areaId, heapData.m_floodSourcePosition2D, heapData.m_lastValidHeight );
		const CCoord coordUp( heapData.m_coord.X, heapData.m_coord.Y + 1 );
		ProcessNeighbourCell( coordUp, cellWorldPosition3D, areaId, heapData.m_floodSourcePosition2D, heapData.m_lastValidHeight );
		const CCoord coordDown( heapData.m_coord.X, heapData.m_coord.Y - 1 );
		ProcessNeighbourCell( coordDown, cellWorldPosition3D, areaId, heapData.m_floodSourcePosition2D, heapData.m_lastValidHeight );
	}
}

void CSwarmFloodFill::AddFloodSource( const Vector3 &floodSourcePosition )
{
	static Float acceptanceDist = 1.0f;
	Int32 x, y;
	m_enviroment->GetCellCoordinatesFromWorldPosition( floodSourcePosition.AsVector2(), x, y );
	const CCoord floodSourcePositionCoord( x, y );

	SwarmEnviromentCelData *const cellData = IsCellElligible( floodSourcePositionCoord );
	// Is cell is not Elligible then do not add the flood source
	if ( cellData == nullptr )
	{
		return;
	}
		
	Float height						= 0.0f;
	Bool cellBlocked					= false;
	const Vector2 cellWorldPosition2D	= m_enviroment->GetCelWorldPositionFromCoordinates( floodSourcePositionCoord.X, floodSourcePositionCoord.Y );

	PathLib::AreaId areaId		= PathLib::INVALID_AREA_ID;
	if ( m_pathLibWorld->ComputeHeight( floodSourcePosition.AsVector2(), floodSourcePosition.Z - SWARM_FLOOD_FILL_SP_RANGE * 0.5f, floodSourcePosition.Z + SWARM_FLOOD_FILL_SP_RANGE * 0.5f, height, areaId ) == false )
	{
		cellBlocked = true;
	}
	if ( cellBlocked == false )
	{
		ProcessCell( height, floodSourcePositionCoord, *cellData, floodSourcePosition.AsVector2(), 0.0f );
	}
}

SwarmEnviromentCelData *const CSwarmFloodFill::IsCellElligible( const CCoord & cellCoord  )const
{
	// Is cell in grid ?
	if ( m_enviroment->IsCorrectCell( cellCoord.X, cellCoord.Y ) )
	{
		// Was this cell already explored by the flood fill ?
		SwarmEnviromentCelData& cellData = m_enviroment->GetChannel( cellCoord.X, cellCoord.Y );
		if ( (cellData.m_flags & CDF_FLOOD_FILL_EXPLORED) == 0 )
		{
			return &cellData;
		}
	}
	return nullptr;
}

void CSwarmFloodFill::ProcessCell( Float height, const CCoord & cellCoord, SwarmEnviromentCelData& cellData, const Vector2 &floodSourcePosition2D, Float lastValidHeight )
{
	const Vector2 cellWorldPosition2D = m_enviroment->GetCelWorldPositionFromCoordinates( cellCoord.X, cellCoord.Y );
	Bool cellBlocked = false;
	
	const Vector3 cellWorldPosition3D( cellWorldPosition2D.X, cellWorldPosition2D.Y,  height);
	if ( m_pathLibWorld->TestLocation( cellWorldPosition3D, m_cellRadius, PathLib::CT_DEFAULT ) == false )
	{
		cellBlocked = true;
	}
	

	// Finally making sure this cell is within area boundings
	if ( cellBlocked == false && m_areaBoundings )
	{
		if ( MathUtils::GeometryUtils::TestEncapsulationPolygonCircle2D( m_areaBoundings->GetWorldPoints(), cellWorldPosition2D, m_cellRadius ) == false )
		{
			cellBlocked = true;
		}
	}
			
	if ( cellBlocked == false )
	{
		cellData.m_flags		|= CDF_FLOOD_FILL_EXPLORED;
		cellData.m_z			= height;
	
		if ( cellBlocked )
		{
			cellData.m_flags	|= CDF_BLOCKED;
		}
		else
		{
			lastValidHeight		= cellData.m_z;
			cellData.m_flags	&= ~CDF_BLOCKED;
		}
		
		const Float sqDistanceFromFloodSource = (floodSourcePosition2D - cellWorldPosition2D).SquareMag();
		m_priorityQueue.PushHeap( CSwarmHeadData( cellCoord, sqDistanceFromFloodSource, floodSourcePosition2D, lastValidHeight ) );
			
	}
}
void CSwarmFloodFill::ProcessNeighbourCell( const CCoord & coord, const Vector3 &lastExporedPosition, PathLib::AreaId &areaId, const Vector2 &floodSourcePosition2D, Float lastValidHeight )
{
	SwarmEnviromentCelData *const cellData = IsCellElligible( coord );
	if ( cellData == nullptr )
	{
		return;
	}
		
	Float height				= lastValidHeight;
	Bool cellBlocked			= false;
	Vector2 cellWorldPosition2D	= m_enviroment->GetCelWorldPositionFromCoordinates( coord.X, coord.Y );
	
	if ( m_pathLibWorld->ComputeHeightFrom( areaId, cellWorldPosition2D, lastExporedPosition, height ) == false )
	{
		cellBlocked = true;
	}
	if ( cellBlocked == false )
	{
		ProcessCell( height, coord, *cellData, floodSourcePosition2D, lastExporedPosition.Z );
	}
}