/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#ifndef NO_RED_GUI
#ifndef NO_DEBUG_WINDOWS

#include "redGuiWindow.h"

class CEnvironmentManager;

namespace DebugWindows
{
	class CDebugWindowEnvironment : public RedGui::CRedGuiWindow
	{
	public:
		CDebugWindowEnvironment();
		~CDebugWindowEnvironment();

	private:
		void SetValuesFromEnvToGui();

		virtual void OnWindowOpened( CRedGuiControl* control );

		void NotifyFakeDaycycleChanged( RedGui::CRedGuiEventPackage& eventPackage, Float value );
		void NotifyFakeDaycycleEnabled( RedGui::CRedGuiEventPackage& eventPackage, Bool value );
		void NotifyFiltersChanged( RedGui::CRedGuiEventPackage& eventPackage, Bool value );
		void NotifyOnTickEvent( RedGui::CRedGuiEventPackage& eventPackage, Float deltaTime );
		void NotifyWeatherPresetSelected( RedGui::CRedGuiEventPackage& eventPackage, Int32 selectedItem );
		void NotifyRefreshLoadedDefinitions( RedGui::CRedGuiEventPackage& eventPackage );

		void CollectVehicles( TDynArray< CAreaEnvironmentComponent* >& components );

	private:		
		RedGui::CRedGuiCheckBox*	m_enableFakeDaycycle;
		RedGui::CRedGuiLabel*		m_fakeTimeLable;
		RedGui::CRedGuiLabel*		m_activeTimeLabel;
		RedGui::CRedGuiSlider*		m_fakeDaycycle;

		RedGui::CRedGuiComboBox*	m_weatherPresets;
		RedGui::CRedGuiLabel*		m_currentWeatherLabel;
		RedGui::CRedGuiLabel*		m_targetWeatherLabel;

		RedGui::CRedGuiCheckBox*	m_instantAdaptionOption;
		RedGui::CRedGuiCheckBox*	m_trajectoryDisplayOption;
		RedGui::CRedGuiCheckBox*	m_envProbeInstantUpdate;
		RedGui::CRedGuiCheckBox*	m_allowEnvProbeUpdateOption;
		RedGui::CRedGuiCheckBox*	m_allowBloomOption;
		RedGui::CRedGuiCheckBox*	m_allowColorBalanceOption;
		RedGui::CRedGuiCheckBox*	m_allowAntialiasingOption;
		RedGui::CRedGuiCheckBox*	m_allowGlobalFogOption;
		RedGui::CRedGuiCheckBox*	m_allowDepthOfFieldOption;
		RedGui::CRedGuiCheckBox*	m_allowSSAOOption;
		RedGui::CRedGuiCheckBox*	m_allowCloudsShadowOption;
		RedGui::CRedGuiCheckBox*	m_allowVignetteOption;
		RedGui::CRedGuiCheckBox*	m_allowWaterShaderOption;
		RedGui::CRedGuiCheckBox*	m_forceCutsceneDOFModeOption;
		RedGui::CRedGuiCheckBox*	m_envAreaDebugFlag;
		RedGui::CRedGuiCheckBox*	m_envProbeInstancesDebugFlag;
		RedGui::CRedGuiCheckBox*	m_envProbeOverlayDebugFlag;

		RedGui::CRedGuiList*		m_activeEnvs;
		RedGui::CRedGuiList*		m_allLoadedEnvs;
		RedGui::CRedGuiButton*		m_refreshInformation;

		CEnvironmentManager*		m_envManager;
	};

}	// namespace DebugWindows

#endif	// NO_DEBUG_WINDOWS
#endif	// NO_RED_GUI
