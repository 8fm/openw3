
#pragma once

#include "../core/engineQsTransform.h"
#include "skeletalAnimationEntry.h"

class CAnimPointCloudLookAtParam;
class CRenderFrame;

namespace AnimPointCloud
{
	typedef Vector		TPoint;
	typedef Vector2		TPointSC;

	typedef TDynArray< TDynArray< Int32 > > TPointToTrisMapping;


	struct Triangle
	{
		Int16 m_indices[3];

		friend IFile& operator<<( IFile& file, Triangle& t ) 
		{ 
			file << t.m_indices[ 0 ];
			file << t.m_indices[ 1 ];
			file << t.m_indices[ 2 ];
			return file; 
		}

		RED_INLINE Bool operator==( const Triangle& t ) const
		{
			return ( m_indices[ 0 ] == t.m_indices[ 0 ] || m_indices[ 0 ] == t.m_indices[ 1 ] || m_indices[ 0 ] == t.m_indices[ 2 ] ) &&
				( m_indices[ 1 ] == t.m_indices[ 0 ] || m_indices[ 1 ] == t.m_indices[ 1 ] || m_indices[ 1 ] == t.m_indices[ 2 ] ) &&
				( m_indices[ 2 ] == t.m_indices[ 0 ] || m_indices[ 2 ] == t.m_indices[ 1 ] || m_indices[ 2 ] == t.m_indices[ 2 ] );
		}

		RED_INLINE Bool Contains( Int32 index ) const
		{
			return index == m_indices[ 0 ] || index == m_indices[ 1 ] || index == m_indices[ 2 ];
		}
	};

	namespace Generators
	{
		class Sphere
		{
		public:
			struct Input
			{
				TDynArray< Matrix >		m_boneTransformsMS;
				Vector					m_directionLS;
			};

			struct OutData
			{
				TDynArray< Triangle >	m_tris;
				TDynArray< TPoint >		m_pointsBS;
				TPointToTrisMapping		m_pointToTriMapping;
			};

			// nn[0] - X N, nn[1] - Y P, nn[2] - X P, nn[3] - Y N
			static void FindNearestPoints( Int32 inputPoint, const TDynArray< AnimPointCloud::TPoint >& points, Int32 nn[4] );

			static Bool Generate( const Input& input, OutData& out );

		private:
			static Bool CreateTriangle( Int32 pA, Int32 pB, Int32 pC, Triangle& outTri );
			static void CheckTrisNormal( const Vector& boneMS, TDynArray< Triangle >& tris, const TDynArray< TPoint >& pointsMS );
		};

		class Grid
		{
		public:
			struct Input
			{
				TDynArray< Matrix >		m_boneTransformsMS;
				Vector					m_directionLS;
			};

			struct OutData
			{
				TDynArray< Triangle >	m_tris;
				TDynArray< TPoint >		m_pointsBS;
				TPointToTrisMapping		m_pointToTriMapping;
			};

			static Bool Generate( const Input& input, OutData& out );

		private:
			static void CheckTri( const Triangle& tri, Int32 numVerts );
		};
	}

	class SphereLookAt
	{
	public:
		typedef Int32		TQueryCookie;
		const static Int32	COOKIE_EMPTY = -1;

		struct Input
		{
			AnimVector4							m_pointWS;
			AnimQsTransform						m_boneMS;
			AnimQsTransform						m_localToWorld;
			const CAnimPointCloudLookAtParam*	m_params;
		};

		struct Output
		{
			Int32								m_poses[3];
			Float								m_weights[3];
			Vector								m_pointBS;
			Int32								m_nearestPoint;

			Output() 
				: m_pointBS( Vector::ZERO_3D_POINT )
				, m_nearestPoint( -1 )
			{
				for ( Uint32 i=0; i<3; ++i )
				{
					m_poses[ i ] = -1;
					m_weights[ i ] = 0.f;
				}
			}
		};

		static Bool DoLookAt( const Input& input, Output& output, TQueryCookie& inOutCookie );

		static TQueryCookie GenerateCookie();
	};

	namespace Render
	{
		void DrawPointCloud( CRenderFrame *frame, const Matrix& l2w, const Matrix& boneMS, const CAnimPointCloudLookAtParam* params );
		void DrawPointCloud( CRenderFrame *frame, const Matrix& boneWS, const CAnimPointCloudLookAtParam* params );
	}
};

class CAnimPointCloudLookAtParam : public ISkeletalAnimationSetEntryParam
{
	DECLARE_RTTI_SIMPLE_CLASS( CAnimPointCloudLookAtParam );

	CName									m_boneName;
	Vector									m_directionLS;
	Matrix									m_boneMSInv;
	EngineQsTransform						m_boneTransMSInv;

	TDynArray< AnimPointCloud::Triangle >	m_tris;
	TDynArray< AnimPointCloud::TPoint >		m_pointsBS;
	AnimPointCloud::TPointToTrisMapping		m_pointToTriMapping;

	TEngineQsTransformArray					m_refPose;

#ifndef NO_EDITOR
public:
	struct InitData
	{
		CName									m_boneName;
		Matrix									m_boneMS;
		Vector									m_directionLS;
		TDynArray< AnimPointCloud::Triangle >	m_tris;
		TDynArray< AnimPointCloud::TPoint >		m_pointsBS;
		AnimPointCloud::TPointToTrisMapping		m_pointToTriMapping;
		TEngineQsTransformArray					m_refPose;
	};

	void Init( InitData& data );
#endif

public:
	CAnimPointCloudLookAtParam();

	virtual void OnSerialize( IFile& file );

	virtual Bool EditorOnly() const override { return false; }

	RED_INLINE const Vector& GetDirectionLS() const { return m_directionLS; }
	RED_INLINE const CName& GetBoneName() const { return m_boneName; }

	RED_INLINE const TDynArray< AnimPointCloud::TPoint >& GetPointsBS() const { return m_pointsBS; }
	RED_INLINE const TDynArray< AnimPointCloud::Triangle >& GetTris() const { return m_tris; }
	RED_INLINE Bool HasPointToTriMapping( Int32 pointIdx ) const { return pointIdx != -1 && pointIdx < m_pointToTriMapping.SizeInt(); }
	RED_INLINE const TDynArray< Int32 >& GetPointToTriMapping( Int32 pointIdx ) const { return m_pointToTriMapping[ pointIdx ]; }
	RED_INLINE const TEngineQsTransformArray& GetReferencePose() const { return m_refPose; }

	void CalcMatrixBSToWS( const Matrix& boneMS, Matrix& out ) const;
	void TransformPointWSToBS( const AnimQsTransform& l2w, const AnimQsTransform& boneMS, const AnimVector4& pointWS, Vector& pointBS ) const;
};

BEGIN_CLASS_RTTI( CAnimPointCloudLookAtParam );
	PARENT_CLASS( ISkeletalAnimationSetEntryParam );
	PROPERTY( m_boneName );
	PROPERTY( m_boneMSInv );
	PROPERTY( m_boneTransMSInv );
	PROPERTY( m_directionLS );
	PROPERTY( m_pointsBS );
	PROPERTY( m_pointToTriMapping );
	PROPERTY( m_refPose );
END_CLASS_RTTI();
