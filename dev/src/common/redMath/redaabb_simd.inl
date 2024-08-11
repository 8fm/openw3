/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

namespace RedMath
{
	namespace SIMD
	{
		RedAABB::RedAABB( const RedVector4& _min, const RedVector4& _max )
			: Min( _min )
			, Max( _max )
		{
		}

		RedAABB::RedAABB( const RedVector3& _min, const RedVector3& _max )
			: Min( _min )
			, Max( _max )
		{
		}

		RedAABB::RedAABB()
		{
		}

		RedAABB::~RedAABB()
		{
		}

		bool RedAABB::OverlapsWith( const RedAABB& _box ) const
		{
			bool minCmp = false;
			bool maxCmp = false;
			if( ( Min.X <= _box.Max.X ) && ( Min.Y <= _box.Max.Y ) && ( Min.Z <= _box.Max.Z ) )
			{
				minCmp = true;
			}
			if( ( _box.Min.X <= Max.X ) && ( _box.Min.Y <= Max.Y ) && ( _box.Min.Z <= Max.Z ) )
			{
				maxCmp = true;
			}
			return ( minCmp && maxCmp );
		}

		
		bool RedAABB::Contains( const RedAABB& _box ) const
		{
			bool minCmp = false;
			bool maxCmp = false;
			if( ( Min.X <= _box.Min.X ) && ( Min.Y <= _box.Min.Y ) && ( Min.Z <= _box.Min.Z ) )
			{
				minCmp = true;
			}
			if( ( _box.Max.X <= Max.X ) && ( _box.Max.Y <= Max.Y ) && ( _box.Max.Z <= Max.Z ) )
			{
				maxCmp = true;
			}
			return ( minCmp && maxCmp );
		}

		bool RedAABB::ContainsPoint( const RedVector4& _v ) const
		{
			bool minCmp = false;
			bool maxCmp = false;
			if( ( Min.X <= _v.X ) && ( Min.Y <= _v.Y ) && ( Min.Z <= _v.Z ) )
			{
				minCmp = true;
			}
			if( ( _v.X <= Max.X ) && ( _v.Y <= Max.Y ) && ( _v.Z <= Max.Z ) )
			{
				maxCmp = true;
			}
			return ( minCmp && maxCmp );
		}

		void RedAABB::AddPoint(const RedVector4& _v )
		{
			RedMath::SIMD::Min(Min, Min, _v);
			RedMath::SIMD::Max(Max, Max, _v);
		}

		void RedAABB::AddAABB( const RedAABB& _aabb )
		{
			Min = RedMath::SIMD::Min( Min, _aabb.Min );
			Max = RedMath::SIMD::Max( Max, _aabb.Max );
		}
		
		bool RedAABB::IsOk() const
		{
			if( Min.IsOk() && Max.IsOk() &&
				Min.X <= Max.X && Min.Y <= Max.Y && Min.Z <= Max.Z)
			{
				return true;
			}

			return false;
		}

		void RedAABB::SetEmpty()
		{
			Min.Set( FLT_MAX );
			Max.Set( -FLT_MAX );
		}

		bool RedAABB::IsEmpty() const
		{
			return (Min.X > Max.X || Min.Y > Max.Y || Min.Z > Max.Z);
		}

	};
};
