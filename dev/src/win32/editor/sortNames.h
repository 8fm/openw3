#pragma once

namespace
{
	template < class TArrayType >
	static void SortNames( TArrayType& a )
	{
		struct Order
		{
			Bool operator()( CName p1, CName p2 ) const
			{
				const Bool p1IsDefault = p1 == CNAME( default );
				const Bool p2IsDefault = p2 == CNAME( default );
				if ( p1IsDefault != p2IsDefault )
				{
					return p1IsDefault;
				}

				return Red::System::StringCompare( p1.AsChar(), p2.AsChar() ) < 0;
			}
		};
		::Sort( a.Begin(), a.End(), Order() );
	};
};
