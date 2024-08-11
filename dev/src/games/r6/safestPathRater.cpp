#include "build.h"
#include "safestPathRater.h"
#include "coversManager.h"
#include "enemyAwareComponent.h"

#include "..\..\common\engine\pathlibGraph.h"
#include "..\..\common\game\nodeStorage.h"
#include "..\..\common\core\mathUtils.h"

const Float	CSafestParhRater::DISTANCE_MULTIPLER	= 100;	
const Float	CSafestParhRater::COVER_SEARCH_MARGIN	= 0.5f;
const Int32 CSafestParhRater	::MAX_COVERS			= 6;
const Float	CSafestParhRater::MIN_COST_MULTIPLER	= 0.01f;

void CSafestParhRater::PathFindingStarted()
{
	m_enemyAwareComponent = m_ownerEntity->FindComponent< CEnemyAwareComponent >();
}

PathLib::PathCost CSafestParhRater::CountRealRate( const Vector3& from, const PathLib::CPathLink* pathLink )
{				

	Vector nearestKnownEnemyPosition;
	if( !m_enemyAwareComponent || !m_enemyAwareComponent->GetNearestKnownPosition( nearestKnownEnemyPosition ) )
	{
		return pathLink->GetCost();
	}

	Float distance = pathLink->GetCost();
	distance *= DISTANCE_MULTIPLER;
	
	Vector to = pathLink->GetDestination()->GetPosition();	
	

	Int32 coversCount = CountCoveringCovers( from, to, MAX_COVERS, nearestKnownEnemyPosition );

	//coversCount = Min< Int32 >( coversCount, MAX_COVERS  );
	PathLib::PathCost cost;
	if( coversCount == 0 )
	{
		cost =  ( PathLib::PathCost )(distance * 2);
	}
	else if( coversCount == MAX_COVERS )
	{
		cost = ( PathLib::PathCost )( MIN_COST_MULTIPLER * distance );
	}
	else
	{
		cost = ( PathLib::PathCost )( distance - distance*( (( Float )coversCount)/MAX_COVERS ) ); //pathLink->GetCost() + coversCount>1 ? 0 : ( PathLib::PathCost )distance; //
	}	
	
	return cost;	
}

PathLib::PathCost CSafestParhRater::CountHeurusticRate( const Vector3& from, const Vector3& to )
{
	Float distance = ( Float ) PathLib::CalculatePathCost( ( from - to ).Mag() );
	distance *= DISTANCE_MULTIPLER;

	return  ( PathLib::PathCost )( MIN_COST_MULTIPLER * distance );
}
Int32 CSafestParhRater::CountCoveringCovers( const Vector& from, const Vector& to, Int32 coversLimit, const Vector& nearestKnownEnemyPosition )
{
	TDynArray< THandle< CNode > > coversInRange;
	FindCovers( from, to, coversInRange, coversLimit, nearestKnownEnemyPosition );

	Vector fromDirection	= from	- nearestKnownEnemyPosition;	
	Vector toDirection		= to	- nearestKnownEnemyPosition;

	fromDirection	.Normalize3();
	toDirection		.Normalize3();

	Vector fromMargin	= from	+ fromDirection * COVER_SEARCH_MARGIN;
	Vector toMargin		= to	+ toDirection	* COVER_SEARCH_MARGIN;

	Int32 coveringCoversCounter = 0;

	for( Uint32 i=0; i<coversInRange.Size(); ++i )
	{
		CNode* coverNode = coversInRange[i].Get();
		if( coverNode )
		{
			if( MathUtils::GeometryUtils::IsPointInsideTriangle( fromMargin, toMargin, nearestKnownEnemyPosition, coverNode->GetWorldPosition() ) )
			{
				CCoverPointComponent* cover = static_cast< CCoverPointComponent* >( coverNode );
				if( cover->IsCoverAgainst( nearestKnownEnemyPosition ) )
				{
					++coveringCoversCounter;
				}
			}
		}
	}
	
	return coveringCoversCounter;
}

void  CSafestParhRater::FindCovers( const Vector& from, const Vector& to, TDynArray< THandle< CNode > >& coveringCovers, Int32 coversLimit, const Vector& nearestKnownEnemyPosition )
{
	Vector minBound( 0, 0 , 0 );
	Vector maxBound( 0, 0 , 0 );

	minBound.X = Min< Float >( from.X, to.X, nearestKnownEnemyPosition.X ) - COVER_SEARCH_MARGIN;
	minBound.Y = Min< Float >( from.Y, to.Y, nearestKnownEnemyPosition.Y ) - COVER_SEARCH_MARGIN;
	minBound.Z = Min< Float >( from.Z, to.Z, nearestKnownEnemyPosition.Z ) - COVER_SEARCH_MARGIN;

	maxBound.X = Max< Float >( from.X, to.X, nearestKnownEnemyPosition.X ) + COVER_SEARCH_MARGIN;
	maxBound.Y = Max< Float >( from.Y, to.Y, nearestKnownEnemyPosition.Y ) + COVER_SEARCH_MARGIN;
	maxBound.Z = Max< Float >( from.Z, to.Z, nearestKnownEnemyPosition.Z ) + COVER_SEARCH_MARGIN;

	const Box bound( Vector( 0, 0, 0 ), maxBound - minBound );

	

	CCoversManager* coversManager = static_cast< CR6Game* >( GGame )->GetCoversManager();
	CNodesBinaryStorage* coversStorage = coversManager->GetCoverPointsStorage();

	coversStorage->Query( minBound, coveringCovers, bound, true, coversLimit, NULL, 0 );	
}