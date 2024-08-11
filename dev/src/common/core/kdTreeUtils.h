
#pragma once

#include "kdTreeTypes.h"

#define KD_POINT_SWAP( a, b ) { typename TTree::TreeIdx tmp = idxbase[ idx + a ]; idxbase[ idx + a ] = idxbase[ idx + b ]; idxbase[ idx + b ] = tmp; }

namespace kdTreeUtils
{

	template< class TTree >
	typename TTree::Coord Spread( const typename TTree::PointSet& pa, typename TTree::TreeIdx* idxbase, typename TTree::TreeIdx idx, Int32 n, Int32 d )
	{
		typename TTree::Coord min = KD_POINT_VAL( 0, d );
		typename TTree::Coord max = KD_POINT_VAL( 0, d );
		for ( Int32 i = 1; i < n; ++i ) 
		{
			typename TTree::Coord c = KD_POINT_VAL( i, d );
			if ( c < min ) min = c;
			else if ( c > max ) max = c;
		}
		return ( max - min );
	}

	template< class TTree >
	Int32 MaxSpread( const typename TTree::PointSet& pa, typename TTree::TreeIdx* idxbase, typename TTree::TreeIdx idx, Int32 n )
	{
		const Int32 dim = pa.GetPointDim();
		Int32 maxDim = 0;
		typename TTree::Coord maxSpr = 0;

		if ( n == 0 )
		{
			return maxDim;
		}

		for ( Int32 d = 0; d < dim; d++ )
		{		
			typename TTree::Coord spr = Spread< TTree >( pa, idxbase, idx, n, d );
			if ( spr > maxSpr )
			{			
				maxSpr = spr;
				maxDim = d;
			}
		}
		return maxDim;
	}

	template< class TTree >
	void MedianSplit( const typename TTree::PointSet& pa,  typename TTree::TreeIdx* idxbase, typename TTree::TreeIdx idx, Int32 n, Int32 d, typename TTree::Coord& cv, int nLo )
	{
		Int32 l = 0;
		Int32 r = n - 1;

		while ( l < r ) 
		{
			Int32 i = ( r + l ) / 2;
			Int32 k;

			if ( KD_POINT_VAL( i, d ) > KD_POINT_VAL( r, d ) )
				KD_POINT_SWAP( i, r )
				KD_POINT_SWAP( l, i );

			typename TTree::Coord c = KD_POINT_VAL( l, d );
			i = l;
			k = r;

			while ( 1 ) 
			{
				while ( KD_POINT_VAL( ++i, d ) < c );
				while ( KD_POINT_VAL( --k, d ) > c );
				if ( i < k ) KD_POINT_SWAP( i, k ) else break;
			}

			KD_POINT_SWAP( l, k );

			if ( k > nLo )		r = k - 1;
			else if ( k < nLo )	l = k + 1;
			else break;
		}

		if ( nLo > 0 ) 
		{
			typename TTree::Coord c = KD_POINT_VAL( 0, d );
			Int32 k = 0;

			for ( Int32 i = 1; i < nLo; i++ ) 
			{
				if ( KD_POINT_VAL( i, d ) > c ) 
				{
					c = KD_POINT_VAL( i, d );
					k = i;
				}
			}

			KD_POINT_SWAP( nLo - 1, k );
		}

		cv = ( KD_POINT_VAL( nLo - 1, d ) + KD_POINT_VAL( nLo, d ) ) / ( typename TTree::Coord ) 2.f;
	}

	template< class TTree >
	void MinMax( const typename TTree::PointSet& pa, typename TTree::TreeIdx* idxbase, typename TTree::TreeIdx idx, Int32 n, Int32 d, typename TTree::Coord& min, typename TTree::Coord& max )
	{
		min = KD_POINT_VAL( 0, d );
		max = KD_POINT_VAL( 0, d );

		for ( Int32 i = 1; i < n; ++i ) 
		{
			typename TTree::Coord c = KD_POINT_VAL( i, d );
			if ( c < min ) min = c;
			else if ( c > max ) max = c;
		}
	}

	template< class TTree >
	void PlaneSplit( const typename TTree::PointSet& pa, typename TTree::TreeIdx* idxbase, typename TTree::TreeIdx idx, Int32 n, Int32 d, typename TTree::Coord cv, Int32& br1, Int32& br2 )
	{
		Int32 l = 0;
		Int32 r = n - 1;

		while ( 1 ) 
		{
			while ( l < n && KD_POINT_VAL( l, d ) < cv ) l++;
			while ( r >= 0 && KD_POINT_VAL( r, d ) >= cv ) r--;

			if ( l > r ) break;

			KD_POINT_SWAP( l, r );
			l++; r--;
		}

		br1 = l;
		r = n - 1;

		while ( 1 )
		{
			while ( l < n && KD_POINT_VAL( l, d ) <= cv ) l++;
			while ( r >= br1 && KD_POINT_VAL( r, d ) > cv ) r--;

			if ( l > r ) break;

			KD_POINT_SWAP( l, r );
			l++; r--;
		}

		br2 = l;
	}

	template< class TTree >
	typename TTree::Dist BoxDistance( const typename TTree::Point q, const typename TTree::Point lo, const typename TTree::Point hi, Int32 dim )
	{
		typename TTree::Dist dist = 0.0;
		typename TTree::Dist t;

		for ( Int32 d = 0; d < dim; ++d ) 
		{
			if ( q[ d ] < lo[ d ] ) 
			{
				t = typename TTree::Dist( lo[ d ] ) - typename TTree::Dist( q[ d ] );
				dist = KD_TREE_SUM( dist, KD_TREE_POW( t ) );
			}
			else if ( q[d] > hi[d] ) 
			{
				t = typename TTree::Dist( q[ d ]) - typename TTree::Dist( hi[ d ] );
				dist = KD_TREE_SUM( dist, KD_TREE_POW( t ) );
			}
		}

		return dist;
	}

	template< class TTree >
	Int32 SplitBalance( const typename TTree::PointSet& pa, typename TTree::TreeIdx* idxbase, typename TTree::TreeIdx idx, Int32 n, Int32 d, typename TTree::Coord cv )
	{
		Int32 nLo = 0;

		for( Int32 i = 0; i < n; ++i ) 
		{
			if ( KD_POINT_VAL( i, d ) < cv )
			{
				++nLo;
			}
		}

		return nLo - n/2;
	}

}
