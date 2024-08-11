/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#ifndef NO_DEBUG_WINDOWS
#include "redGuiWindow.h"
#include "inGameConfig.h"
#include "functional"

namespace DebugWindows
{
	class CDebugWindowInGameConfig : public RedGui::CRedGuiWindow
	{
	public:
		CDebugWindowInGameConfig();
		~CDebugWindowInGameConfig();

	private:
		void CreateControls();
		void AddConfigGroupToPanel( RedGui::CRedGuiTab* tabs, Int32 tabIndex, InGameConfig::IConfigGroup* group );
		void OnChangeTab( RedGui::CRedGuiEventPackage& eventPackage, RedGui::CRedGuiControl* tab );
		void AddConfigGroupPresetableToPanel( RedGui::CRedGuiTab* tabs, Int32 tabIndex, InGameConfig::IConfigGroup* group );
		void AddConfigVarControlsToGrid( RedGui::CRedGuiGridLayout* grid, InGameConfig::IConfigVar* configVar );

		struct SControlToConfigVarMapping
		{
			SControlToConfigVarMapping() {}
			SControlToConfigVarMapping( const RedGui::CRedGuiControl* controlRef, InGameConfig::IConfigVar* var )
				: controlRef( controlRef )
				, var( var )
			{ /* Intentionally Empty */ }

			const RedGui::CRedGuiControl* controlRef;
			InGameConfig::IConfigVar* var;
		};

		struct SPresetControlToConfigGroupMapping
		{
			SPresetControlToConfigGroupMapping() {}
			SPresetControlToConfigGroupMapping( const InGameConfig::SConfigPresetOption presetOption, const RedGui::CRedGuiControl* controlRef, InGameConfig::IConfigGroup* presetable )
				: presetOption( presetOption )
				, controlRef( controlRef )
				, presetable( presetable )
			{ /* Intentionally Empty */ }

			const InGameConfig::SConfigPresetOption presetOption;
			const RedGui::CRedGuiControl* controlRef;
			InGameConfig::IConfigGroup* presetable;
		};

		struct SPresetControlToConfigVarListOfOptMapping
		{
			SPresetControlToConfigVarListOfOptMapping() : optionId(-1) {}
			SPresetControlToConfigVarListOfOptMapping( const Int32 optionId, const RedGui::CRedGuiControl* controlRef, InGameConfig::IConfigVar* configVar )
				: optionId( optionId )
				, controlRef( controlRef )
				, configVar( configVar )
			{ /* Intentionally Empty */ }

			const Int32 optionId;
			const RedGui::CRedGuiControl* controlRef;
			InGameConfig::IConfigVar* configVar;
		};

		struct SControlToConfigVarListOfValuesMapping
		{
			SControlToConfigVarListOfValuesMapping() {}
			SControlToConfigVarListOfValuesMapping( const RedGui::CRedGuiComboBox* controlRef, InGameConfig::IConfigVar* configVar )
				: controlRef( controlRef )
				, configVar( configVar )
			{ /* Intentionally Empty */ }

			const RedGui::CRedGuiComboBox* controlRef;
			InGameConfig::IConfigVar* configVar;
		};

		void OnCheckBoxForConfigVarChanged( RedGui::CRedGuiEventPackage& eventPackage, Bool value );
		void OnPresetButtonForConfigGroupClicked( RedGui::CRedGuiEventPackage& eventPackage );
		void OnPresetButtonForConfigVarClicked(RedGui::CRedGuiEventPackage& eventPackage);
		void OnSaveConfigButtonClicked(RedGui::CRedGuiEventPackage& eventPackage);
		void OnApplySettingsButtonClicked(RedGui::CRedGuiEventPackage& eventPackage);
		void OnResetConfigButtonClicked(RedGui::CRedGuiEventPackage& eventPackage);
		void OnComboBoxForConfigVarChanged(RedGui::CRedGuiEventPackage& eventPackage, Int32 value );
		void OnSliderForConfigVarChanged(RedGui::CRedGuiEventPackage& eventPackage, Float value );
		void OnSpinBoxForConfigVarChanged(RedGui::CRedGuiEventPackage& eventPackage, Int32 value );

		virtual void OnWindowOpened(CRedGuiControl* control);

		// Refreshing Config UI
		void RefreshConfigUI();

	private:
		TDynArray< SControlToConfigVarMapping > m_checkBoxToConfigVarMapping;
		TDynArray< SPresetControlToConfigGroupMapping > m_presetButtonToConfigGroupMapping;
		TDynArray< SPresetControlToConfigVarListOfOptMapping > m_presetButtonToConfigVarMapping;
		TDynArray< SControlToConfigVarListOfValuesMapping > m_ComboBoxToConfigVarListOfValuesMapping;
		TDynArray< SControlToConfigVarMapping > m_SliderToConfigVarSliderMapping;
		TDynArray< SControlToConfigVarMapping > m_SpinBoxToConfigVarSliderMapping;
		RedGui::CRedGuiTab*			m_mainTab;

		typedef std::function< void() > CRefreshCallback;
		Bool m_refreshing;
		Bool m_initializing;
		TDynArray< CRefreshCallback > m_refreshCallbacks;
	};
}

#endif
