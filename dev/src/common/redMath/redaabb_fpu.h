/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

namespace Red
{
	namespace Math
	{
		namespace Fpu
		{
			class RedAABB
			{
				public:
					RedVector4 Min;
					RedVector4 Max;

					RED_INLINE RedAABB();
					RED_INLINE RedAABB( const RedVector4& _min, const RedVector4& _max );
					RED_INLINE RedAABB( const RedVector3& _min, const RedVector3& _max );
					
					RED_INLINE ~RedAABB();

					RED_INLINE RedAABB operator+( const RedVector4& _v ) const;
					RED_INLINE RedAABB operator+( const RedAABB& _aabb ) const;

					RED_INLINE RedAABB& operator+=( const RedVector4& _v );
					RED_INLINE RedAABB& operator+=( const RedAABB& _aabb );

					RED_INLINE bool OverlapsWith( const RedAABB& _box ) const;
					
					RED_INLINE bool Contains( const RedAABB& _box ) const;
					RED_INLINE bool ContainsPoint( const RedVector4& _v ) const;
					RED_FORCE_INLINE void AddPoint( const RedVector4& _v );
					RED_INLINE void AddAABB( const RedAABB& _aabb );

					RED_INLINE bool IsOk() const;
					RED_INLINE void SetEmpty();
					RED_INLINE bool IsEmpty() const;
			};
		};
	};
};