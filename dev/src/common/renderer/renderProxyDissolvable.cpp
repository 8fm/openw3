/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
* Kamil Nowakowski
*/

#include "build.h"
#include "renderProxyDissolvable.h"
#include "renderScene.h"

using namespace DissolveHelpers;

//-----------------------------------------------------------------------------

CRenderProxyObjectGroup::CRenderProxyObjectGroup( const Uint64 groupTag )
	: m_groupTag( groupTag )
	, m_ready( false )
{
}

CRenderProxyObjectGroup::~CRenderProxyObjectGroup()
{
}

void CRenderProxyObjectGroup::RegisterProxy( IRenderProxyDissolvable* proxy )
{
	if ( proxy )
	{
		m_proxies.PushBack( proxy );
		proxy->AddRef();

		m_ready = false;
	}
}

void CRenderProxyObjectGroup::UnregisterProxy( IRenderProxyDissolvable* proxy )
{
	if ( m_proxies.Remove( proxy ) )
	{
		proxy->Release();
	}
}

Bool CRenderProxyObjectGroup::IsGroupReadyForRendering( const CRenderCollector& collector )
{
	// reevaluate
	if ( !m_ready )
	{
		// check all proxies
		Bool isReady = true;
		for ( auto* proxy : m_proxies )
		{
			if ( !proxy->IsProxyReadyForRendering() )
			{
				isReady = false;
				break;
			}
		}

		// proxy is ready
		m_ready = isReady;
	}

	// return the flag
	return m_ready;
}

//-----------------------------------------------------------------------------

IRenderProxyDissolvable::IRenderProxyDissolvable( ERenderProxyType type, const RenderProxyInitInfo& initInfo )
	: IRenderProxyDrawable( type, initInfo )
{

}


IRenderProxyDissolvable::~IRenderProxyDissolvable()
{

}

Bool IRenderProxyDissolvable::IsProxyReadyForRendering() const
{
	return true;
}

void IRenderProxyDissolvable::GetLOD( const CRenderCollector& collector, Int32& outBaseLOD, Int32& outNextLOD, Bool* outLODChange ) const
{
	if ( m_lodSelector.GetNumLODs() == 0 )
	{
		// TODO: WHY DO WE HAVE PROXIES WITH NOTHING TO RENDER ?
		outNextLOD = -1;
		outBaseLOD = -1;
	}
	else
	{
		m_lodSelector.GetLODIndices( outBaseLOD, outNextLOD );
	}

	// are we dissolving this proxy ?
	if ( outLODChange )
	{
		*outLODChange |= m_lodSelector.IsDuringLODChange();
	}
}


const Vector IRenderProxyDissolvable::CalcDissolveValues( const Int32 lodIndex ) const
{
	RED_FATAL_ASSERT( m_scene != nullptr, "Not part of the scene" );

	const auto& sc = m_scene->GetDissolveSynchronizer();

	const CRenderDissolveAlpha mergedAlpha = IRenderProxyFadeable::CalcMergedDissolve(sc);
	return m_lodSelector.ComputeDissolveValue( lodIndex, sc, mergedAlpha );
}

const Vector IRenderProxyDissolvable::CalcShadowDissolveValuesNoLod( ) const
{
	const auto& sc = m_scene->GetDissolveSynchronizer();
	const CRenderDissolveAlpha mergedAlpha = IRenderProxyFadeable::GetGenericFadeAlpha(sc) * m_shadowFadeAlpha; // temporary fade is not affecting shadows
	return CalculateBiasVectorPatternPositive( mergedAlpha.ToFraction() );
}

const Vector IRenderProxyDissolvable::CalcShadowDissolveValues( const Int32 lodIndex ) const
{
	RED_FATAL_ASSERT( m_scene != nullptr, "Not part of the scene" );

	const auto& sc = m_scene->GetDissolveSynchronizer();

	const CRenderDissolveAlpha mergedAlpha = IRenderProxyFadeable::GetGenericFadeAlpha(sc) * m_shadowFadeAlpha; // temporary fade is not affecting shadows
	return m_lodSelector.ComputeDissolveValue( lodIndex, sc, mergedAlpha );
}


Bool IRenderProxyDissolvable::IsDissolved() const
{
	const auto& sc = m_scene->GetDissolveSynchronizer();
	return IRenderProxyFadeable::IsFading(sc) || m_lodSelector.IsDuringLODChange();
}

Bool IRenderProxyDissolvable::IsShadowDissolved() const
{
	return !m_shadowFadeAlpha.IsOne() && !m_shadowFadeAlpha.IsZero();
}



void IRenderProxyDissolvable::UpdateLOD( const CRenderCollector& collector, const Bool wasVisibleLastFrame, Bool forceLod0 )
{
	UpdateLOD( GetCachedDistanceSquared(), collector, wasVisibleLastFrame, forceLod0 );
}

void IRenderProxyDissolvable::UpdateLOD( const Float altLodDistance, const CRenderCollector& collector, const Bool wasVisibleLastFrame, const Bool forceLod0, const Int8 minLOD, const Int8 maxLOD )
{
	// When there's a  material replacement force no LOD... ( HACK )
	if ( forceLod0 )
	{
		m_lodSelector.ForceLOD(0);
	}
	else
	{
		m_lodSelector.ComputeLOD( altLodDistance, GetCachedDistanceSquared(), wasVisibleLastFrame && !collector.WasLastFrameCameraInvalidated(), collector.GetDissolveSynchronizer(), minLOD, maxLOD );
	}
}


void IRenderProxyDissolvable::UpdateFades( const CRenderCollector& collector, const Bool wasVisibleLastFrame )
{
	const Bool doDissolve = wasVisibleLastFrame && !collector.WasLastFrameCameraInvalidated();

	// complete the fadeout if proxy was not visible last frame
	if ( FT_FadeOutAndDestroy == GetFadeType() && !doDissolve )
	{
		SetGenericFadeZero();
	}
}
