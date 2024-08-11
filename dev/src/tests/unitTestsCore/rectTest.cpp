#include "build.h"

#include "../../common/core/math.h"

TEST( RectTestsWorldSpace, RectInRectSameQuarters )
{
	{
		// Simple case - inner rectangle is completely inside outer rectangle
			
		Rect outer;
		Rect inner;
		Rect intersection;
		Bool areIntersecting;

		{
			// positive X, positive Y
			outer.m_left = 1;
			outer.m_right = 100;
			outer.m_top = 5;
			outer.m_bottom = 95;
				
			inner.m_left = 10;
			inner.m_right = 35;
			inner.m_top = 11;
			inner.m_bottom = 25;
				
			areIntersecting = Rect::GetIntersection( outer, inner, intersection );
			EXPECT_TRUE( areIntersecting );
			EXPECT_TRUE( intersection.m_left == inner.m_left );
			EXPECT_TRUE( intersection.m_right == inner.m_right );
			EXPECT_TRUE( intersection.m_top == inner.m_top );
			EXPECT_TRUE( intersection.m_bottom == inner.m_bottom );
		}

		{
			// negative X, positive Y
			outer.m_left = -100;
			outer.m_right = -1;
			outer.m_top = 5;
			outer.m_bottom = 95;

			inner.m_left = -35;
			inner.m_right = -10;
			inner.m_top = 11;
			inner.m_bottom = 25;

			areIntersecting = Rect::GetIntersection( outer, inner, intersection );
			EXPECT_TRUE( areIntersecting );
			EXPECT_TRUE( intersection.m_left == inner.m_left );
			EXPECT_TRUE( intersection.m_right == inner.m_right );
			EXPECT_TRUE( intersection.m_top == inner.m_top );
			EXPECT_TRUE( intersection.m_bottom == inner.m_bottom );
		}
			
		{
			// negative X, negative Y
			outer.m_left = -100;
			outer.m_right = -1;
			outer.m_top = -95;
			outer.m_bottom = -5;

			inner.m_left = -35;
			inner.m_right = -10;
			inner.m_top = -25;
			inner.m_bottom = -11;

			areIntersecting = Rect::GetIntersection( outer, inner, intersection );
			EXPECT_TRUE( areIntersecting );
			EXPECT_TRUE( intersection.m_left == inner.m_left );
			EXPECT_TRUE( intersection.m_right == inner.m_right );
			EXPECT_TRUE( intersection.m_top == inner.m_top );
			EXPECT_TRUE( intersection.m_bottom == inner.m_bottom );
		}

		{
			// positive X, negative Y
			outer.m_left = 1;
			outer.m_right = 100;
			outer.m_top = -95;
			outer.m_bottom = -5;

			inner.m_left = 10;
			inner.m_right = 35;
			inner.m_top = -25;
			inner.m_bottom = -11;

			areIntersecting = Rect::GetIntersection( outer, inner, intersection );
			EXPECT_TRUE( areIntersecting );
			EXPECT_TRUE( intersection.m_left == inner.m_left );
			EXPECT_TRUE( intersection.m_right == inner.m_right );
			EXPECT_TRUE( intersection.m_top == inner.m_top );
			EXPECT_TRUE( intersection.m_bottom == inner.m_bottom );
		}
	}
}

TEST( RectTestsWorldSpace, TestRectInRect_OutsideRectSpansAcrossDifferentQuarters )
{
	{
		Rect outer;
		Rect inner;
		Rect intersection;
		Bool areIntersecting;

		{
			// outside - positive Y
			outer.m_left = -100;
			outer.m_right = 100;
			outer.m_top = 5;
			outer.m_bottom = 95;

			// inner - positive X, positive Y
			inner.m_left = 10;
			inner.m_right = 35;
			inner.m_top = 11;
			inner.m_bottom = 25;

			areIntersecting = Rect::GetIntersection( outer, inner, intersection );
			EXPECT_TRUE( areIntersecting );
			EXPECT_TRUE( intersection.m_left == inner.m_left );
			EXPECT_TRUE( intersection.m_right == inner.m_right );
			EXPECT_TRUE( intersection.m_top == inner.m_top );
			EXPECT_TRUE( intersection.m_bottom == inner.m_bottom );

			// inner - negative X, positive Y
			inner.m_left = -35;
			inner.m_right = -10;
			inner.m_top = 11;
			inner.m_bottom = 25;

			areIntersecting = Rect::GetIntersection( outer, inner, intersection );
			EXPECT_TRUE( areIntersecting );
			EXPECT_TRUE( intersection.m_left == inner.m_left );
			EXPECT_TRUE( intersection.m_right == inner.m_right );
			EXPECT_TRUE( intersection.m_top == inner.m_top );
			EXPECT_TRUE( intersection.m_bottom == inner.m_bottom );

			// inner - spanned X, positive Y
			inner.m_left = -35;
			inner.m_right = 35;
			inner.m_top = 11;
			inner.m_bottom = 25;

			areIntersecting = Rect::GetIntersection( outer, inner, intersection );
			EXPECT_TRUE( areIntersecting );
			EXPECT_TRUE( intersection.m_left == inner.m_left );
			EXPECT_TRUE( intersection.m_right == inner.m_right );
			EXPECT_TRUE( intersection.m_top == inner.m_top );
			EXPECT_TRUE( intersection.m_bottom == inner.m_bottom );
		}

		{
			// outside - negative Y
			outer.m_left = -100;
			outer.m_right = 100;
			outer.m_top = -95;
			outer.m_bottom = -5;

			// inner - positive X, negative Y
			inner.m_left = 10;
			inner.m_right = 35;
			inner.m_top = -25;
			inner.m_bottom = -11;

			areIntersecting = Rect::GetIntersection( outer, inner, intersection );
			EXPECT_TRUE( areIntersecting );
			EXPECT_TRUE( intersection.m_left == inner.m_left );
			EXPECT_TRUE( intersection.m_right == inner.m_right );
			EXPECT_TRUE( intersection.m_top == inner.m_top );
			EXPECT_TRUE( intersection.m_bottom == inner.m_bottom );

			// inner - negative X, negative Y
			inner.m_left = -35;
			inner.m_right = -10;
			inner.m_top = -25;
			inner.m_bottom = -11;

			areIntersecting = Rect::GetIntersection( outer, inner, intersection );
			EXPECT_TRUE( areIntersecting );
			EXPECT_TRUE( intersection.m_left == inner.m_left );
			EXPECT_TRUE( intersection.m_right == inner.m_right );
			EXPECT_TRUE( intersection.m_top == inner.m_top );
			EXPECT_TRUE( intersection.m_bottom == inner.m_bottom );

			// inner - spanned X, negative Y
			inner.m_left = -35;
			inner.m_right = 35;
			inner.m_top = -25;
			inner.m_bottom = -11;

			areIntersecting = Rect::GetIntersection( outer, inner, intersection );
			EXPECT_TRUE( areIntersecting );
			EXPECT_TRUE( intersection.m_left == inner.m_left );
			EXPECT_TRUE( intersection.m_right == inner.m_right );
			EXPECT_TRUE( intersection.m_top == inner.m_top );
			EXPECT_TRUE( intersection.m_bottom == inner.m_bottom );
		}

		{
			// outside - positive X
			outer.m_left = 1;
			outer.m_right = 100;
			outer.m_top = -95;
			outer.m_bottom = 95;

			// inner - positive X, positive Y
			inner.m_left = 10;
			inner.m_right = 35;
			inner.m_top = 11;
			inner.m_bottom = 25;

			areIntersecting = Rect::GetIntersection( outer, inner, intersection );
			EXPECT_TRUE( areIntersecting );
			EXPECT_TRUE( intersection.m_left == inner.m_left );
			EXPECT_TRUE( intersection.m_right == inner.m_right );
			EXPECT_TRUE( intersection.m_top == inner.m_top );
			EXPECT_TRUE( intersection.m_bottom == inner.m_bottom );

			// inner - positive X, negative Y
			inner.m_left = 10;
			inner.m_right = 35;
			inner.m_top = -25;
			inner.m_bottom = -11;

			areIntersecting = Rect::GetIntersection( outer, inner, intersection );
			EXPECT_TRUE( areIntersecting );
			EXPECT_TRUE( intersection.m_left == inner.m_left );
			EXPECT_TRUE( intersection.m_right == inner.m_right );
			EXPECT_TRUE( intersection.m_top == inner.m_top );
			EXPECT_TRUE( intersection.m_bottom == inner.m_bottom );

			// inner - positive X, spanned Y
			inner.m_left = 10;
			inner.m_right = 35;
			inner.m_top = -25;
			inner.m_bottom = 25;

			areIntersecting = Rect::GetIntersection( outer, inner, intersection );
			EXPECT_TRUE( areIntersecting );
			EXPECT_TRUE( intersection.m_left == inner.m_left );
			EXPECT_TRUE( intersection.m_right == inner.m_right );
			EXPECT_TRUE( intersection.m_top == inner.m_top );
			EXPECT_TRUE( intersection.m_bottom == inner.m_bottom );
		}

		{
			// outside - negative X
			outer.m_left = -100;
			outer.m_right = -1;
			outer.m_top = -95;
			outer.m_bottom = 95;

			// inner - negative X, positive Y
			inner.m_left = -35;
			inner.m_right = -10;
			inner.m_top = 11;
			inner.m_bottom = 25;

			areIntersecting = Rect::GetIntersection( outer, inner, intersection );
			EXPECT_TRUE( areIntersecting );
			EXPECT_TRUE( intersection.m_left == inner.m_left );
			EXPECT_TRUE( intersection.m_right == inner.m_right );
			EXPECT_TRUE( intersection.m_top == inner.m_top );
			EXPECT_TRUE( intersection.m_bottom == inner.m_bottom );

			// inner - negative X, negative Y
			inner.m_left = -35;
			inner.m_right = -10;
			inner.m_top = -25;
			inner.m_bottom = -11;

			areIntersecting = Rect::GetIntersection( outer, inner, intersection );
			EXPECT_TRUE( areIntersecting );
			EXPECT_TRUE( intersection.m_left == inner.m_left );
			EXPECT_TRUE( intersection.m_right == inner.m_right );
			EXPECT_TRUE( intersection.m_top == inner.m_top );
			EXPECT_TRUE( intersection.m_bottom == inner.m_bottom );

			// inner - negative X, spanned Y
			inner.m_left = -35;
			inner.m_right = -10;
			inner.m_top = -25;
			inner.m_bottom = 25;

			areIntersecting = Rect::GetIntersection( outer, inner, intersection );
			EXPECT_TRUE( areIntersecting );
			EXPECT_TRUE( intersection.m_left == inner.m_left );
			EXPECT_TRUE( intersection.m_right == inner.m_right );
			EXPECT_TRUE( intersection.m_top == inner.m_top );
			EXPECT_TRUE( intersection.m_bottom == inner.m_bottom );
		}

		{
			// outside - spanned X, spanned Y
			outer.m_left = -100;
			outer.m_right = 100;
			outer.m_top = -95;
			outer.m_bottom = 95;

			// #1: inner - positive X, positive Y
			inner.m_left = -35;
			inner.m_right = -10;
			inner.m_top = 11;
			inner.m_bottom = 25;

			areIntersecting = Rect::GetIntersection( outer, inner, intersection );
			EXPECT_TRUE( areIntersecting );
			EXPECT_TRUE( intersection.m_left == inner.m_left );
			EXPECT_TRUE( intersection.m_right == inner.m_right );
			EXPECT_TRUE( intersection.m_top == inner.m_top );
			EXPECT_TRUE( intersection.m_bottom == inner.m_bottom );

			// #2: inner - negative X, positive Y
			inner.m_left = -35;
			inner.m_right = -10;
			inner.m_top = 11;
			inner.m_bottom = 25;

			areIntersecting = Rect::GetIntersection( outer, inner, intersection );
			EXPECT_TRUE( areIntersecting );
			EXPECT_TRUE( intersection.m_left == inner.m_left );
			EXPECT_TRUE( intersection.m_right == inner.m_right );
			EXPECT_TRUE( intersection.m_top == inner.m_top );
			EXPECT_TRUE( intersection.m_bottom == inner.m_bottom );

			// #3: inner - negative X, negative Y
			inner.m_left = -35;
			inner.m_right = -10;
			inner.m_top = -25;
			inner.m_bottom = -11;

			areIntersecting = Rect::GetIntersection( outer, inner, intersection );
			EXPECT_TRUE( areIntersecting );
			EXPECT_TRUE( intersection.m_left == inner.m_left );
			EXPECT_TRUE( intersection.m_right == inner.m_right );
			EXPECT_TRUE( intersection.m_top == inner.m_top );
			EXPECT_TRUE( intersection.m_bottom == inner.m_bottom );

			// #4: inner - positive X, negative Y
			inner.m_left = 10;
			inner.m_right = 35;
			inner.m_top = -25;
			inner.m_bottom = -11;

			areIntersecting = Rect::GetIntersection( outer, inner, intersection );
			EXPECT_TRUE( areIntersecting );
			EXPECT_TRUE( intersection.m_left == inner.m_left );
			EXPECT_TRUE( intersection.m_right == inner.m_right );
			EXPECT_TRUE( intersection.m_top == inner.m_top );
			EXPECT_TRUE( intersection.m_bottom == inner.m_bottom );

			// #5: inner - spanned X, positive Y
			inner.m_left = -35;
			inner.m_right = 35;
			inner.m_top = 11;
			inner.m_bottom = 25;

			areIntersecting = Rect::GetIntersection( outer, inner, intersection );
			EXPECT_TRUE( areIntersecting );
			EXPECT_TRUE( intersection.m_left == inner.m_left );
			EXPECT_TRUE( intersection.m_right == inner.m_right );
			EXPECT_TRUE( intersection.m_top == inner.m_top );
			EXPECT_TRUE( intersection.m_bottom == inner.m_bottom );

			// #6: inner - spanned X, negative Y
			inner.m_left = -35;
			inner.m_right = 35;
			inner.m_top = -25;
			inner.m_bottom = -11;

			areIntersecting = Rect::GetIntersection( outer, inner, intersection );
			EXPECT_TRUE( areIntersecting );
			EXPECT_TRUE( intersection.m_left == inner.m_left );
			EXPECT_TRUE( intersection.m_right == inner.m_right );
			EXPECT_TRUE( intersection.m_top == inner.m_top );
			EXPECT_TRUE( intersection.m_bottom == inner.m_bottom );

			// #7: inner - negative X, spanned Y
			inner.m_left = -35;
			inner.m_right = -10;
			inner.m_top = -25;
			inner.m_bottom = 25;

			areIntersecting = Rect::GetIntersection( outer, inner, intersection );
			EXPECT_TRUE( areIntersecting );
			EXPECT_TRUE( intersection.m_left == inner.m_left );
			EXPECT_TRUE( intersection.m_right == inner.m_right );
			EXPECT_TRUE( intersection.m_top == inner.m_top );
			EXPECT_TRUE( intersection.m_bottom == inner.m_bottom );

			// #8: inner - positive X, spanned Y
			inner.m_left = 10;
			inner.m_right = 35;
			inner.m_top = -25;
			inner.m_bottom = 25;

			areIntersecting = Rect::GetIntersection( outer, inner, intersection );
			EXPECT_TRUE( areIntersecting );
			EXPECT_TRUE( intersection.m_left == inner.m_left );
			EXPECT_TRUE( intersection.m_right == inner.m_right );
			EXPECT_TRUE( intersection.m_top == inner.m_top );
			EXPECT_TRUE( intersection.m_bottom == inner.m_bottom );

			// #9: inner - spanned X, spanned Y
			inner.m_left = -35;
			inner.m_right = 35;
			inner.m_top = -25;
			inner.m_bottom = 25;

			areIntersecting = Rect::GetIntersection( outer, inner, intersection );
			EXPECT_TRUE( areIntersecting );
			EXPECT_TRUE( intersection.m_left == inner.m_left );
			EXPECT_TRUE( intersection.m_right == inner.m_right );
			EXPECT_TRUE( intersection.m_top == inner.m_top );
			EXPECT_TRUE( intersection.m_bottom == inner.m_bottom );
		}
	}
}

TEST( RectTestsWorldSpace, TestRectIntersection_FirstQuarter )
{
	{
		Rect outer;
		Rect inner;
		Rect intersection;
		Bool areIntersecting;

		// outside
		outer.m_left = 50;
		outer.m_right = 75;
		outer.m_top = 60;
		outer.m_bottom = 95;

		{
			// #1: 
			inner.m_left = 40;
			inner.m_right = 60;
			inner.m_top = 50;
			inner.m_bottom = 70;

			areIntersecting = Rect::GetIntersection( outer, inner, intersection );
			EXPECT_TRUE( areIntersecting );
			EXPECT_TRUE( intersection.m_left == 50 );
			EXPECT_TRUE( intersection.m_right == 60 );
			EXPECT_TRUE( intersection.m_top == 60 );
			EXPECT_TRUE( intersection.m_bottom == 70 );
		}

		{
			// #2: 
			inner.m_left = 40;
			inner.m_right = 60;
			inner.m_top = 70;
			inner.m_bottom = 80;

			areIntersecting = Rect::GetIntersection( outer, inner, intersection );
			EXPECT_TRUE( areIntersecting );
			EXPECT_TRUE( intersection.m_left == 50 );
			EXPECT_TRUE( intersection.m_right == 60 );
			EXPECT_TRUE( intersection.m_top == 70 );
			EXPECT_TRUE( intersection.m_bottom == 80 );
		}

		{
			// #3: 
			inner.m_left = 40;
			inner.m_right = 60;
			inner.m_top = 80;
			inner.m_bottom = 100;

			areIntersecting = Rect::GetIntersection( outer, inner, intersection );
			EXPECT_TRUE( areIntersecting );
			EXPECT_TRUE( intersection.m_left == 50 );
			EXPECT_TRUE( intersection.m_right == 60 );
			EXPECT_TRUE( intersection.m_top == 80 );
			EXPECT_TRUE( intersection.m_bottom == 95 );
		}

		{
			// #4: 
			inner.m_left = 60;
			inner.m_right = 70;
			inner.m_top = 50;
			inner.m_bottom = 70;

			areIntersecting = Rect::GetIntersection( outer, inner, intersection );
			EXPECT_TRUE( areIntersecting );
			EXPECT_TRUE( intersection.m_left == 60 );
			EXPECT_TRUE( intersection.m_right == 70 );
			EXPECT_TRUE( intersection.m_top == 60 );
			EXPECT_TRUE( intersection.m_bottom == 70 );
		}

		{
			// #5: 
			inner.m_left = 60;
			inner.m_right = 70;
			inner.m_top = 80;
			inner.m_bottom = 100;

			areIntersecting = Rect::GetIntersection( outer, inner, intersection );
			EXPECT_TRUE( areIntersecting );
			EXPECT_TRUE( intersection.m_left == 60 );
			EXPECT_TRUE( intersection.m_right == 70 );
			EXPECT_TRUE( intersection.m_top == 80 );
			EXPECT_TRUE( intersection.m_bottom == 95 );
		}

		{
			// #6: 
			inner.m_left = 70;
			inner.m_right = 90;
			inner.m_top = 50;
			inner.m_bottom = 70;

			areIntersecting = Rect::GetIntersection( outer, inner, intersection );
			EXPECT_TRUE( areIntersecting );
			EXPECT_TRUE( intersection.m_left == 70 );
			EXPECT_TRUE( intersection.m_right == 75 );
			EXPECT_TRUE( intersection.m_top == 60 );
			EXPECT_TRUE( intersection.m_bottom == 70 );
		}

		{
			// #7: 
			inner.m_left = 70;
			inner.m_right = 90;
			inner.m_top = 70;
			inner.m_bottom = 80;

			areIntersecting = Rect::GetIntersection( outer, inner, intersection );
			EXPECT_TRUE( areIntersecting );
			EXPECT_TRUE( intersection.m_left == 70 );
			EXPECT_TRUE( intersection.m_right == 75 );
			EXPECT_TRUE( intersection.m_top == 70 );
			EXPECT_TRUE( intersection.m_bottom == 80 );
		}

		{
			// #8: 
			inner.m_left = 70;
			inner.m_right = 90;
			inner.m_top = 80;
			inner.m_bottom = 100;

			areIntersecting = Rect::GetIntersection( outer, inner, intersection );
			EXPECT_TRUE( areIntersecting );
			EXPECT_TRUE( intersection.m_left == 70 );
			EXPECT_TRUE( intersection.m_right == 75 );
			EXPECT_TRUE( intersection.m_top == 80 );
			EXPECT_TRUE( intersection.m_bottom == 95 );
		}
	}
}

TEST( RectTestsWorldSpace, TestRectIntersection_ThirdQuarter )
{
	{
		Rect outer;
		Rect inner;
		Rect intersection;
		Bool areIntersecting;

		// outside
		outer.m_left = 50;
		outer.m_right = 75;
		outer.m_top = 60;
		outer.m_bottom = 95;

		{
			// #1: 
			inner.m_left = 40;
			inner.m_right = 60;
			inner.m_top = 50;
			inner.m_bottom = 70;

			areIntersecting = Rect::GetIntersection( outer, inner, intersection );
			EXPECT_TRUE( areIntersecting );
			EXPECT_TRUE( intersection.m_left == 50 );
			EXPECT_TRUE( intersection.m_right == 60 );
			EXPECT_TRUE( intersection.m_top == 60 );
			EXPECT_TRUE( intersection.m_bottom == 70 );
		}

		{
			// #2: 
			inner.m_left = 40;
			inner.m_right = 60;
			inner.m_top = 70;
			inner.m_bottom = 80;

			areIntersecting = Rect::GetIntersection( outer, inner, intersection );
			EXPECT_TRUE( areIntersecting );
			EXPECT_TRUE( intersection.m_left == 50 );
			EXPECT_TRUE( intersection.m_right == 60 );
			EXPECT_TRUE( intersection.m_top == 70 );
			EXPECT_TRUE( intersection.m_bottom == 80 );
		}

		{
			// #3: 
			inner.m_left = 40;
			inner.m_right = 60;
			inner.m_top = 80;
			inner.m_bottom = 100;

			areIntersecting = Rect::GetIntersection( outer, inner, intersection );
			EXPECT_TRUE( areIntersecting );
			EXPECT_TRUE( intersection.m_left == 50 );
			EXPECT_TRUE( intersection.m_right == 60 );
			EXPECT_TRUE( intersection.m_top == 80 );
			EXPECT_TRUE( intersection.m_bottom == 95 );
		}

		{
			// #4: 
			inner.m_left = 60;
			inner.m_right = 70;
			inner.m_top = 50;
			inner.m_bottom = 70;

			areIntersecting = Rect::GetIntersection( outer, inner, intersection );
			EXPECT_TRUE( areIntersecting );
			EXPECT_TRUE( intersection.m_left == 60 );
			EXPECT_TRUE( intersection.m_right == 70 );
			EXPECT_TRUE( intersection.m_top == 60 );
			EXPECT_TRUE( intersection.m_bottom == 70 );
		}

		{
			// #5: 
			inner.m_left = 60;
			inner.m_right = 70;
			inner.m_top = 80;
			inner.m_bottom = 100;

			areIntersecting = Rect::GetIntersection( outer, inner, intersection );
			EXPECT_TRUE( areIntersecting );
			EXPECT_TRUE( intersection.m_left == 60 );
			EXPECT_TRUE( intersection.m_right == 70 );
			EXPECT_TRUE( intersection.m_top == 80 );
			EXPECT_TRUE( intersection.m_bottom == 95 );
		}

		{
			// #6: 
			inner.m_left = 70;
			inner.m_right = 90;
			inner.m_top = 50;
			inner.m_bottom = 70;

			areIntersecting = Rect::GetIntersection( outer, inner, intersection );
			EXPECT_TRUE( areIntersecting );
			EXPECT_TRUE( intersection.m_left == 70 );
			EXPECT_TRUE( intersection.m_right == 75 );
			EXPECT_TRUE( intersection.m_top == 60 );
			EXPECT_TRUE( intersection.m_bottom == 70 );
		}

		{
			// #7: 
			inner.m_left = 70;
			inner.m_right = 90;
			inner.m_top = 70;
			inner.m_bottom = 80;

			areIntersecting = Rect::GetIntersection( outer, inner, intersection );
			EXPECT_TRUE( areIntersecting );
			EXPECT_TRUE( intersection.m_left == 70 );
			EXPECT_TRUE( intersection.m_right == 75 );
			EXPECT_TRUE( intersection.m_top == 70 );
			EXPECT_TRUE( intersection.m_bottom == 80 );
		}

		{
			// #8: 
			inner.m_left = 70;
			inner.m_right = 90;
			inner.m_top = 80;
			inner.m_bottom = 100;

			areIntersecting = Rect::GetIntersection( outer, inner, intersection );
			EXPECT_TRUE( areIntersecting );
			EXPECT_TRUE( intersection.m_left == 70 );
			EXPECT_TRUE( intersection.m_right == 75 );
			EXPECT_TRUE( intersection.m_top == 80 );
			EXPECT_TRUE( intersection.m_bottom == 95 );
		}
	}
}
