/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "actionAreaComponent.h"
#include "../../common/core/gatheredResource.h"
#include "../core/mathUtils.h"
#include "../engine/renderFrame.h"
#include "../engine/bitmapTexture.h"

extern CGatheredResource resWayPointIcon;
extern Matrix UnscaleMatrix( const Matrix & matrix );

namespace RenderingUtils
{
	void CreateArrow( const Vector & startPoint, const Vector & endPoint, TDynArray<Vector> & outPoints, Float length = 0.2f, Float sideSize = 0.05f );
}

void CActionAreaComponent::EditorOnlyData::RenderLinkValidity( CRenderFrame* frame, const Vector & worldPosition, const CHitProxyID & hitProxyId )
{
	Bool hasNotBurnedLinks  = false;
	Bool allLinksAreInvalid = true;

	CBitmapTexture* icon = NULL;
	for ( Uint32 i = 0; i < m_offMeshLinks.Size(); ++i )
	{
		EditorOnlyData::LinkInfo & link = m_offMeshLinks[i];

		if ( ! link.m_startValid )
		{
			if ( ! icon )
			{
				icon = resWayPointIcon.LoadAndGet< CBitmapTexture >();
			}
			if ( icon )
			{
				Float screenScale = frame->GetFrameInfo().CalcScreenSpaceScale( link.m_start );
				const Float size = Max( 0.25f, 0.25f * screenScale * 0.33f ); 
				frame->AddSprite( link.m_start, size, Color::RED, hitProxyId, icon );
			}
		}
		if ( ! link.m_endValid )
		{
			if ( ! icon )
			{
				icon = resWayPointIcon.LoadAndGet< CBitmapTexture >();
			}
			if ( icon )
			{
				Float screenScale = frame->GetFrameInfo().CalcScreenSpaceScale( link.m_end );
				const Float size = Max( 0.25f, 0.25f * screenScale * 0.33f ); 
				frame->AddSprite( link.m_end, size, Color::RED, hitProxyId, icon );
			}
		}

		if ( link.m_startValid && link.m_endValid )
		{
			allLinksAreInvalid = false;
			hasNotBurnedLinks  = hasNotBurnedLinks || ! link.m_isBurned;

			frame->AddDebugSphere( link.m_start, 0.25f, Matrix::IDENTITY, Color::LIGHT_BLUE );
		}
	}

	if ( allLinksAreInvalid )
	{
		frame->AddDebugText( worldPosition, TXT("All links are outside navmesh"), true, Color::RED );
	}
	else
	if ( hasNotBurnedLinks )
	{
		frame->AddDebugText( worldPosition, TXT("This action area needs navmesh preprocessing to work correctly"), true, Color::RED );
	}
}

void CActionAreaComponent::InvalidateParabola( Bool fillParabolaNow )
{
	if ( m_editorData != NULL )
	{
		m_editorData->m_localParabolaPoints.Clear();
	}
	
	// Fill local parabola points if needed
	if ( fillParabolaNow )
	{
		if ( m_editorData == NULL )
		{
			m_editorData = new EditorOnlyData();
		}

		// Get total shift
		if ( ! m_totalTransformationIsKnown )
		{
			CalculateTotalShift();
		}

		Vector totalTranslation = m_fullTransformation.GetTranslation();
		Vector startPoint = Vector::ZERO_3D_POINT;
		Vector endPoint   = startPoint + totalTranslation;

		Vector midPoint;
		midPoint.X = startPoint.X + totalTranslation.X * 0.5f;
		midPoint.Y = startPoint.Y + totalTranslation.Y * 0.5f;
		midPoint.Z = Max( startPoint.Z, endPoint.Z ) + 1.f;
		midPoint.W = 1.f;

		TDynArray<Vector> keyPoints;
		keyPoints.PushBack( startPoint );
		keyPoints.PushBack( midPoint );
		keyPoints.PushBack( endPoint );

		m_editorData->CreateParabola( GetLocalToWorld(), keyPoints );
	}
}

namespace Interpolation
{
	void InterpolatePoints( const TDynArray< Vector > &keyPoints, TDynArray< Vector > &outPoints, Uint32 pointsNum )
	{
		if ( pointsNum == 0 )
			return;
		ASSERT( pointsNum );

		Float pntDelta   = (keyPoints.Size() - 1) / (Float) (pointsNum+1);
		Float curPnt     = pntDelta;
		Uint32  numCreated = 0;

		for ( Uint32 i = 0; i < keyPoints.Size()-1; ++i )
		{
			const Vector &p1 = i > 0 ? keyPoints[ i - 1 ] : keyPoints[ i ];
			const Vector &p2 = keyPoints[ i    ];
			const Vector &p3 = keyPoints[ i + 1];
			const Vector &p4 = i + 2 < keyPoints.Size() ? keyPoints[ i + 2 ] : p3;

			while ( curPnt <= i + 1 )
			{
				Float alpha = curPnt - i;
				curPnt += pntDelta;

				outPoints.PushBack( MathUtils::InterpolationUtils::CubicInterpolate( p1,p2,p3,p4, alpha ) );
				++numCreated;

				if ( numCreated == pointsNum )
					return;
			}
		}
	}
}

void CActionAreaComponent::EditorOnlyData::CreateParabola( const Matrix & localToWorld, const TDynArray<Vector> & keyPoints )
{
	ASSERT( keyPoints.Size() > 2 );

	m_localParabolaPoints.Clear();
	m_localParabolaPoints.PushBack( keyPoints[0] );
	Interpolation::InterpolatePoints( keyPoints, m_localParabolaPoints, 20 );
	m_localParabolaPoints.PushBack( keyPoints.Back() );
	
	Matrix transformMatrix = UnscaleMatrix( localToWorld );

	// Create world parabola
	m_worldParabolaPoints.Clear();
	for ( Uint32 i = 0; i < m_localParabolaPoints.Size(); ++i )
	{
		m_worldParabolaPoints.PushBack( transformMatrix.TransformPoint( m_localParabolaPoints[ i ] ) );
	}

	// Create world arrow
	{
		const Vector & startPoint = m_worldParabolaPoints[ m_worldParabolaPoints.Size() - 2 ];
		const Vector & endPoint   = m_worldParabolaPoints.Back();
		RenderingUtils::CreateArrow( startPoint, endPoint, m_worldParabolaArrowPoints );
	}
}
