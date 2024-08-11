/**
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */
#include "build.h"

#include "communitySystem.h"
#include "communityData.h"
#include "../engine/debugPage.h"
#include "../engine/debugPageManagerBase.h"
#include "../engine/renderFrame.h"


#ifndef NO_DEBUG_PAGES

/// Debug page with community informations on screen log
class CDebugPageCommunityLog : public IDebugPage, public ICommunityDebugPage
{
private:
	struct Msg
	{
		String		m_msg;
		Color		m_color;
		Msg( const String& msg = TXT( "" ), const Color& color = Color::WHITE ) : m_msg( msg ), m_color( color ) {}
	};

protected:
	Float					m_refreshTimer; // data refresh timer
	Int32						m_msgShift;
	TDynArray< Msg >		m_messages;
	Int32						m_subPageCurrent;
	Int32						m_subPageMax;
	Vector					m_cameraTarget;			//!< Current camera target
	Vector					m_cameraPosition;		//!< Current camera position
	Float					m_cameraRotation;		//!< NPC camera rotation

public:
	CDebugPageCommunityLog()
		: IDebugPage( TXT("Community Log") )
		, m_refreshTimer( 0 )
		, m_msgShift( 0 )
		, m_subPageCurrent( 0 )
		, m_subPageMax( 0 )
		, m_cameraRotation( 0.0f )
		, m_cameraTarget( Vector::ZEROS )
		, m_cameraPosition( Vector::ZEROS )
	{
	};

	~CDebugPageCommunityLog()
	{
	}

	//! This debug page was shown
	virtual void OnPageShown()
	{
		IDebugPage::OnPageShown();

		// Clear data
		m_subPageCurrent = 0;
		m_messages.Clear();

		// Reset camera
		if( GGame != nullptr && GGame->GetActiveWorld() != nullptr && GGame->GetActiveWorld()->GetCameraDirector() != nullptr )
		{
			m_cameraTarget = GGame->GetActiveWorld()->GetCameraDirector()->GetCameraPosition();
		}
		m_cameraPosition = m_cameraTarget;

		// Pause game
		GGame->Pause( TXT( "CDebugPageCommunityLog" ) );
	}

	//! This debug page was hidden
	virtual void OnPageHidden()
	{
		IDebugPage::OnPageHidden();

		// Unpause game
		GGame->Unpause( TXT( "CDebugPageCommunityLog" ) );
	}

	virtual void OnTick( Float timeDelta )
	{
		CCommunitySystem *cs = GCommonGame->GetSystem< CCommunitySystem >();
		if ( !cs ) return;

		m_refreshTimer += timeDelta;
		if ( m_refreshTimer > 1.0f ) // update every second
		{
			m_refreshTimer = 0;
			m_messages.ClearFast();
			cs->DebugUpdateStatusPage( m_subPageCurrent, *this );
		}

		UpdateCameraPosition( timeDelta );
	}

	void UpdateCameraPosition( Float timeDelta )
	{
		// Update virtual camera rotation
		m_cameraRotation += timeDelta * 30.0f;

		// Interpolate
		Vector cameraDist = m_cameraTarget - m_cameraPosition;
		const Float dist = cameraDist.Mag3();
		if ( dist > 0.0f )
		{
			// Fly to target
			const Float move = Min< Float >( dist, timeDelta * 20.0f );
			m_cameraPosition += cameraDist.Normalized3() * move;
		}
	}

	virtual void OnViewportGenerateFragments( IViewport *view, CRenderFrame *frame )
	{
		// Header
		Int32 x = 55;
		Int32 y = 65;
		frame->AddDebugScreenText( x, y, TXT("Community System info:") );
		y += 15;

		KeepMsgShiftInBounds( (Int32)m_messages.Size() );

		// List
		Uint32 xCol0 = x + 10;			// First column
		Int32 maxCount = 30;
		for ( Int32 i = m_msgShift; i < (Int32)m_messages.Size() && maxCount > 0; ++i, --maxCount )
		{
			frame->AddDebugScreenText( xCol0, y, m_messages[i].m_msg, m_messages[i].m_color );

			// Move down
			y += 15;
		}

		if ( m_subPageCurrent > 1 )
		{
			// if we're viewing a stub status, highlight its position
			frame->AddDebugSphere( m_cameraTarget, 1.0f, Matrix::IDENTITY, Color::BLUE );
		}
	}

	virtual Bool OnViewportInput( IViewport* view, enum EInputKey key, enum EInputAction action, Float data )
	{
		// Send the event
		if ( action == IACT_Press )
		{
			switch ( key )
			{
			case IK_Down:
				ScroolMsgDown();
				break;
			case IK_Up:
				ScroolMsgUp();
				break;
			case IK_Pad_Y_TRIANGLE:
			case IK_Tab:
				NextSubPage();
				break;
			case IK_V:
				SwitchCommunityVisibilityRadiuses();
				break;
			}
		}

		// Not processed
		return false;
	}

private:
	void ScroolMsgDown( Int32 size = 1 )
	{
		m_msgShift += size;

		KeepMsgShiftInBounds( (Int32)m_messages.Size() );
	}

	void ScroolMsgUp( Int32 size = 1 )
	{
		m_msgShift -= size;
		if ( m_msgShift < 0 ) m_msgShift = 0;
	}

	void KeepMsgShiftInBounds( Int32 maxSize )
	{
		if ( m_msgShift >= maxSize ) m_msgShift = maxSize - 1;
		if ( m_msgShift < 0 ) m_msgShift = 0;
	}

	void NextSubPage()
	{
		CCommunitySystem *cs = GCommonGame->GetSystem< CCommunitySystem >();
		if ( cs )
		{
			m_subPageMax = cs->DebugGetNumStatusPages();
		}
		else
		{
			m_subPageMax = 2;
		}

		if ( ++m_subPageCurrent == m_subPageMax )
		{
			m_subPageCurrent = 0;
		}

		if ( m_subPageCurrent <= 1 )
		{
			// Reset camera
			m_cameraTarget = GGame->GetActiveWorld()->GetCameraDirector()->GetCameraPosition();
		}
	}

	void SwitchCommunityVisibilityRadiuses()
	{
		CCommunitySystem *cs = GCommonGame->GetSystem< CCommunitySystem >();
		if ( !cs ) return;

		cs->EnableVisibilityRadiusChange( !cs->IsVisibilityRadiusChangeEnalbed() );
		if ( !cs->IsVisibilityRadiusChangeEnalbed() )
		{
			cs->RestoreDefaultVisibilityRadiuses();
		}
	}

	//! Override camera
	virtual Bool OnViewportCalculateCamera( IViewport* view, CRenderCamera &camera )
	{
		// Show the selected stub :)
		if ( m_subPageCurrent > 1 )
		{
			// Orbit camera around NPC
			Vector pos = m_cameraPosition;
			pos.Z += 1.0f;
			EulerAngles rot( 0.0f, 0.0f, m_cameraRotation );

			// Offset camera
			Vector dir, right;
			rot.ToAngleVectors( &dir, &right, NULL );
			pos -= dir * 1.5f;

			// Construct preview camera
			CRenderCamera previewCamera( pos, rot, camera.GetFOV(), camera.GetAspect(), camera.GetNearPlane(), camera.GetFarPlane(), camera.GetZoom() );
			camera = previewCamera;
			return true;
		}

		// Not
		return false;
	}

	// -----------------------------------------------------------------------
	// ICommunityDebugPage implementation
	// -----------------------------------------------------------------------
	virtual void AddText( const String& str, const Color& color ) 
	{
		m_messages.PushBack( Msg( str, color ) );
	}

	virtual void FocusOn( const Vector& pos )
	{
		m_cameraTarget = pos;
	}
};

void CreateDebugPageCommunityLog()
{
	IDebugPage* page = new CDebugPageCommunityLog();
	IDebugPageManagerBase::GetInstance()->RegisterDebugPage( page );
}

#endif