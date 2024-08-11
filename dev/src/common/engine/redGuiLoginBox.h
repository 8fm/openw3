/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#ifndef NO_RED_GUI 

#include "redGuiModalWindow.h"

namespace RedGui
{
	class CRedGuiLoginBox : public CRedGuiModalWindow
	{
		DECLARE_CLASS_MEMORY_POOL( MemoryPool_RedGui, MC_RedGuiControls );
	public:
		CRedGuiLoginBox( const String& uniqueName );
		virtual ~CRedGuiLoginBox();

		Event1_Package EventLoginClicked;
		Event1_Package EventCancelClicked;

		String GetLogin() const;
		String GetPassword() const;

		void SetVisible( Bool value ) override;

	private:
		void CreateControls();

		void OnNotifyCancelClicked( CRedGuiEventPackage& package );
		void OnNotifyLoginClicked( CRedGuiEventPackage& package );
		void OnNotifySavePassCheck(CRedGuiEventPackage& package, Bool value );

		void LoadSettings();
		void SaveSettings() const;

	private:
		CRedGuiTextBox*		m_userTextBox;
		CRedGuiTextBox*		m_passwordTextBox;
		CRedGuiCheckBox*	m_rememberMe;

		String				m_uniqueName;
	};

}	// namespace RedGui

#endif	// NO_RED_GUI
