
#include "kdTreeTypes.h"
#include "kdTreeUtils.h"

#pragma once

//////////////////////////////////////////////////////////////////////////

namespace kdTreeSplitters
{

	template < class TTree >
	class DefaultSplitter
	{
	public:
		static void Split (
			const typename TTree::PointSet&		pa,
			typename TTree::TreeIdx*			idxbase,
			typename TTree::TreeIdx				idx,
			const typename TTree::Point&		bndLo,
			const typename TTree::Point&		bndHi,
			Int32								n,
			Int32&								cutDim,
			typename TTree::Coord&				cutVal,
			Int32&								nLo )
		{
			cutDim = kdTreeUtils::MaxSpread< TTree >( pa, idxbase, idx, n );
			nLo = n / 2;

			kdTreeUtils::MedianSplit< TTree >( pa, idxbase, idx, n, cutDim, cutVal, nLo );
		}
	};

	template < class TTree >
	class SlMidptSplitter
	{
	public:
		static void Split (
			const typename TTree::PointSet&		pa,
			typename TTree::TreeIdx*			idxbase,
			typename TTree::TreeIdx				idx,
			const typename TTree::Point&		bndLo,
			const typename TTree::Point&		bndHi,
			Int32								n,
			Int32&								cutDim,
			typename TTree::Coord&				cutVal,
			Int32&								nLo )
		{
			const Int32 dim = pa.GetPointDim();
			Int32 d;

			typename TTree::Coord max_length = bndHi[ 0 ] - bndLo[ 0 ];

			for ( d = 1; d < dim; ++d ) 
			{
				typename TTree::Coord length = bndHi[ d ] - bndLo[ d ];
				if ( length > max_length ) 
				{
					max_length = length;
				}
			}

			const Float ERR	= 0.00001f;

			typename TTree::Coord max_spread = -1;
			for ( d = 0; d < dim; ++d ) 
			{
				if ( ( bndHi[ d ] - bndLo[ d ] ) >= ( 1 - ERR ) * max_length ) 
				{
					typename TTree::Coord spr = kdTreeUtils::Spread< TTree >( pa, idxbase, idx, n, d );

					if ( spr > max_spread ) 
					{
						max_spread = spr;
						cutDim = d;
					}
				}
			}
		
			typename TTree::Coord ideal_cut_val = ( bndLo[ cutDim ] + bndHi[ cutDim ])/2;

			typename TTree::Coord min, max;
			kdTreeUtils::MinMax< TTree >( pa, idxbase, idx, n, cutDim, min, max );

			if ( ideal_cut_val < min )
			{
				cutVal = min;
			}
			else if ( ideal_cut_val > max )
			{
				cutVal = max;
			}
			else
			{
				cutVal = ideal_cut_val;
			}

			int br1, br2;
			kdTreeUtils::PlaneSplit< TTree >( pa, idxbase, idx, n, cutDim, cutVal, br1, br2 );

			if ( ideal_cut_val < min )
			{ 
				nLo = 1;
			}
			else if ( ideal_cut_val > max ) 
			{
				nLo = n - 1;
			}
			else if ( br1 > n / 2 ) 
			{
				nLo = br1;
			}
			else if ( br2 < n / 2 ) 
			{
				nLo = br2;
			}
			else 
			{
				nLo = n / 2;
			}
		}
	};

	template < class TTree >
	class MidptSplit
	{
	public:
		static void Split (
			const typename TTree::PointSet&		pa,
			typename TTree::TreeIdx*			idxbase,
			typename TTree::TreeIdx				idx,
			const typename TTree::Point&		bndLo,
			const typename TTree::Point&		bndHi,
			Int32								n,
			Int32&								cutDim,
			typename TTree::Coord&				cutVal,
			Int32&								nLo )
		{
			const Int32 dim = pa.GetPointDim();
			int d;

			typename TTree::Coord max_length = bndHi[ 0 ] - bndLo[ 0 ];

			for ( d = 1; d < dim; ++d ) 
			{
				typename TTree::Coord length = bndHi[ d ] - bndLo[ d ];
				if ( length > max_length ) 
				{
					max_length = length;
				}
			}

			typename TTree::Coord max_spread = -1;

			for ( d = 0; d < dim; ++d ) 
			{
				const Float eps	= 0.00001f;

				if ( ( bndHi[ d ] - bndLo[ d ] ) >= ( 1 - eps ) * max_length ) 
				{
					typename TTree::Coord spr = kdTreeUtils::Spread< TTree >( pa, idxbase, idx, n, d );
					if ( spr > max_spread ) 
					{
						max_spread = spr;
						cutDim = d;
					}
				}
			}

			cutVal = ( bndLo[ cutDim ] + bndHi[ cutDim ]) / 2;

			int br1, br2;

			kdTreeUtils::PlaneSplit< TTree >( pa, idxbase, idx, n, cutDim, cutVal, br1, br2 );

			if ( br1 > n / 2 ) 
			{
				nLo = br1;
			}
			else if ( br2 < n / 2 ) 
			{
				nLo = br2;
			}
			else 
			{
				nLo = n / 2;
			}
		}
	};

	template < class TTree >
	class FairSplit
	{
	public:
		static void Split (
			const typename TTree::PointSet&		pa,
			typename TTree::TreeIdx*			idxbase,
			typename TTree::TreeIdx*			idx,
			const typename TTree::Point&		bndLo,
			const typename TTree::Point&		bndHi,
			Int32								n,
			Int32&								cutDim,
			typename TTree::Coord&				cutVal,
			Int32&								nLo )
		{
			const Int32 dim = pa.GetPointDim();
			int d;

			typename TTree::Coord max_length = bndHi[ 0 ] - bndLo[ 0 ];
			cutDim = 0;

			for ( d = 1; d < dim; ++d ) 
			{
				typename TTree::Coord length = bndHi[ d ] - bndLo[ d ];
				if ( length > max_length ) 
				{
					max_length = length;
					cutDim = d;
				}
			}

			const Float FS_ASPECT_RATIO = 3.0;

			typename TTree::Coord max_spread = 0;
			cutDim = 0;

			for ( d = 0; d < dim; ++d ) 
			{
				typename TTree::Coord length = bndHi[ d ] - bndLo[ d ];

				if ( ( ( double ) max_length ) * 2.0 / ( ( double ) length ) <= FS_ASPECT_RATIO ) 
				{
					typename TTree::Coord spr = kdTreeUtils::Spread< TTree >( pa, idxbase, idx, n, d );
					if (spr > max_spread) 
					{
						max_spread = spr;
						cutDim = d;
					}
				}
			}

			max_length = 0;
			for ( d = 0; d < dim; ++d ) 
			{
				typename TTree::Coord length = bndHi[ d ] - bndLo[ d ];

				if ( d != cutDim && length > max_length )
				{
					max_length = length;
				}
			}

			typename TTree::Coord small_piece = max_length / FS_ASPECT_RATIO;
			typename TTree::Coord lo_cut = bndLo[ cutDim ] + small_piece;
			typename TTree::Coord hi_cut = bndHi[ cutDim ] - small_piece;

			int br1, br2;

			if ( kdTreeUtils::SplitBalance< TTree >( pa, idxbase, idx, n, cutDim, lo_cut ) >= 0 ) 
			{
				cutVal = lo_cut;
				kdTreeUtils::PlaneSplit< TTree >( pa, idxbase, idx, n, cutDim, cutVal, br1, br2 );
				nLo = br1;
			}
			else if ( kdTreeUtils::SplitBalance< TTree >( pa, idxbase, idx, n, cutDim, hi_cut ) <= 0 ) 
			{
				cutVal = hi_cut;
				kdTreeUtils::PlaneSplit< TTree >( pa, idxbase, idx, n, cutDim, cutVal, br1, br2 );
				nLo = br2;
			}
			else 
			{
				nLo = n/2;
				kdTreeUtils::MedianSplit< TTree >( pa, idxbase, idx, n, cutDim, cutVal, nLo );
			}
		}
	};

	template < class TTree >
	class SlFairSplit
	{
	public:
		static void Split (
			const typename TTree::PointSet&		pa,
			typename TTree::TreeIdx*			idxbase,
			typename TTree::TreeIdx				idx,
			const typename TTree::Point&		bndLo,
			const typename TTree::Point&		bndHi,
			Int32								n,
			Int32&								cutDim,
			typename TTree::Coord&				cutVal,
			Int32&								nLo )
		{
			const Int32 dim = pa.GetPointDim();
			int d;

			typename TTree::Coord min, max;
			int br1, br2;

			typename TTree::Coord max_length = bndHi[ 0 ] - bndLo[ 0 ];
			cutDim = 0;

			for ( d = 1; d < dim; ++d ) 
			{
				typename TTree::Coord length = bndHi[ d ] - bndLo[ d ];
				if ( length	> max_length ) 
				{
					max_length = length;
					cutDim = d;
				}
			}

			const Float FS_ASPECT_RATIO = 3.0;

			typename TTree::Coord max_spread = 0;
			cutDim = 0;

			for ( d = 0; d < dim; ++d ) 
			{
				typename TTree::Coord length = bndHi[ d ] - bndLo[ d ];

				if ( ( ( Float ) max_length ) * 2.0 / ( ( Float ) length) <= FS_ASPECT_RATIO ) 
				{
					typename TTree::Coord spr = kdTreeUtils::Spread< TTree >( pa, idxbase, idx, n, d );
					if ( spr > max_spread ) 
					{
						max_spread = spr;
						cutDim = d;
					}
				}
			}

			max_length = 0;

			for ( d = 0; d < dim; ++d ) 
			{
				typename TTree::Coord length = bndHi[ d ] - bndLo[ d ];
				if ( d != cutDim && length > max_length )
				{
					max_length = length;
				}
			}

			typename TTree::Coord small_piece = max_length / FS_ASPECT_RATIO;
			typename TTree::Coord lo_cut = bndLo[ cutDim ] + small_piece;
			typename TTree::Coord hi_cut = bndHi[ cutDim ] - small_piece;

			kdTreeUtils::MinMax< TTree >( pa, idxbase, idx, n, cutDim, min, max );

			if ( kdTreeUtils::SplitBalance< TTree >( pa, idxbase, idx, n, cutDim, lo_cut ) >= 0 ) 
			{
				if ( max > lo_cut ) 
				{
					cutVal = lo_cut;
					kdTreeUtils::PlaneSplit< TTree >( pa, idxbase, idx, n, cutDim, cutVal, br1, br2 );
					nLo = br1;
				}
				else 
				{
					cutVal = max;
					kdTreeUtils::PlaneSplit< TTree >( pa, idxbase, idx, n, cutDim, cutVal, br1, br2 );
					nLo = n-1;
				}
			}
			else if (kdTreeUtils::SplitBalance< TTree >( pa, idxbase, idx, n, cutDim, hi_cut ) <= 0 ) 
			{
				if ( min < hi_cut ) 
				{
					cutVal = hi_cut;
					kdTreeUtils::PlaneSplit< TTree >( pa, idxbase, idx, n, cutDim, cutVal, br1, br2 );
					nLo = br2;
				}
				else 
				{
					cutVal = min;
					kdTreeUtils::PlaneSplit< TTree >( pa, idxbase, idx, n, cutDim, cutVal, br1, br2 );
					nLo = 1;
				}
			}
			else 
			{
				nLo = n / 2;
				kdTreeUtils::MedianSplit< TTree >( pa, idxbase, idx, n, cutDim, cutVal, nLo );
			}
		}

	};

};
