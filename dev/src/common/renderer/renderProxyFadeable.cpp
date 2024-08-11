/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
* Kamil Nowakowski
*/

#include "build.h"
#include "renderProxyFadeable.h"
#include "renderScene.h"
#include "renderElementMap.h"

using namespace DissolveHelpers;

IRenderProxyFadeable::IRenderProxyFadeable( ERenderProxyType type, const RenderProxyInitInfo& initInfo )
	: IRenderProxyBase( type , initInfo )
	, m_fadeType( FT_None )
{

}


IRenderProxyFadeable::~IRenderProxyFadeable()
{

}

Float IRenderProxyFadeable::GetGenericFadeFraction() const
{
	return m_genericFade.ComputeAlpha( m_scene->GetDissolveSynchronizer() ).ToFraction();
}


Float IRenderProxyFadeable::GetTemporalFadeFraction() const
{
	return m_temporaryFade.ComputeAlpha( m_scene->GetDissolveSynchronizer() ).ToFraction();
}


CRenderDissolveAlpha IRenderProxyFadeable::GetGenericFadeAlpha() const
{
	return m_genericFade.ComputeAlpha( m_scene->GetDissolveSynchronizer() );
}


CRenderDissolveAlpha IRenderProxyFadeable::GetTemporalFadeAlpha() const
{
	return m_temporaryFade.ComputeAlpha( m_scene->GetDissolveSynchronizer() );
}


void IRenderProxyFadeable::FinishGenericFade()
{
	m_genericFade.Finish();
}



Bool IRenderProxyFadeable::IsFadeVisible() const 
{ 
	const auto& sc = m_scene->GetDissolveSynchronizer();
	return !m_genericFade.ComputeAlpha(sc).IsZero() && !m_temporaryFade.ComputeAlpha(sc).IsZero();
}

const Vector IRenderProxyFadeable::CalcDissolveValues() const
{
	const auto& sc = m_scene->GetDissolveSynchronizer();
	return CalculateBiasVectorPatternPositive( CalcMergedDissolve(sc).ToFraction() );
}


bool IRenderProxyFadeable::IsFadeAndDestroyFinished( Int32 currFrameIndex )
{
	// No fading by default, so as soon as we're FadeOutAndDestroy, it's finished.
	if ( FT_FadeOutAndDestroy != GetFadeType() )
	{
		return false;
	}

	// We can be destroyed after we fade out
	return m_genericFade.ComputeAlpha( m_scene->GetDissolveSynchronizer() ).IsZero();
}

void IRenderProxyFadeable::UpdateDistanceFade( Float currentDistanceSq , Float autoHideDistanceSq, Bool doFade )
{
	EFadeType prevFadeType = m_fadeType;
	if( m_fadeType != FT_FadeOutAndDestroy )
	{
		if ( currentDistanceSq < autoHideDistanceSq )
		{
			if ( ! GetGenericFadeAlpha().IsOne() )
			{
				SetFadeType( FT_FadeIn );
			}
			else
			{
				SetFadeType( FT_None );
			}
		}
		else
		{
			// Beyond autohide distance. If we aren't faded out, or current fade mode is FadeIn, fade out.
			if ( ! GetGenericFadeAlpha().IsZero() || m_fadeType == FT_FadeIn || m_fadeType == FT_FadeInStart )
			{
				SetFadeType( FT_FadeOut );
			}
			else
			{
				SetFadeType( FT_None );
			}
		}
	}

	// If we weren't doing a new fade-in, and we weren't visible previously, finish the fade now.
	// If we went FadeInStart->FadeIn, then we successfully started fading in, and don't want to finish it yet. If we went
	// FadeInStart->FadeOut, then we aren't fading in, and should finish the fadeout.
	if ( !( prevFadeType == FT_FadeInStart && m_fadeType == FT_FadeIn ) && !doFade )
	{
		FinishGenericFade();
	}
}


void IRenderProxyFadeable::SetFadeType( EFadeType type )
{
	// Update fade type
	if ( m_fadeType != type )
	{

		// we need to be in a scene to set the fadein/fade out stuff
		if ( m_scene != nullptr )
		{
			Bool fadingOut = false;

			auto& synchronizer = m_scene->GetDissolveSynchronizer();
			
			// Is current render frame isn't invalidated
			const Bool invalidatedFrame = synchronizer.IsFinishPending() ;

			// Change the disappear value
			if( invalidatedFrame )
			{
				switch( type )
				{
				case FT_FadeIn:
				case FT_FadeInStart:
					m_genericFade.SetAlphaOne();	// reset
					break;

				case FT_FadeOut:
				case FT_FadeOutAndDestroy:
					m_genericFade.SetAlphaZero();	// reset
					fadingOut = true;
					break;
				}
			}
			else
			{
				switch( type )
				{
				case FT_FadeIn:
					m_genericFade.FadeIn( synchronizer ); // no reset
					break;

				case FT_FadeInStart:
					m_genericFade.StartFadeIn( synchronizer ); // reset state
					break;

				case FT_FadeOut:
				case FT_FadeOutAndDestroy:
					m_genericFade.FadeOut( synchronizer ); // no reset
					fadingOut = true;
					break;
				}
			}

			// If we're registered in the render element map, and we're fading out, set flag so we can be drawn even
			// past the autohide distance.
			if ( m_scene->GetRenderElementMap() && GetRegCount() > 0 )
			{
				m_scene->GetRenderElementMap()->SetProxyFadingOut( GetEntryID(), fadingOut );
			}
		}

		if ( FT_FadeOutAndDestroy != m_fadeType )
		{
			m_fadeType = type; 

			if ( FT_FadeOutAndDestroy == type )
			{
				GetScene()->RegisterFadeOutRemoval( this );
			}
		}
		else
		{
			// FadeOutAndDestroy state is considered permanent, so just log warning.
			// It's meant to be called before objects disposal.
			WARN_RENDERER( TXT("FadeOutAndDestroy renderProxyDrawable state change prevented.") );
		}
	}
}


void IRenderProxyFadeable::SetTemporaryFade()
{
	// hide harshly and allow to fade in, unless we get another call for temporary fade (it's being sent every frame)
	if ( GetScene() != nullptr )
	{
		m_temporaryFade.SetAlphaZero();
		m_temporaryFade.StartFadeIn( GetScene()->GetDissolveSynchronizer() );
	}
}
