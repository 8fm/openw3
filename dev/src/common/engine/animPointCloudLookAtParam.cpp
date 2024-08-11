
#include "build.h"
#include "animPointCloudLookAtParam.h"
#include "behaviorGraphUtils.inl"
#include "../core/mathUtils.h"
#include "skeletalAnimationEntry.h"
#include "behaviorIncludes.h"
#include "renderVertices.h"

//#pragma optimize("",off)

IMPLEMENT_ENGINE_CLASS( CAnimPointCloudLookAtParam );

namespace AnimPointCloud
{
	void Generators::Sphere::FindNearestPoints( Int32 inputPoint, const TDynArray< AnimPointCloud::TPoint >& points, Int32 nn[4] )
	{
		Int32 bestIdx_X_P =  -1;
		Int32 bestIdx_X_N =  -1;
		Int32 bestIdx_Y_P =  -1;
		Int32 bestIdx_Y_N =  -1;

		Float bestVal_X_P = NumericLimits< Float >::Max();
		Float bestVal_X_N = -NumericLimits< Float >::Max();
		Float bestVal_Y_P = NumericLimits< Float >::Max();
		Float bestVal_Y_N = -NumericLimits< Float >::Max();

		const Float THRESHOLD = 2.5f;

		const AnimPointCloud::TPoint& iPoint = points[ inputPoint ];

		AnimVector4 iVec( iPoint.X, iPoint.Y, iPoint.Z, 1.f );
		BehaviorUtils::SphericalFromCartesian( iVec );

		iVec.Y = RAD2DEG( iVec.Y );
		iVec.Z = RAD2DEG( iVec.Z );

		const Int32 numPoints = points.SizeInt();
		for ( Int32 i=0; i<numPoints; ++i )
		{
			if ( i != inputPoint )
			{
				const AnimPointCloud::TPoint& sPoint = points[ i ];

				AnimVector4 vec( sPoint.X, sPoint.Y, sPoint.Z, 1.f );

				BehaviorUtils::SphericalFromCartesian( vec );

				vec.Y = RAD2DEG( vec.Y );
				vec.Z = RAD2DEG( vec.Z );

				const Bool hasDiff_X = MAbs( EulerAngles::AngleDistance( vec.Y, iVec.Y ) ) > THRESHOLD;
				const Bool hasDiff_Y = MAbs( EulerAngles::AngleDistance( vec.Z, iVec.Z ) ) > THRESHOLD;

				if ( !hasDiff_X )
				{
					// Test Y
					const Float dist = EulerAngles::AngleDistance( iVec.Z, vec.Z );
					if ( dist < 0.f && dist > bestVal_Y_N )
					{
						bestVal_Y_N = dist;
						bestIdx_Y_N = i;
					}
					else if ( dist > 0.f && dist < bestVal_Y_P )
					{
						bestVal_Y_P = dist;
						bestIdx_Y_P = i;
					}
				}
				else if ( !hasDiff_Y )
				{
					// Test X
					const Float dist = EulerAngles::AngleDistance( iVec.Y, vec.Y );
					if ( dist < 0.f && dist > bestVal_X_N )
					{
						bestVal_X_N = dist;
						bestIdx_X_N = i;
					}
					else if ( dist > 0.f && dist < bestVal_X_P )
					{
						bestVal_X_P = dist;
						bestIdx_X_P = i;
					}
				}
			}
		}

		ASSERT( bestIdx_X_N != inputPoint );
		ASSERT( bestIdx_Y_N != inputPoint );
		ASSERT( bestIdx_X_P != inputPoint );
		ASSERT( bestIdx_Y_P != inputPoint );

		nn[0] = bestIdx_X_N;
		nn[1] = bestIdx_Y_P;
		nn[2] = bestIdx_X_P;
		nn[3] = bestIdx_Y_N;
	}

	Bool Generators::Sphere::CreateTriangle( Int32 pA, Int32 pB, Int32 pC, Triangle& outTri )
	{
		if ( pA != -1 && pB != -1 && pC != -1 )
		{
			ASSERT( pA != pB );
			ASSERT( pB != pC );
			ASSERT( pA != pC );

			outTri.m_indices[ 0 ] = (Int16)pA;
			outTri.m_indices[ 1 ] = (Int16)pB;
			outTri.m_indices[ 2 ] = (Int16)pC;

			return true;
		}

		return false;
	}

	void Generators::Sphere::CheckTrisNormal( const Vector& boneMS, TDynArray< Triangle >& tris, const TDynArray< TPoint >& pointsMS )
	{
		const Uint32 numTris = tris.Size();
		for ( Uint32 i=0; i<numTris; ++i )
		{
			Triangle& tri = tris[ i ];

			ASSERT( tri.m_indices[ 0 ] != tri.m_indices[ 1 ] );
			ASSERT( tri.m_indices[ 0 ] != tri.m_indices[ 2 ] );
			ASSERT( tri.m_indices[ 1 ] != tri.m_indices[ 2 ] );

			const TPoint& pointA = pointsMS[ tri.m_indices[ 0 ] ];
			const TPoint& pointB = pointsMS[ tri.m_indices[ 1 ] ];
			const TPoint& pointC = pointsMS[ tri.m_indices[ 2 ] ];

			Vector dirBoneToTri = pointA - boneMS;
			dirBoneToTri.Normalize3();

			const Vector triNormal = MathUtils::GeometryUtils::GetTriangleNormal( pointA, pointB, pointC );

			const Float dot = Vector::Dot3( dirBoneToTri, triNormal );

			if ( dot > 0.f )
			{
				Swap( tri.m_indices[ 1 ], tri.m_indices[ 2 ] );
			}
			
			ASSERT( tri.m_indices[ 0 ] != tri.m_indices[ 1 ] );
			ASSERT( tri.m_indices[ 0 ] != tri.m_indices[ 2 ] );
			ASSERT( tri.m_indices[ 1 ] != tri.m_indices[ 2 ] );
		}
	}

	Bool Generators::Sphere::Generate( const Generators::Sphere::Input& input, Generators::Sphere::OutData& out )
	{
		const Uint32 size = input.m_boneTransformsMS.Size();
		if ( size == 0 )
		{
			return false;
		}

		const Vector directionLS = input.m_directionLS.Normalized3();

		for ( Uint32 i=0; i<size; ++i )
		{
			const Matrix& boneMS = input.m_boneTransformsMS[ i ];

			const Vector directionMS = boneMS.TransformVector( directionLS );

			Bool found = false;
			for ( Uint32 j=0; j<out.m_pointsBS.Size(); ++j )
			{
				if ( Vector::Near3( out.m_pointsBS[ j ], directionMS ) )
				{
					found = true;
					break;
				}
			}

			if ( found )
			{
				continue;
			}

			out.m_pointsBS.PushBack( directionMS );
		}

		const Vector bonePosition = input.m_boneTransformsMS[ 0 ].GetTranslation();

		const Int32 numPoints = out.m_pointsBS.SizeInt();
		for ( Int32 i=0; i<numPoints; ++i )
		{
			Int32 nn[4];

			FindNearestPoints( i, out.m_pointsBS, nn );

			AnimPointCloud::Triangle tri;

			//for ( Uint32 j=0; j<4; ++j )
			//{
			//	const Uint32 next = j<3 ? j+1 : 0;
			//
			//	if ( CreateTriangle( i, nn[j], nn[next], tri ) && !out.m_tris.Exist( tri ) /*&& CheckTri( bonePosition, tri, out.m_pointsMS )*/ )
			//	{
			//		out.m_tris.PushBack( tri );
			//	}
			//}

			if ( CreateTriangle( i, nn[0], nn[1], tri ) && !out.m_tris.Exist( tri ) )
			{
				out.m_tris.PushBack( tri );
			}
			if ( CreateTriangle( i, nn[2], nn[3], tri ) && !out.m_tris.Exist( tri ) )
			{
				out.m_tris.PushBack( tri );
			}
		}

		CheckTrisNormal( bonePosition, out.m_tris, out.m_pointsBS );

		out.m_pointToTriMapping.Resize( numPoints );

		const Uint32 numTris = out.m_tris.Size();

		for ( Int32 i=0; i<numPoints; ++i )
		{
			for ( Uint32 j=0; j<numTris; ++j )
			{
				const Triangle& tri = out.m_tris[ j ];

				if ( tri.Contains( i ) )
				{
					out.m_pointToTriMapping[ i ].PushBack( j );
				}
			}

			ASSERT( out.m_pointToTriMapping[ i ].Size() > 0 );
			ASSERT( out.m_pointToTriMapping[ i ].Size() <= 6 );
		}

		return true;
	}

	//////////////////////////////////////////////////////////////////////////

	Bool Generators::Grid::Generate( const Generators::Grid::Input& input, Generators::Grid::OutData& out )
	{
		const Uint32 size = input.m_boneTransformsMS.Size();
		if ( size == 0 )
		{
			return false;
		}

		const Vector directionLS = input.m_directionLS.Normalized3();

		for ( Uint32 i=0; i<size; ++i )
		{
			const Matrix& boneMS = input.m_boneTransformsMS[ i ];

			const Vector directionMS = boneMS.TransformVector( directionLS );

			Bool found = false;
			for ( Uint32 j=0; j<out.m_pointsBS.Size(); ++j )
			{
				if ( Vector::Near3( out.m_pointsBS[ j ], directionMS ) )
				{
					found = true;
					break;
				}
			}

			if ( found )
			{
				continue;
			}

			out.m_pointsBS.PushBack( directionMS );
		}

		const Int32 numPoints = out.m_pointsBS.SizeInt();

		{
			// Check grid size
			Int32 numPointInRow = 0;
			Int32 numPointInColumn = 0;
			{
				if ( numPoints > 0 )
				{
					const Float THRESHOLD = 0.12f; // 12cm

					const AnimPointCloud::TPoint& refPoint = out.m_pointsBS[ 0 ];

					for ( Int32 i=0; i<numPoints; ++i )
					{
						const AnimPointCloud::TPoint& testPoint = out.m_pointsBS[ i ];

						const Bool hasDiff = MAbs( refPoint.Z - testPoint.Z ) > THRESHOLD;
						if ( hasDiff )
						{
							break;
						}

						numPointInRow++;
					}
				}
			}

			if ( numPointInRow > 0 )
			{
				numPointInColumn = numPoints / numPointInRow;
			}

			ASSERT( numPointInColumn * numPointInRow == numPoints );

			// Create triangles
			{
				for ( Int32 i=0; i<numPointInColumn; ++i )
				{
					for ( Int32 j=0; j<numPointInRow; ++j )
					{
						if ( i + 1 < numPointInColumn && j + 1 < numPointInRow )
						{
							{
								AnimPointCloud::Triangle tri;
								tri.m_indices[ 0 ] = (Int16)(numPointInRow*(i+0) + (j+0));
								tri.m_indices[ 1 ] = (Int16)(numPointInRow*(i+1) + (j+0));
								tri.m_indices[ 2 ] = (Int16)(numPointInRow*(i+0) + (j+1));

								CheckTri( tri, numPoints );

								out.m_tris.PushBack( tri );
							}

							{
								AnimPointCloud::Triangle tri;
								tri.m_indices[ 0 ] = (Int16)(numPointInRow*(i+1) + (j+0));
								tri.m_indices[ 1 ] = (Int16)(numPointInRow*(i+1) + (j+1));
								tri.m_indices[ 2 ] = (Int16)(numPointInRow*(i+0) + (j+1));

								CheckTri( tri, numPoints );

								out.m_tris.PushBack( tri );
							}
						}
					}
				}
			}
		}

		out.m_pointToTriMapping.Resize( numPoints );

		const Uint32 numTris = out.m_tris.Size();

		for ( Int32 i=0; i<numPoints; ++i )
		{
			for ( Uint32 j=0; j<numTris; ++j )
			{
				const Triangle& tri = out.m_tris[ j ];

				CheckTri( tri, numPoints );

				if ( tri.Contains( i ) )
				{
					out.m_pointToTriMapping[ i ].PushBack( j );
				}
			}

			ASSERT( out.m_pointToTriMapping[ i ].Size() > 0 );
			ASSERT( out.m_pointToTriMapping[ i ].Size() <= 6 );
		}

		return true;
	}

	void Generators::Grid::CheckTri( const AnimPointCloud::Triangle& tri, Int32 numVerts )
	{
		ASSERT( !tri.Contains( -1 ) );
		ASSERT( tri.m_indices[ 0 ] < numVerts );
		ASSERT( tri.m_indices[ 1 ] < numVerts );
		ASSERT( tri.m_indices[ 2 ] < numVerts );
	}

	//////////////////////////////////////////////////////////////////////////

	SphereLookAt::TQueryCookie SphereLookAt::GenerateCookie()
	{
		return COOKIE_EMPTY;
	}

	Bool SphereLookAt::DoLookAt( const Input& input, Output& output, TQueryCookie& inOutCookie )
	{
		// 1. Convert point WS to BS
		Vector pointBS;
		input.m_params->TransformPointWSToBS( input.m_localToWorld, input.m_boneMS, input.m_pointWS, pointBS );

		output.m_pointBS = pointBS;

		// 3. Check last query
		const TDynArray< AnimPointCloud::Triangle >& tris = input.m_params->GetTris();
		const TDynArray< AnimPointCloud::TPoint >& cloudPoints = input.m_params->GetPointsBS();

		if ( inOutCookie != COOKIE_EMPTY )
		{
			//const AnimPointCloud::Triangle& tri = tris[ inOutCookie ];

			//const Vector& pointA = cloudPoints[ tri.m_indices[ 0 ] ];
			//const Vector& pointB = cloudPoints[ tri.m_indices[ 1 ] ];
			//const Vector& pointC = cloudPoints[ tri.m_indices[ 2 ] ];
		}

		// 4. Find nearest point
		Int32 nearestPoint = -1;
		Float nearestPointDistVal = NumericLimits< Float >::Max();

		const Int32 numCloudPoints = cloudPoints.SizeInt();
		for ( Int32 i=0; i<numCloudPoints; ++i )
		{
			// This is not mathematically correct but for us should be ok
			const Float dist2 = pointBS.DistanceSquaredTo( cloudPoints[ i ] );
			if ( dist2 < nearestPointDistVal )
			{
				nearestPoint = i;
				nearestPointDistVal = dist2;
			}
		}

		output.m_nearestPoint = nearestPoint;

		if ( nearestPoint != -1 && input.m_params->HasPointToTriMapping( nearestPoint ) )
		{
			// 5a. Find triangle
			const TDynArray< Int32 >& pointsTris = input.m_params->GetPointToTriMapping( nearestPoint );
			const Int32 numPointsTris = pointsTris.SizeInt();
			for ( Int32 i=0; i<numPointsTris; ++i )
			{
				const Int32 index = pointsTris[ i ];
				const AnimPointCloud::Triangle& tri = tris[ index ];

				const Vector& pointA = cloudPoints[ tri.m_indices[ 0 ] ];
				const Vector& pointB = cloudPoints[ tri.m_indices[ 1 ] ];
				const Vector& pointC = cloudPoints[ tri.m_indices[ 2 ] ];

				Float u = 0.f;
				Float v = 0.f;
				if ( MathUtils::GeometryUtils::IsPointInsideTriangle_UV( pointA, pointB, pointC, pointBS, u, v ) )
				{
					// 6. Calc output
					output.m_poses[ 0 ] = tri.m_indices[ 0 ];
					output.m_poses[ 1 ] = tri.m_indices[ 1 ];
					output.m_poses[ 2 ] = tri.m_indices[ 2 ];

					output.m_weights[ 1 ] = u;
					output.m_weights[ 2 ] = v;
					output.m_weights[ 0 ] = 1.f - ( u + v );

					// 7. Save cookie
					inOutCookie = index;

					return true;
				}
			}

			// 5b. Find edge
			Float bestDist = NumericLimits< Float >::Max();
			Uint32 bestIndex = 0;
			Int32 bestTri = -1;
			Float bestU = 0.f;

			for ( Int32 i=0; i<numPointsTris; ++i )
			{
				const Int32 index = pointsTris[ i ];
				const AnimPointCloud::Triangle& tri = tris[ index ];

				for ( Uint32 j=0; j<3; ++j )
				{
					const Uint32 next = j==2 ? 0 : j+1;

					const Vector& pointA = cloudPoints[ tri.m_indices[ j ] ];
					const Vector& pointB = cloudPoints[ tri.m_indices[ next ] ];

					const Float u = MathUtils::GeometryUtils::ProjectVecOnEdge( pointBS, pointA, pointB );
					const Vector pointAB = Vector::Interpolate( pointA, pointB, u );
					const Float dist = pointAB.DistanceSquaredTo( pointBS );

					if ( dist < bestDist )
					{
						bestDist = dist;
						bestIndex = j;
						bestU = u;
						bestTri = i;
					}
				}
			}

			if ( bestTri != -1 )
			{
				const Int32 triIndex = pointsTris[ bestTri ];
				const AnimPointCloud::Triangle& tri = tris[ triIndex ];

				const Uint32 next = bestIndex==2 ? 0 : bestIndex+1;

				// 6. Calc output
				output.m_poses[ 0 ] = tri.m_indices[ bestIndex ];
				output.m_poses[ 1 ] = tri.m_indices[ next ];
				output.m_poses[ 2 ] = -1;

				output.m_weights[ 1 ] = bestU;
				output.m_weights[ 0 ] = 1.f - bestU;
				output.m_weights[ 2 ] = 0.f;

				// 7. Save cookie
				inOutCookie = bestTri;

				return true;
			}
		}

		return false;
	}

	void Render::DrawPointCloud( CRenderFrame *frame, const Matrix& l2w, const Matrix& boneMS, const CAnimPointCloudLookAtParam* params )
	{
		Matrix boneWS = Matrix::Mul( l2w, boneMS );

		DrawPointCloud( frame, boneWS, params );
	}

	void Render::DrawPointCloud( CRenderFrame *frame, const Matrix& boneWS, const CAnimPointCloudLookAtParam* params )
	{
		const TDynArray< AnimPointCloud::TPoint >& points = params->GetPointsBS();
		const TDynArray< AnimPointCloud::Triangle >& tris = params->GetTris();

		Box box( Vector::ZERO_3D_POINT, 0.01f );
		Matrix mat( Matrix::IDENTITY );
		Color color( 0, 255, 0 );

		TDynArray< DebugVertex > renderVertices;
		TDynArray< DebugVertex > renderVertices2;
		TDynArray< Uint16 > renderIndices;
		TDynArray< Uint16 > renderIndices2;

		Color vectexColor( Color::LIGHT_YELLOW );
		vectexColor.A = 128;
		Color vectexColor2( Color::BLACK );

		Matrix bsToWsMatrix;
		params->CalcMatrixBSToWS( boneWS, bsToWsMatrix );

		const Uint32 numPoints = points.Size();
		for ( Uint32 i=0; i<numPoints; ++i )
		{
			const AnimPointCloud::TPoint& p = points[ i ];

			const Vector pWS = bsToWsMatrix.TransformPoint( p );

			mat.SetTranslation( pWS );
			frame->AddDebugBox( box, mat, color, false );

			renderVertices.PushBack( DebugVertex( pWS, vectexColor ) );
			renderVertices2.PushBack( DebugVertex( pWS, vectexColor2 ) );
		}

		const Int32 numTris = tris.SizeInt();
		for ( Int32 i=0; i<numTris; ++i )
		{
			const AnimPointCloud::Triangle& t = tris[ i ];

			static Int32 triToTest = -1;
			if ( triToTest != -1 && triToTest != i )
			{
				continue;
			}

			renderIndices.PushBack( (Uint16)t.m_indices[ 0 ] );
			renderIndices.PushBack( (Uint16)t.m_indices[ 1 ] );
			renderIndices.PushBack( (Uint16)t.m_indices[ 2 ] );

			renderIndices2.PushBack( (Uint16)t.m_indices[ 0 ] );
			renderIndices2.PushBack( (Uint16)t.m_indices[ 1 ] );

			renderIndices2.PushBack( (Uint16)t.m_indices[ 1 ] );
			renderIndices2.PushBack( (Uint16)t.m_indices[ 2 ] );

			renderIndices2.PushBack( (Uint16)t.m_indices[ 2 ] );
			renderIndices2.PushBack( (Uint16)t.m_indices[ 0 ] );
		}

		if ( renderVertices.Size() > 0 && renderIndices.Size() > 0 )
		{
			frame->AddDebugTriangles( Matrix::IDENTITY, renderVertices.TypedData(), renderVertices.Size(), renderIndices.TypedData(), renderIndices.Size(), vectexColor, false );
			frame->AddDebugIndexedLines( renderVertices2.TypedData(), renderVertices2.Size(), renderIndices2.TypedData(), renderIndices2.Size() );
		}
	}
};

//////////////////////////////////////////////////////////////////////////

CAnimPointCloudLookAtParam::CAnimPointCloudLookAtParam()
	: m_directionLS( 0.f, 1.f, 0.f )
{

}

void CAnimPointCloudLookAtParam::OnSerialize( IFile& file )
{
	ISkeletalAnimationSetEntryParam::OnSerialize( file );

	if ( !file.IsGarbageCollector() )
	{
		file << m_tris;
	}
}

void CAnimPointCloudLookAtParam::CalcMatrixBSToWS( const Matrix& boneMS, Matrix& out ) const
{
	out = Matrix::Mul( boneMS, m_boneMSInv );
}

void CAnimPointCloudLookAtParam::TransformPointWSToBS( const AnimQsTransform& l2w, const AnimQsTransform& _boneMS, const AnimVector4& pointWS, Vector& pointBS ) const
{
	AnimVector4 pointMS;
	pointMS.SetTransformedInversePos( l2w, pointWS );

	RedVector4 dirMS = Sub( pointMS, _boneMS.Translation );
	dirMS.Normalize3();

	AnimQsTransform boneMS( _boneMS );
	boneMS.Translation.Set( 0.f, 0.f, 0.f, 1.f );

	const AnimQsTransform boneTransMSInv = ENGINE_QS_TRANSFORM_TO_CONST_ANIM_QS_TRANSFORM_REF( m_boneTransMSInv );

	AnimQsTransform diff;
	diff.SetMul( boneMS, boneTransMSInv );

	AnimVector4 finalDir;
	finalDir.InverseRotateDirection( diff.Rotation, dirMS );

	pointBS = AnimVectorToVector( finalDir );
}

#ifndef NO_EDITOR

void CAnimPointCloudLookAtParam::Init( CAnimPointCloudLookAtParam::InitData& data )
{
	m_boneName = data.m_boneName;
	m_directionLS = data.m_directionLS;
	m_boneMSInv = data.m_boneMS.FullInverted();
	const AnimQsTransform trans = MatrixToAnimQsTransform( m_boneMSInv );
	m_boneTransMSInv = ANIM_QS_TRANSFORM_TO_CONST_ENGINE_QS_TRANSFORM_REF( trans );
	m_tris = data.m_tris;
	m_pointsBS = data.m_pointsBS;
	m_pointToTriMapping = data.m_pointToTriMapping;
	m_refPose = data.m_refPose;
}

#endif

//#pragma optimize("",on)
