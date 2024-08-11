/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "viewport.h"
#include "pathComponent.h"
#ifndef NO_EDITOR
	#include "curveEntity.h"
#endif
#include "../core/scriptStackFrame.h"
#include "renderFragment.h"
#include "../core/mathUtils.h"
#include "../core/curveBase.h"
#include "layer.h"
#include "world.h"
#include "renderFrame.h"


IMPLEMENT_ENGINE_CLASS( CPathComponent );

using namespace MathUtils::InterpolationUtils;

namespace RenderingUtils
{
	void CreateArrow( const Vector & startPoint, const Vector & endPoint, TDynArray<Vector> & outPoints, Float length, Float sideSize )
	{
		Vector arrowDir   = ( endPoint - startPoint ).Normalized3();
		Vector arrowStart = endPoint - arrowDir * length;

		outPoints.ClearFast();
		outPoints.Reserve( 5 );
		outPoints.PushBack( endPoint );

		Vector side = Vector::Cross( endPoint, startPoint ).Normalized3() * sideSize;
		outPoints.PushBack( arrowStart + side );
		outPoints.PushBack( arrowStart - side );

		side = Vector::Cross( arrowDir, side ).Normalized3() * sideSize;
		outPoints.PushBack( arrowStart + side );
		outPoints.PushBack( arrowStart - side );
	}

	void RenderArrow( CRenderFrame* frame, const Color & wireFrameColor, const Color & color, const TDynArray<Vector> & points, Bool overlay )
	{
		ASSERT( points.Size() >= 5 );

		frame->AddDebugLine( points[ 0 ], points[ 1 ], wireFrameColor );
		frame->AddDebugLine( points[ 0 ], points[ 2 ], wireFrameColor );
		frame->AddDebugLine( points[ 0 ], points[ 3 ], wireFrameColor );
		frame->AddDebugLine( points[ 0 ], points[ 4 ], wireFrameColor );
		frame->AddDebugLine( points[ 1 ], points[ 3 ], wireFrameColor );
		frame->AddDebugLine( points[ 1 ], points[ 4 ], wireFrameColor );
		frame->AddDebugLine( points[ 2 ], points[ 3 ], wireFrameColor );
		frame->AddDebugLine( points[ 2 ], points[ 4 ], wireFrameColor );

		TStaticArray< DebugVertex, 5 > vertices( 5 );
		TStaticArray< Uint16, 24 > indices( 24 );

		Color faceColor = color;
		// Alpha
		faceColor.A = 80;

		// Write
		Uint16* index = indices.TypedData();
		DebugVertex* vertex = vertices.TypedData();

		// Faces
		vertex[0].Set( points[ 0 ], faceColor );
		vertex[1].Set( points[ 1 ], faceColor );
		vertex[2].Set( points[ 2 ], faceColor );
		vertex[3].Set( points[ 3 ], faceColor );
		vertex[4].Set( points[ 4 ], faceColor );

		// Indices
		index[0] = 0;	index[1] = 1;	index[2] = 3;	index[3] = 3;	index[4] = 1;	index[5] = 0;	index[6] = 0;	index[7] = 2;
		index[8] = 3;	index[9] = 3;	index[10] = 2;	index[11] = 0;	index[12] = 0;	index[13] = 1;	index[14] = 4;	index[15] = 4;
		index[16] = 1;	index[17] = 0;	index[18] = 0;	index[19] = 2;	index[20] = 4;	index[21] = 4;	index[22] = 2;	index[23] = 0;

		// Draw faces
		new ( frame ) CRenderFragmentDebugPolyList( frame, Matrix::IDENTITY, vertices.TypedData(), vertices.Size(), indices.TypedData(), indices.Size(), overlay ? RSG_DebugOverlay : RSG_DebugUnlit );
	}

	void RenderPolyline( CRenderFrame* frame, const Color & color, const TDynArray<Vector> & points, Bool closed )
	{
		Uint32 nVertices = points.Size();
		closed         = closed && nVertices > 2;
		Uint32 nEdges    = closed ? nVertices : nVertices - 1;

		// No edges to draw
		if ( nEdges == 0 )
		{
			return;
		}

		// Hit proxy mode, render fat lines
		if ( frame->GetFrameInfo().m_renderingMode == RM_HitProxies )
		{
			for ( Uint32 i=0; i < nEdges; ++i )
			{
				frame->AddDebugFatLine( points[ i ], points[ (i+1) % nVertices ], color, 0.1f );
			}
			return;
		}

		// Draw lines
		{
			TDynArray< DebugVertex > vertices;
			vertices.Resize( nEdges * 2 );
			DebugVertex* write = vertices.TypedData();

			for ( Uint32 i=0; i < nEdges; ++i )
			{
				// Add edge points
				(write++)->Set( points[ i                 ], color );
				(write++)->Set( points[ (i+1) % nVertices ], color );
			}
			for ( Uint32 i=0; i < nEdges; ++i )
			{
				frame->AddDebugFatLine( points[ i ], points[ (i+1) % nVertices ], color, 0.02f );
			}
			for ( Uint32 i=0; i < nEdges; ++i )
			{
				frame->AddDebugFatLine( points[ i ], points[ (i+1) % nVertices ], Color::BLACK, 0.01f );
			}
			//frame->AddDebugFatLine( vertices.TypedData(), vertices.Size() );
			frame->AddDebugLines( vertices.TypedData(), vertices.Size() );
		}
	}
}

CPathComponent::CPathComponent()
	: m_curveTotalDistance( 0.0f )
	, m_speedCurve( )
{
	m_speedCurve.SetUseDayTime(false);
}

Int32 CPathComponent::FindClosestEdge( const Vector& point )
{
	Vector pointLocal;
	m_curve.ToLocalPosition( point, pointLocal );
	return m_curve.FindNearestEdgeIndex( pointLocal );
}

void CPathComponent::GetInterpolationControlPoints( Vector& p1, Vector& p2, Vector& p3, Vector& p4, Int32 edgeIdx )
{
	const Uint32 nVertices = m_curve.Size();
	const Bool closed    = m_curve.IsLooping() && nVertices > 2;
	const Uint32 nEdges    = closed ? nVertices : nVertices - 1;

	ASSERT( edgeIdx >= 0 && edgeIdx < (Int32)nEdges );

	m_curve.GetControlPointPosition(
		(edgeIdx > 0 )
		? ( edgeIdx - 1 )
		: closed ? ( nVertices - 1 ) : edgeIdx,
		p1 );
	m_curve.GetControlPointPosition( edgeIdx, p2 );
	m_curve.GetControlPointPosition( (edgeIdx + 1) % nVertices, p3 );
	m_curve.GetControlPointPosition(
		closed
		? ( (edgeIdx + 2) % nVertices )
		: Min<Int32>( nVertices-1, edgeIdx+2 ),
		p4 );
}

Float CPathComponent::GetAlphaOnEdge( const Vector& point, Int32 edgeIdx, Float epsilon /*= 0.001f*/ )
{
	Vector p1, p2, p3, p4;
	GetInterpolationControlPoints( p1, p2, p3, p4, edgeIdx );

	Vector pointLocal;
	m_curve.ToLocalPosition( point, pointLocal );

	Vector closestPoint;
	return HermiteClosestPoint( p1, p2, p3, p4, pointLocal, closestPoint, epsilon );
}

Vector CPathComponent::GetPathPoint( Int32 edgeIdx, Float edgeAlpha )
{
	Vector p1, p2, p3, p4;
	GetInterpolationControlPoints( p1, p2, p3, p4, edgeIdx );

	Vector worldPoint;
	m_curve.ToAbsolutePosition( HermiteInterpolate( p1, p2, p3, p4, edgeAlpha ), worldPoint );
	return worldPoint;
}

Vector CPathComponent::CorrectPathPoint( const Vector& point, Int32& edgeIdx, Float& edgeAlpha, Float epsilon )
{
	struct Local
	{
		static Float DistanceToEdge( const Vector& pointLocal, Int32 edgeIdx, SMultiCurve& curve )
		{
			Vector a, b;
			curve.GetControlPointPosition( edgeIdx, a );
			curve.GetControlPointPosition( (edgeIdx + 1) % curve.Size(), b );

			return pointLocal.DistanceToEdge( a, b );
		}
	};
	Vector pointLocal;
	m_curve.ToLocalPosition( point, pointLocal );

	const Int32 nVertices	= m_curve.Size();
	const Bool closed		= m_curve.IsLooping() && nVertices > 2;
	const Int32 nEdges		= closed ? nVertices : nVertices - 1;
	
	Float currEdgeDist	= Local::DistanceToEdge( pointLocal, edgeIdx, m_curve );
	Float bestDist		= currEdgeDist;
	Bool movedForward	= false;
	
	// try to move forward
	while ( edgeIdx + 1 < nEdges )
	{
		Int32 tryEdge	= edgeIdx + 1;
		Float tryDist	= Local::DistanceToEdge( pointLocal, tryEdge, m_curve );
		if ( tryDist	>= bestDist )
		{
			break;
		}
		edgeIdx			= tryEdge;
		bestDist		= tryDist;
		movedForward	= true;
	}
	if ( !movedForward )
	{
		while ( edgeIdx > 0 )
		{
			Int32 tryEdge	= edgeIdx-1;
			Float tryDist	= Local::DistanceToEdge( pointLocal, tryEdge, m_curve );
			if ( tryDist	>= bestDist )
			{
				break;
			}
			edgeIdx		= tryEdge;
			bestDist	= tryDist;
		}
	}

	Vector p1, p2, p3, p4;
	GetInterpolationControlPoints( p1, p2, p3, p4, edgeIdx );

	Vector closestPoint;
	edgeAlpha						= HermiteClosestPoint( p1, p2, p3, p4, pointLocal, closestPoint, epsilon );
	return GetLocalToWorld().TransformPoint( closestPoint );
}
void CPathComponent::InitCachedData()
{
	const Int32 nVertices	= m_curve.Size();
	const Bool closed		= m_curve.IsLooping() && nVertices > 2;
	const Uint32 nEdges		= closed ? nVertices : nVertices - 1;
	m_curveTotalDistance	= 0.0f;
	m_curveCachedDistances.Clear();
	m_curveCachedDistances.Grow( nEdges );
	for ( Uint32 i = 0; i < nEdges; ++i )
	{
		Vector p1, p2;
		m_curve.GetControlPointPosition( i, p1 );
		m_curve.GetControlPointPosition( (i + 1) % nVertices, p2 );
		const Float distance		= (p1 - p2).Mag3();
		m_curveCachedDistances[ i ]	= distance;
		m_curveTotalDistance		+= distance;
	}
}
Bool CPathComponent::CalculateSpeedMult( Int32 edgeIdx, Float edgeAlpha, Float& outSpeedMult )
{
	outSpeedMult = 1.0f;
	if ( m_speedCurve.GetNumPoints() == 0 )
	{
		return false;
	}
	const Float t			= CalculateT( edgeIdx, edgeAlpha );
	outSpeedMult			= m_speedCurve.GetFloatValue( t );
	return true;
}

Float CPathComponent::CalculateT( Int32 edgeIdx, Float edgeAlpha )
{
	const Float distance	= CalculateDistanceFromStart( edgeIdx, edgeAlpha );
	return distance / m_curveTotalDistance;
}

Bool CPathComponent::HasSpeedCurve( )const
{
	if ( m_speedCurve.GetNumPoints() == 0 )
	{
		return false;
	}
	return true;
}

Float CPathComponent::CalculateDistanceFromStart( Int32 edgeIdx, Float edgeAlpha )
{
	// precomputed stuff !
	if ( m_curveCachedDistances.Size() == 0 )
	{
		InitCachedData();
	}
	Float distance				= 0.0f; // the distance we went through already

	for ( Int32 i = 0; i < edgeIdx; ++i )
	{
		distance += m_curveCachedDistances[ i ];
	}
	const Float currentSegmentLength	= m_curveCachedDistances[ edgeIdx ];
	const Float distanceFromStart		= (distance + edgeAlpha * currentSegmentLength );

	return distanceFromStart;
}

Vector CPathComponent::GetClosestPointOnPath( const Vector& point, Int32 &edgeIdx, Float &edgeAlpha, Float epsilon /*= 0.001f*/ )
{
	edgeIdx = FindClosestEdge( point );

	Vector pointLocal;
	m_curve.ToLocalPosition( point, pointLocal );

	const Uint32 nVertices = m_curve.Size();
	const Bool closed    = m_curve.IsLooping() && nVertices > 2;
	const Uint32 nEdges    = closed ? nVertices : nVertices - 1;

	Int32  step       = 0;
	Int32  prevStep;
	Vector closestPoint;

	do {

		Vector p1, p2, p3, p4;
		GetInterpolationControlPoints( p1, p2, p3, p4, edgeIdx );

		prevStep = step;
		edgeAlpha = HermiteClosestPoint( p1, p2, p3, p4, pointLocal, closestPoint, epsilon );

		Int32 step = ( edgeAlpha < 0.f ) ? -1
				 : ( edgeAlpha > 1.f ) ?  1
				 :                        0;

		if ( !closed )
		{
			if ( step > 0 && edgeIdx + step > (Int32)nEdges - 1 )
			{
				edgeIdx   = nEdges - 1;
				edgeAlpha = 1.f;
				break;
			}
			else if ( step < 0 && edgeIdx + step < 0 )
			{
				edgeIdx   = 0;
				edgeAlpha = 0.f;
				break;
			}
		}

		edgeIdx = ( edgeIdx + step ) % nEdges;
		if ( edgeIdx < 0 )
			edgeIdx += nEdges;

	} while ( step != 0 && step != -prevStep ); // step != -prevStep - avoid returning to the edge that we previously came from

	closestPoint = GetLocalToWorld().TransformPoint( closestPoint );
	return closestPoint;
}

Vector CPathComponent::GetClosestPointOnPath( const Vector& point, Float epsilon /*= 0.001f*/ )
{
	Int32   edgeIdx;
	Float edgeAlpha;
	return GetClosestPointOnPath( point, edgeIdx, edgeAlpha, epsilon );
}

Vector CPathComponent::GetNextPointOnPath( Int32 &edgeIdx, Float &edgeAlpha, Float distance, Bool &isEndOfPath, Float epsilon /*= 0.001f*/ )
{
	Uint32 nVertices = m_curve.Size();
	Bool closed    = m_curve.IsLooping() && nVertices > 2;
	Uint32 nEdges    = closed ? nVertices : nVertices - 1;
	isEndOfPath = false;

	ASSERT( edgeIdx >= 0 && edgeIdx < (Int32)nEdges );
	ASSERT( edgeAlpha >= 0.f && edgeAlpha <= 1.f );
	ASSERT( distance != 0.f );

	Vector closestPoint;

	do {
		Vector p2, p3;
		m_curve.GetControlPointPosition( edgeIdx, p2);
		m_curve.GetControlPointPosition( (edgeIdx + 1) % nVertices, p3 );

		Float alpha = distance > 0.f ? 1.f - edgeAlpha : edgeAlpha;
		Float edgeLenght = ( p3 - p2 ).Mag3();
		Float edgeLengthLeft = edgeLenght * alpha;

		if ( edgeLengthLeft >= MAbs( distance ) )
		{
			Vector p1, p4;
			
			m_curve.GetControlPointPosition(
				edgeIdx > 0 
				? ( edgeIdx - 1 )
				: closed ? ( nVertices-1 ) : ( edgeIdx ),
				p1 );
			m_curve.GetControlPointPosition(
				closed
				? ( (edgeIdx + 2) % nVertices )
				: Min<Int32>( nVertices-1, edgeIdx+2 ),
				p4 );

			edgeAlpha = edgeAlpha + distance / edgeLengthLeft * alpha;
			closestPoint = HermiteInterpolate( p1, p2, p3, p4, edgeAlpha );
			break;
		}

		if ( distance > 0 )
			distance -= edgeLengthLeft;
		else
			distance += edgeLengthLeft;

		if ( distance > 0.f )
		{
			if ( !closed && edgeIdx + 1 > (Int32)nEdges - 1 )
			{
				edgeIdx   = nEdges - 1;
				edgeAlpha = 1.f;
				closestPoint = p3;
				isEndOfPath  = true;
				break;
			}
			edgeAlpha = 0.f;
			if ( ++edgeIdx == (Int32)nEdges )
				edgeIdx = 0;
		}
		else
		if ( distance < 0.f )
		{
			if ( !closed && edgeIdx - 1 < 0 )
			{
				edgeIdx   = 0;
				edgeAlpha = 0.f;
				closestPoint = p2;
				isEndOfPath  = true;
				break;
			}
			edgeAlpha = 1.f;
			if ( --edgeIdx < 0 )
				edgeIdx += nEdges;
		}

	} while ( true );

	closestPoint = GetLocalToWorld().TransformPoint( closestPoint );
	return closestPoint;
}

Vector CPathComponent::GetNextPointOnPath( const Vector& point, Float distance, Bool &isEndOfPath, Float epsilon /*= 0.001f*/ )
{
	Int32   edgeIdx;
	Float edgeAlpha;
	GetClosestPointOnPath( point, edgeIdx, edgeAlpha, epsilon );
	return GetNextPointOnPath( edgeIdx, edgeAlpha, distance, isEndOfPath, epsilon );
}

Float CPathComponent::GetDistanceToPath( const Vector& point, Float epsilon /*= 0.001f*/ )
{
	return ( GetClosestPointOnPath( point, epsilon ) - point ).Mag3();
}

void CPathComponent::OnSpawned( const SComponentSpawnInfo& spawnInfo )
{
	// Pass to base class
	TBaseClass::OnSpawned( spawnInfo );

	// Initialize the curve
	m_curve.Reset();
	m_curve.SetCurveType( ECurveType_Vector, NULL, false );
	m_curve.SetPositionInterpolationMode( ECurveInterpolationMode_Automatic );
	m_curve.SetTransformationRelativeMode( ECurveRelativeMode_InitialTransform );
	m_curve.SetShowFlags( SHOW_Paths );
	m_curve.SetLooping( false );
	m_curve.SetColor( Color::YELLOW );
	m_curve.AddControlPoint( 0.0f, Vector( -0.5f, -0.5f, 0.f ) );
	m_curve.AddControlPoint( 0.5f, Vector( -0.5f, +0.5f, 0.f ) );
	m_curve.AddControlPoint( 1.0f, Vector( +0.5f, +0.5f, 0.f ) );
	m_curve.EnableAutomaticTimeByDistanceRecalculation( true );
}

void CPathComponent::OnAttached( CWorld* world )
{
	// Pass to base class
	TBaseClass::OnAttached( world );

	// Set up curve parent so the curve calculations can use it when determining absolute curve coordinates
	m_curve.SetParent( this );

#ifndef NO_EDITOR
	// Add editable curve
	if ( GIsEditor )
	{
		CCurveEntity::CreateEditor( world, &m_curve, false );
	}
	else
	{
		// in the editor this is handled by curve entity
		world->GetEditorFragmentsFilter().RegisterEditorFragment( this, SHOW_Paths );
	}
	world->GetEditorFragmentsFilter().RegisterEditorFragment( this, SHOW_PathSpeedValues );
#else
#  ifndef NO_EDITOR_FRAGMENTS
	world->GetEditorFragmentsFilter().RegisterEditorFragment( this, SHOW_Paths );
	world->GetEditorFragmentsFilter().RegisterEditorFragment( this, SHOW_PathSpeedValues );
#  endif
#endif
}

void CPathComponent::OnDetached( CWorld* world )
{
	// Pass to base class
	TBaseClass::OnDetached( world );

#ifndef NO_EDITOR
	// Remove editable curve
	if ( GIsEditor )
	{
		CCurveEntity::DeleteEditor( world, &m_curve );
	}
	else
	{
		// in the editor this is handled by curve entity
		world->GetEditorFragmentsFilter().UnregisterEditorFragment( this, SHOW_Paths );
	}
	world->GetEditorFragmentsFilter().UnregisterEditorFragment( this, SHOW_PathSpeedValues );
#else
#  ifndef NO_EDITOR_FRAGMENTS
	world->GetEditorFragmentsFilter().UnregisterEditorFragment( this, SHOW_Paths );
	world->GetEditorFragmentsFilter().UnregisterEditorFragment( this, SHOW_PathSpeedValues );
#  endif
#endif


}

void CPathComponent::OnSelectionChanged()
{
#ifndef NO_EDITOR
	if ( IsSelected() )
	{
		CCurveEntity::CreateEditor( GetLayer()->GetWorld(), &m_curve, true );
	}
#endif

	TBaseClass::OnSelectionChanged();
}

Vector CPathComponent::GetBeginning()
{
	if ( m_curve.Size() == 0 )
	{
		return Vector::ZEROS;
	}

	Vector pos;
	m_curve.GetAbsoluteControlPointPosition( 0, pos );
	return pos;
}

#ifndef NO_EDITOR

void CPathComponent::EditorOnTransformChangeStart()
{
	TBaseClass::EditorOnTransformChangeStart();
}

void CPathComponent::EditorOnTransformChanged()
{
	TBaseClass::EditorOnTransformChanged();
	m_curve.UpdateInitialTransformFromParent( true );
}

void CPathComponent::EditorOnTransformChangeStop()
{
	TBaseClass::EditorOnTransformChangeStop();
}

#endif


/************************************************************************/
/* Script functions                                                     */
/************************************************************************/
void CPathComponent::funcFindClosestEdge( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, point, Vector::ZEROS );
	FINISH_PARAMETERS;

	Int32 val = FindClosestEdge( point );
	RETURN_INT( val );
}

void CPathComponent::funcGetAlphaOnEdge( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, point, Vector::ZEROS );
	GET_PARAMETER( Int32, edge, 0 );
	GET_PARAMETER_OPT( Float, epsilon, 0.001f );
	FINISH_PARAMETERS;

	Float val = GetAlphaOnEdge( point, edge, epsilon );
	RETURN_FLOAT( val );
}

void CPathComponent::funcGetClosestPointOnPath( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, point, Vector::ZEROS );
	GET_PARAMETER_OPT( Float, epsilon, 0.001f );
	FINISH_PARAMETERS;

	Vector & retVal = *(Vector*)result;
	retVal = GetClosestPointOnPath( point, epsilon );
}

void CPathComponent::funcGetClosestPointOnPathExt( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, point, Vector::ZEROS );
	GET_PARAMETER_REF( Int32, edge, 0 );
	GET_PARAMETER_REF( Float, alpha, 0.f );
	GET_PARAMETER_OPT( Float, epsilon, 0.001f );
	FINISH_PARAMETERS;

	Vector & retVal = *(Vector*)result;
	retVal = GetClosestPointOnPath( point, edge, alpha, epsilon );
}

void CPathComponent::funcGetDistanceToPath( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, point, Vector::ZEROS );
	GET_PARAMETER_OPT( Float, epsilon, 0.001f );
	FINISH_PARAMETERS;

	Float val = GetDistanceToPath( point, epsilon );
	RETURN_FLOAT( val );
}

void CPathComponent::funcGetNextPointOnPath( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, point, Vector::ZEROS );
	GET_PARAMETER( Float, distance, 1.f );
	GET_PARAMETER_REF( Bool, isEndOfPath, false );
	GET_PARAMETER_OPT( Float, epsilon, 0.001f );
	FINISH_PARAMETERS;

	Vector & retVal = *(Vector*)result;
	retVal = GetNextPointOnPath( point, distance, isEndOfPath, epsilon );
}

void CPathComponent::funcGetNextPointOnPathExt( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( Int32, edge, 0 );
	GET_PARAMETER_REF( Float, alpha, 0.f );
	GET_PARAMETER( Float, distance, 1.f );
	GET_PARAMETER_REF( Bool, isEndOfPath, false );
	GET_PARAMETER_OPT( Float, epsilon, 0.001f );
	FINISH_PARAMETERS;

	Vector & retVal = *(Vector*)result;
	retVal = GetNextPointOnPath( edge, alpha, distance, isEndOfPath, epsilon );
}

void CPathComponent::funcGetWorldPoint( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Int32, index, 0 );
	FINISH_PARAMETERS;

	Vector & retVal = *(Vector*)result;
	m_curve.GetAbsoluteControlPointPosition( index, retVal );
}

void CPathComponent::funcGetPointsCount( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_INT( m_curve.Size() );
}

Bool CPathComponent::OnPropertyMissing( CName propertyName, const CVariant& readValue )
{
	// Convert from the old path component format

	if ( propertyName == TXT("localPoints") )
	{
		TDynArray< Vector > localPoints;
		readValue.AsType< TDynArray< Vector > >( localPoints );

		m_curve.SetFromPoints( localPoints );
		m_curve.SetPositionInterpolationMode( ECurveInterpolationMode_Automatic );
		m_curve.SetTransformationRelativeMode( ECurveRelativeMode_InitialTransform );
		m_curve.EnableAutomaticTimeByDistanceRecalculation( true );
		m_curve.SetShowFlags( SHOW_Paths );
		return true;
	}
	else if ( propertyName == TXT("closed") )
	{
		Bool closed;
		readValue.AsType< Bool >( closed );

		m_curve.SetLooping( closed );
		return true;
	}
	else if ( propertyName == TXT("color") )
	{
		Color color;
		readValue.AsType< Color >( color );

		m_curve.SetColor( color );
		return true;
	}

	return TBaseClass::OnPropertyMissing( propertyName, readValue );
}

#ifndef RED_FINAL_BUILD
void CPathComponent::OnSerialize( IFile& file )
{
	TBaseClass::OnSerialize( file );

	// :( SHOW_Pathfinding -> SHOW_Paths refactoring :(
	if ( !IsCooked() && file.IsReader() )
	{
		RefactorPath();
	}
}

void CPathComponent::RefactorPath()
{
	m_curve.SetShowFlags( SHOW_Paths );
	if( m_curve.GetNumCurves() > 0 && m_curve.IsLooping() )
	{
		m_curve.SetLooping( false );
	}
}
#endif 

#ifndef NO_RESOURCE_COOKING	
void CPathComponent::OnCook( class ICookerFramework& cooker )
{
	RefactorPath();

	TBaseClass::OnCook( cooker );
}
#endif

Bool CPathComponent::ShouldGenerateEditorFragments( CRenderFrame* frame ) const
{
	Float dist = frame->GetFrameInfo().GetRenderingDebugOption( VDCommon_MaxRenderingDistance );
	Vector currentPos = const_cast< CPathComponent* >( this )->GetClosestPointOnPath( frame->GetFrameInfo().m_camera.GetPosition() );
	Float distFromCam = frame->GetFrameInfo().m_camera.GetPosition().DistanceSquaredTo( currentPos );
	if ( distFromCam < dist )
	{
		return true;
	}
	else
	{
		return false;
	}
}

void CPathComponent::OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flag )
{
	CComponent::OnGenerateEditorFragments( frame, flag );

	if( flag == SHOW_PathSpeedValues )
	{
		const Uint32 pointCount = m_speedCurve.GetNumPoints();

		for ( Uint32 i = 0; i < pointCount; ++i )
		{
			Vector localPositionOnPath;
			const Float timeAtIndex		= m_speedCurve.GetCurveData().GetTimeAtIndex( i );
			const Float speedAtIndex	= m_speedCurve.GetCurveData().GetFloatValueAtIndex( i );
			m_curve.GetPosition( timeAtIndex, localPositionOnPath );
	
			const Vector positionOnPath	= GetLocalToWorld().TransformPoint( localPositionOnPath );
			frame->AddDebugBox( Box( positionOnPath, 0.5f), Matrix::IDENTITY, Color::RED );
			String debugText = String::Printf( TXT("Speed = %f"), speedAtIndex );
			frame->AddDebugText( positionOnPath + Vector( 0.0f, 0.0f, 1.0f ), debugText, true, Color::LIGHT_BLUE );
		}
	}


	if( flag == SHOW_Paths )
	{
		RED_ASSERT( !GIsEditor, TXT( "This one should not be run in the editor, cause it should not be registered then" ) );

		if( GIsEditor )
		{
			return; // cook only
		}

		// this one is used to be able to draw SHOW_Paths when curve entity is not spawned
		// the exact situation is when running on cook.
		static const Float g_maxLineLength = 0.3f;
		static const Float g_maxTimeLineLength = 0.01f;
		static const Float g_lineWidth = 0.02f;

		Color colors[4] =
		{
			Color(255, 50, 50, 200),
			Color(50, 255, 50, 200),
			Color(0, 50, 255, 200),
			m_curve.GetColor()
		};

		const Float arrowLength = 0.4f;
		const Float arrowSideSize = 0.15f;

		Float prevTime = m_curve.GetControlPointTime( 0 );
		EngineTransform prevTransform;
		m_curve.GetAbsoluteControlPointTransform( 0, prevTransform );

		TDynArray<DebugVertex> positionVerts;
		TDynArray<DebugVertex> rotationVerts;
		TDynArray<Uint16> rotationIndices;

		const Uint32 numControlPoints = m_curve.Size();
		const Uint32 numEndControlPoints = numControlPoints + ( m_curve.IsLooping() ? 1 : 0 );

		for ( Uint32 i = 1; i < numEndControlPoints; i++ )
		{
			// Get next transform

			const Uint32 endControlPointIndex = i < numControlPoints ? i : 0;

			Float nextTime;
			if ( i < numControlPoints )
			{
				nextTime = m_curve.GetControlPointTime( endControlPointIndex );
			}
			else
			{
				nextTime = m_curve.GetTotalTime();
			}
			EngineTransform nextTransform;
			m_curve.GetAbsoluteControlPointTransform( endControlPointIndex, nextTransform );

			// Get the distance between control points
			const Float distanceBetweenControlPoints = prevTransform.GetPosition().DistanceTo( nextTransform.GetPosition() );
			Int32 numLines = distanceBetweenControlPoints <= 0.0f ? 1 : (Int32) Red::Math::MCeil( distanceBetweenControlPoints / g_maxLineLength );
			numLines = Min( numLines, 16 );


			// Draw the lines between two control points
			if ( numLines > 0 )
			{
				const Float subTimeStep = (nextTime - prevTime) / (Float) numLines;

				Float subTime = prevTime;

				for ( int j = 0; j < numLines; j++, subTime += subTimeStep )
				{
					const Float subTimeWrapped = Min( subTime, m_curve.GetTotalTime() );

					EngineTransform subTransform;
					m_curve.GetAbsoluteTransform( subTimeWrapped, subTransform );

					// Generate position vertex data
					{
						const Uint32 colorIndex = 3;
						DebugVertex vertex;
						vertex.Set( subTransform.GetPosition(), colors[ colorIndex ] );
						positionVerts.PushBack( vertex );
					}
				}
			}

			// Go to the next control point
			prevTransform = nextTransform;
			prevTime = nextTime;
		}

		// Finalize drawing curve position
		{
			// Append the last vertex
			const Uint32 colorIndex = 3;
			DebugVertex vertex;
			vertex.Set( prevTransform.GetPosition(), colors[ colorIndex ] );
			positionVerts.PushBack( vertex );

			// Draw
			frame->AddDebugFatLines( positionVerts.TypedData(), positionVerts.Size(), g_lineWidth );
		}
	}
}