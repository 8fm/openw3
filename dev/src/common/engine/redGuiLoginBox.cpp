/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#ifndef NO_RED_GUI 

#include "redGuiLoginBox.h"
#include "redGuiLabel.h"
#include "redGuiPanel.h"
#include "redGuiCheckBox.h"
#include "redGuiButton.h"
#include "redGuiTextBox.h"
#include "redGuiManager.h"
#include "../core/configVar.h"
#include "../core/configVarLegacyWrapper.h"

namespace RedGui
{
	CRedGuiLoginBox::CRedGuiLoginBox( const String& uniqueName )
		: CRedGuiModalWindow( 0, 0, 400, 200 )
		, m_uniqueName( uniqueName )
	{
		SetVisibleCaptionButton(CB_Minimize, false);
		SetVisibleCaptionButton(CB_Maximize, false);
		SetVisibleCaptionButton(CB_Exit, false);
		SetBackgroundColor( Color::CLEAR );
		SetResizable(false);
		SetCaption( TXT("Login box") );

		m_uniqueName.RemoveWhiteSpaces();
		m_uniqueName += TXT("LoginBox");

		CreateControls();
	}

	CRedGuiLoginBox::~CRedGuiLoginBox()
	{
		/* intentionally empty */
	}

	void CRedGuiLoginBox::CreateControls()
	{
		CRedGuiPanel* topPanel = new CRedGuiPanel( 0, 0, GetWidth(), 130 );
		topPanel->SetDock( DOCK_Top );
		topPanel->SetBackgroundColor( Color(20, 20, 20, 255) );
		topPanel->SetPadding( Box2( 20, 20, 20 ,20 ) );
		topPanel->SetMargin( Box2( 5, 5, 5, 5 ) );
		AddChild(topPanel);
		{
			CRedGuiPanel* loginPanel = new CRedGuiPanel( 0,0,GetWidth(), 30 );
			loginPanel->SetDock( DOCK_Top );
			loginPanel->SetBackgroundColor( Color::CLEAR );
			loginPanel->SetBorderVisible( false );
			topPanel->AddChild( loginPanel );
			{
				CRedGuiLabel* loginLabel = new CRedGuiLabel( 0, 0, 60, 30 );
				loginLabel->SetDock( DOCK_Left );
				loginLabel->SetAutoSize( false );
				loginLabel->SetMargin( Box2( 5, 8, 5, 5 ) );
				loginLabel->SetText( TXT("User:") );
				loginPanel->AddChild( loginLabel );

				m_userTextBox = new CRedGuiTextBox( 0, 0, 100, 30 );
				m_userTextBox->SetDock( DOCK_Fill );
				m_userTextBox->SetMargin( Box2( 5, 5, 5, 5 ) );
				m_userTextBox->SetMultiLine( false );
				m_userTextBox->SetTabIndex( 0 );
				loginPanel->AddChild( m_userTextBox );
			}

			CRedGuiPanel* passwordPanel = new CRedGuiPanel( 0,0,GetWidth(), 30 );
			passwordPanel->SetDock( DOCK_Top );
			passwordPanel->SetBackgroundColor( Color::CLEAR );
			passwordPanel->SetBorderVisible( false );
			topPanel->AddChild( passwordPanel );
			{
				CRedGuiLabel* passwordLabel = new CRedGuiLabel( 0, 0, 60, 30 );
				passwordLabel->SetAutoSize( false );
				passwordLabel->SetText( TXT("Password:") );
				passwordLabel->SetDock( DOCK_Left );
				passwordLabel->SetMargin( Box2( 5, 8, 5, 5 ) );
				passwordPanel->AddChild( passwordLabel );

				m_passwordTextBox = new CRedGuiTextBox( 0, 0, 100, 30 );
				m_passwordTextBox->SetDock( DOCK_Fill );
				m_passwordTextBox->SetMargin( Box2( 5, 5, 5, 5 ) );
				m_passwordTextBox->SetPasswordMode( true );
				m_passwordTextBox->SetMultiLine( false );
				m_passwordTextBox->SetTabIndex( 1 );
				m_passwordTextBox->EventTextEnter.Bind( this, &CRedGuiLoginBox::OnNotifyLoginClicked );
				passwordPanel->AddChild( m_passwordTextBox );
			}

			CRedGuiPanel* savePasswordPanel = new CRedGuiPanel( 0,0,GetWidth(), 30 );
			savePasswordPanel->SetDock( DOCK_Top );
			savePasswordPanel->SetBackgroundColor( Color::CLEAR );
			savePasswordPanel->SetBorderVisible( false );
			topPanel->AddChild( savePasswordPanel );
			{
				CRedGuiLabel* passwordLabel = new CRedGuiLabel( 0, 0, 60, 30 );
				passwordLabel->SetAutoSize( false );
				passwordLabel->SetText( TXT("") );
				passwordLabel->SetDock( DOCK_Left );
				passwordLabel->SetMargin( Box2( 5, 8, 5, 5 ) );
				savePasswordPanel->AddChild( passwordLabel );

				m_rememberMe = new CRedGuiCheckBox( 0, 0, 15, 15 );
				m_rememberMe->SetText( TXT("Remember me") );
				m_rememberMe->SetDock( DOCK_Fill );
				m_rememberMe->SetMargin( Box2( 5, 5, 5, 5 ) );
				m_rememberMe->SetTabIndex( 2 );
				m_rememberMe->EventCheckedChanged.Bind( this, &CRedGuiLoginBox::OnNotifySavePassCheck );
				savePasswordPanel->AddChild( m_rememberMe );
			}
		}

		//
		CRedGuiPanel* buttonsPanel = new CRedGuiPanel( 0, 0, GetWidth(), 30 );
		buttonsPanel->SetDock( DOCK_Top );
		buttonsPanel->SetBackgroundColor( Color::CLEAR );
		buttonsPanel->SetBorderVisible( false );
		AddChild( buttonsPanel );
		{
			CRedGuiButton* cancelButton = new CRedGuiButton( 0, 0, 100, 30 );
			cancelButton->SetMargin( Box2( 5, 5, 5, 5 ) );
			cancelButton->SetDock( DOCK_Right );
			cancelButton->SetText( TXT("Cancel") );
			cancelButton->SetTabIndex( 4 );
			cancelButton->EventButtonClicked.Bind( this, &CRedGuiLoginBox::OnNotifyCancelClicked );
			buttonsPanel->AddChild( cancelButton );

			CRedGuiButton* loginButton = new CRedGuiButton( 0, 0, 100, 30 );
			loginButton->SetMargin( Box2( 5, 5, 5, 5 ) );
			loginButton->SetDock( DOCK_Right );
			loginButton->SetText( TXT("Login") );
			loginButton->SetTabIndex( 3 );
			loginButton->EventButtonClicked.Bind( this, &CRedGuiLoginBox::OnNotifyLoginClicked );
			buttonsPanel->AddChild( loginButton );
		}
	}

	void CRedGuiLoginBox::OnNotifyCancelClicked( CRedGuiEventPackage& package )
	{
		SetVisible( false );
		EventCancelClicked( this );
	}

	void CRedGuiLoginBox::OnNotifyLoginClicked( CRedGuiEventPackage& package )
	{
		SetVisible( false );
		EventLoginClicked( this );
	}

	String CRedGuiLoginBox::GetLogin() const
	{
		return m_userTextBox->GetText();
	}

	String CRedGuiLoginBox::GetPassword() const
	{
		return m_passwordTextBox->GetText();
	}

	void CRedGuiLoginBox::SetVisible( Bool value )
	{
		if( value == true )
		{
			m_userTextBox->SetText( TXT("") );
			m_passwordTextBox->SetText( TXT("") );
			m_rememberMe->SetChecked( false );

			LoadSettings();
		}
		else
		{
			SaveSettings();
		}

		CRedGuiWindow::SetVisible( value );
	}

	void CRedGuiLoginBox::OnNotifySavePassCheck( CRedGuiEventPackage& package, Bool value )
	{
		if( value == true )
		{
			GRedGui::GetInstance().MessageBox( TXT("Login and password will be save as raw, uncoded text in User.ini file"), TXT("!!! REMEMBER !!!"), RedGui::MESSAGEBOX_Warning );
		}
	}

	void CRedGuiLoginBox::LoadSettings()
	{
		Bool rememberMe = false;
		SConfig::GetInstance().GetLegacy().ReadParam( TXT("user"), m_uniqueName.AsChar(), TXT("RememberMe"), rememberMe );
		m_rememberMe->SetChecked( rememberMe, true );

		if( rememberMe == true )
		{
			String tempValue = String::EMPTY;

			SConfig::GetInstance().GetLegacy().ReadParam( TXT("user"), m_uniqueName.AsChar(), TXT("User"), tempValue );
			m_userTextBox->SetText( tempValue );

			tempValue = String::EMPTY;
			SConfig::GetInstance().GetLegacy().ReadParam( TXT("user"), m_uniqueName.AsChar(), TXT("Password"), tempValue );
			m_passwordTextBox->SetText( tempValue );
		}
	}

	void CRedGuiLoginBox::SaveSettings() const
	{
		Bool rememberMe = m_rememberMe->GetChecked();
		SConfig::GetInstance().GetLegacy().WriteParam( TXT("user"), m_uniqueName.AsChar(), TXT("RememberMe"), rememberMe );
		if( rememberMe == true )
		{
			SConfig::GetInstance().GetLegacy().WriteParam( TXT("user"), m_uniqueName.AsChar(), TXT("User"), m_userTextBox->GetText() );
			SConfig::GetInstance().GetLegacy().WriteParam( TXT("user"), m_uniqueName.AsChar(), TXT("Password"), m_passwordTextBox->GetText() );
		}
		SConfig::GetInstance().GetLegacy().Save();
	}

}	// namespace RedGui

#endif	// NO_RED_GUI
