/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

namespace PathLib
{

namespace WalkableSpot
{

	template < class Base >
	CNavNode* PerformQuery( Base& base )
	{
		CPathLibWorld& pathlib = base.Query_GetPathLib();

		// setup test bbox
		Box& bbox = base.Query_GetBBox();
		Float& maxDist = base.Query_GetMaxDist();
		Vector destination = base.Query_GetDestination();

		auto funUpdateBBox =
			[ &bbox, &destination, &maxDist ] ()
		{
			Box cropBox( Vector( destination ), maxDist );
			bbox.Crop( cropBox );
		};

		funUpdateBBox();

		// consider if we have starting region
		CCentralNodeFinder::RegionAcceptor regionSetup;
		base.Query_SetupRegionAcceptor( regionSetup );
		if ( regionSetup.IsInvalid() )
		{
			return nullptr;
		}

		// base area test (as it is most obvious answer)
		CNavNode* bestNode = nullptr;

		auto funTestArea =
			[ &base, &bestNode, &funUpdateBBox, &regionSetup ] ( CAreaDescription* area ) -> Bool
		{
	
			if ( CNavNode* node = base.Query_FindClosestNode( area, regionSetup ) )
			{
				bestNode = node;
				funUpdateBBox();
				return true;
			}
			return false;
		};

		CAreaDescription* limitToBaseArea = base.Query_LimitToBaseArea();

		if ( limitToBaseArea )
		{
			funTestArea( limitToBaseArea );

			return bestNode;
		}

		Bool bailOutOnSuccess = base.Query_BailOutOnSuccess();

		// collect areas to iterate on
		CAreaDescription* areasList[ 16 ];
		Uint32 areasCount = pathlib.CollectAreasAt( bbox, areasList, 16, true );

		Bool areasListProcessPending[ 16 ];
		for ( Uint32 i = 0; i < areasCount; ++i )
		{
			areasListProcessPending[ i ] = true;
		}

		for ( Uint32 i = 0; i < areasCount; ++i )
		{
			if ( areasList[ i ]->GetBBox().Contains( destination ) )
			{
				if ( funTestArea( areasList[ i ] ) && bailOutOnSuccess )
				{
					return bestNode;
				}
				areasListProcessPending[ i ] = false;
			}
		}

		for ( Uint32 i = 0; i < areasCount; ++i )
		{
			if ( areasListProcessPending[ i ] && areasList[ i ]->GetBBox().Touches( bbox ) )
			{
				if ( funTestArea( areasList[ i ] ) && bailOutOnSuccess )
				{
					return bestNode;
				}
			}
		}

		return bestNode;
	}

};

};			// namespace PathLib

