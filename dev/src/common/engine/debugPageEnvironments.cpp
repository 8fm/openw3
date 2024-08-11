/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#ifndef NO_DEBUG_PAGES

#include "debugPageParam.h"
#include "debugWindowsManager.h"
#include "debugPage.h"
#include "debugCheckBox.h"
#include "debugPageManagerBase.h"
#include "inputBufferedInputEvent.h"
#include "inputKeys.h"
#include "environmentManager.h"
#include "environmentGameParams.h"
#include "gameTime.h"
#include "game.h"
#include "environmentComponentArea.h"
#include "renderFrame.h"
#include "worldIterators.h"
#include "component.h"
#include "entity.h"

/// Special check box option for activating envs
class CDebugCheckBoxEnvironment : public IDebugCheckBox
{
protected:
	THandle<CAreaEnvironmentComponent>		m_component;

public:
	CDebugCheckBoxEnvironment( IDebugCheckBox* parent, CAreaEnvironmentComponent* component )
		: IDebugCheckBox( parent, component->GetEntity() ? component->GetEntity()->GetName() : TXT("<name invalid>"), false, true )
		, m_component( component )
	{};

	//! Is this item checked ?
	virtual Bool IsChecked() const
	{
		CAreaEnvironmentComponent* component = m_component.Get();
		if ( component )
		{
			return component->IsActive();
		}
		return false;
	}

	//! Toggle state of this item, only for check items
	virtual void OnToggle()
	{
		CAreaEnvironmentComponent* component = m_component.Get();
		if ( component )
		{
			if ( component->IsActive() )
			{
				component->Deactivate( true );
			}
			else
			{
				component->Activate( true );
			}
		}
	}
};

/// Special check box option for activating envs
class CDebugCheckBoxOverride : public IDebugCheckBox
{
protected:
	Bool*	m_override;

public:
	CDebugCheckBoxOverride( IDebugCheckBox* parent, Bool& override )
		: IDebugCheckBox( parent, TXT("Force daytime override"), false, true )
		, m_override( &override )
	{};

	//! Is this item checked ?
	virtual Bool IsChecked() const
	{
		return *m_override;
	}

	//! Toggle state of this item, only for check items
	virtual void OnToggle()
	{
		*m_override = !*m_override;
	}
};

/// Debug page with environments status
class CDebugPageEnvironments : public IDebugPage
{
protected:
	CDebugOptionsTree*	m_tree;
	Float				m_time;
	Bool				m_override;

protected:
	enum 
	{
		// Offsets X
		OFFX_LEFTMOST	= 55,
			OFFX_INDEX		= OFFX_LEFTMOST,
		OFFX_PRIORITY	= 75,
		OFFX_WEIGHT		= 140,
		OFFX_GLOBAL		= 195,
		OFFX_NAME		= 260,

		// Offsets Y
		OFFY_TITLE		= 65,
		OFFY_ENTRIES	= 95
	};

	void DumpNoWorldInfo( CRenderFrame *frame )
	{
		frame->AddDebugScreenText( OFFX_LEFTMOST, OFFY_ENTRIES, TXT("World not present"), Color::RED );
	}

	void DumpEmptyInfo( CRenderFrame *frame )
	{
		frame->AddDebugScreenText( OFFX_LEFTMOST, OFFY_ENTRIES, TXT("No area environments found"), Color::WHITE );
	}

	void DumpTitles( CRenderFrame *frame )
	{
		frame->AddDebugScreenText( OFFX_INDEX,		OFFY_TITLE, TXT("INDEX"),		Color::YELLOW );
		frame->AddDebugScreenText( OFFX_GLOBAL,		OFFY_TITLE, TXT("IS_GLOBAL"),	Color::YELLOW );
		frame->AddDebugScreenText( OFFX_NAME,		OFFY_TITLE, TXT("NAME"),		Color::YELLOW );	
	}

	void DumpAreaEnvInfo( CRenderFrame *frame, Uint32 index, CEnvironmentManager *manager, const CAreaEnvironmentComponent *areaEnv )
	{	
		String	name	= areaEnv->GetEntity() ? areaEnv->GetEntity()->GetName() : TXT("<name invalid>");
		Int32		y		= OFFY_ENTRIES + 13 * index;
		Color	color	= areaEnv->IsActive() ? Color(255, 255, 255) : Color(128, 128, 128);

		frame->AddDebugScreenText( OFFX_INDEX,		y, String::Printf(TXT("%i"),(Int32)index ),						color );
		frame->AddDebugScreenText( OFFX_NAME,		y, name,														color );
	}

public:
	CDebugPageEnvironments()
		: IDebugPage( TXT("Environments") )
	{
		m_tree = NULL;
	};

	//! This debug page was shown
	virtual void OnPageShown()
	{
		IDebugPage::OnPageShown();

		if ( !m_tree )
		{
			// Create debug tree
			m_tree = new CDebugOptionsTree( 370, 65, 320, 500, this );
			

			THandle< CWorld > world = GGame ? GGame->GetActiveWorld() : nullptr;
			if ( NULL == world.Get() )
			{
				return;
			}

			CEnvironmentManager *envManager = world->GetEnvironmentManager();
			ASSERT( NULL != envManager );

			CGameEnvironmentParams& envGameParams = envManager->GetGameEnvironmentParams();

			m_override = envGameParams.m_dayCycleOverride.m_fakeDayCycleEnable;
			m_time = envManager->GetCurrentGameTime().ToFloat() / (60.0f * 60.0f);

			IDebugCheckBox* overrideCheckBox = new CDebugCheckBoxOverride( NULL, m_override );

			// Force override
			m_tree->AddRoot( overrideCheckBox );

			// Time slider
			m_tree->AddRoot( new CDebugSliderParam( NULL, m_time, TXT("Day time"), 0.f, 24.f, 1.0f ) );

			// Get area environments

			TDynArray< CAreaEnvironmentComponent* > areaEnvComponents;
			{
				for ( WorldAttachedComponentsIterator it( world ); it; ++it )
				{
					CAreaEnvironmentComponent *component = Cast< CAreaEnvironmentComponent > ( *it );
					if (component)
					{
						areaEnvComponents.PushBack( component );
					}
				}
			}

			for ( Uint32 i=0; i<areaEnvComponents.Size(); ++i )
			{
				m_tree->AddRoot( new CDebugCheckBoxEnvironment( NULL, areaEnvComponents[i]) );
			}

			m_tree->SelectItem( overrideCheckBox );
		}
	}

	//! This debug page was hidden
	virtual void OnPageHidden()
	{
		IDebugPage::OnPageHidden();

		if ( m_tree )
		{
			delete m_tree;
			m_tree = NULL;
		}
	}

	//! Generate debug viewport fragments
	virtual void OnViewportGenerateFragments( IViewport *view, CRenderFrame *frame )
	{
#ifndef NO_DEBUG_WINDOWS
		String message = TXT("This debug page is converted to debug window. If you want use it, click key: Enter.");

		frame->AddDebugRect(49, 80, 502, 20, Color(100,100,100,255));
		frame->AddDebugScreenFormatedText( 60, 95, Color(255, 255, 255, 255), TXT("Debug Message Box"));

		frame->AddDebugRect(50, 100, 500, 50, Color(0,0,0,255));
		frame->AddDebugFrame(50, 100, 500, 50, Color(100,100,100,255));

		frame->AddDebugScreenFormatedText( 60, 120, Color(127, 255, 0, 255), message.AsChar());

		frame->AddDebugScreenFormatedText( 275, 140, Color(255, 255, 255), TXT(">> OK <<"));
		return;
#endif

		IDebugPage::OnViewportGenerateFragments( view, frame );

		if ( m_tree )
		{
			m_tree->OnRender( frame );		

			THandle< CWorld > world = GGame ? GGame->GetActiveWorld() : nullptr;
			if ( NULL == world.Get() )
			{
				DumpTitles( frame );
				DumpNoWorldInfo( frame );
				return;
			}

			CEnvironmentManager *envManager = world->GetEnvironmentManager();
			ASSERT( NULL != envManager );

			// Get area environments
			TDynArray< CAreaEnvironmentComponent* > areaEnvComponents;
			{
				for ( WorldAttachedComponentsIterator it( world ); it; ++it )
				{
					CAreaEnvironmentComponent *component = Cast< CAreaEnvironmentComponent > ( *it );
					if (component)
					{
						areaEnvComponents.PushBack( component );
					}
				}
			}

			
			// Display area environments

			DumpTitles( frame );

			if ( areaEnvComponents.Empty() )
			{
				DumpEmptyInfo( frame );
			}
			else
			{
				for ( Uint32 i=0; i<areaEnvComponents.Size(); ++i )
				{
					DumpAreaEnvInfo( frame, i, envManager, areaEnvComponents[i] );
				}
			}

		}
	}

	virtual Bool OnViewportInput( IViewport* view, enum EInputKey key, enum EInputAction action, Float data )
	{
#ifndef NO_RED_GUI
#ifndef NO_DEBUG_WINDOWS
		if( action == IACT_Press && key == IK_Enter)
		{
			GDebugWin::GetInstance().SetVisible(true);
			GDebugWin::GetInstance().ShowDebugWindow( DebugWindows::DW_Environment );
		}
		return true;
#endif	// NO_DEBUG_WINDOWS
#endif	// NO_RED_GUI

		// Send the event
		if ( m_tree->OnInput( key, action, data ) )
		{
			return true;
		}

		// Not processed
		return false;
	}

	virtual void OnTick( Float timeDelta )
	{
		IDebugPage::OnTick( timeDelta );

		m_tree->OnTick( timeDelta );

		THandle< CWorld > world = GGame ? GGame->GetActiveWorld() : nullptr;
		if ( NULL == world.Get() )
		{
			return;
		}

		CEnvironmentManager *envManager = world->GetEnvironmentManager();
		ASSERT( NULL != envManager );

		envManager->GetGameEnvironmentParams().m_dayCycleOverride.m_fakeDayCycleEnable = m_override;

		if ( m_override )
		{
			envManager->GetGameEnvironmentParams().m_dayCycleOverride.m_fakeDayCycleHour = m_time;
		}
	}
};

void CreateDebugPageEnvironments()
{
	IDebugPage* page = new CDebugPageEnvironments();
	IDebugPageManagerBase::GetInstance()->RegisterDebugPage( page );
}

#endif