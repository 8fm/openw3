/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#ifndef NO_RED_GUI
#ifndef NO_DEBUG_WINDOWS

#include "redGuiWindow.h"

namespace DebugWindows
{
	class CDebugWindowPhysics : public RedGui::CRedGuiWindow
	{
		enum ETabType
		{
			TT_Physics,
			TT_PhysicsSlider,
			TT_CharacterController,
			TT_CharacterControllerJumpFalling,
			TT_CharacterControllerSwimmingDiving,

			TT_Count
		};

	public:
		CDebugWindowPhysics();
		~CDebugWindowPhysics();

	private:
		virtual void OnWindowOpened( CRedGuiControl* control );

		void NotifyOnTick( RedGui::CRedGuiEventPackage& eventPackage, Float deltaTime );
		void NotifyOnTabChanged( RedGui::CRedGuiEventPackage& eventPackage, RedGui::CRedGuiControl* selectedTab );
		void NotifyOnSliderChanged( RedGui::CRedGuiEventPackage& eventPackage, Float value );
		void NotifyOnCheckedChanged( RedGui::CRedGuiEventPackage& eventPackage, Bool value );
		void NotifyButtonClicked( RedGui::CRedGuiEventPackage& eventPackage );

		void NotifyButtonClickedStats( RedGui::CRedGuiEventPackage& eventPackage );

		void CreateControls();
		void CreateSlider( RedGui::CRedGuiControl* parent, Float min, Float max, Float step, const String& name, const String& userDataType, RedGui::RedGuiAny userData );
		void CreateCheckBox( RedGui::CRedGuiControl* parent, const String& name, RedGui::RedGuiAny userData );

		void UpdateLeftInfoPanel();
		void UpdateMiddleInfoPanel();
		void UpdateRightInfoPanel();

		void AddPhysicsTab();
		void AddPhysicsSlidersTab();
		void AddCharacterControllerTab();
		void AddCharacterControllerJumpFallingTab();
		void AddCharacterControllerSwimmingDivingTab();

	private:
		// tabs with options
		RedGui::CRedGuiTab*		m_tabs;

		RedGui::CRedGuiButton*	m_dumpComponentsWithPhysics;				//!<
		RedGui::CRedGuiButton*	m_dumpCollisionCache;				//!<


		//
		RedGui::CRedGuiAdvancedSlider* m_offsetMovingSwimmingslider;
		RedGui::CRedGuiAdvancedSlider* m_emergingSpeedSlider;

		// left info column
		RedGui::CRedGuiLabel*	m_wind;
		RedGui::CRedGuiLabel*	m_water;
		RedGui::CRedGuiLabel*	m_camera;
		RedGui::CRedGuiLabel*	m_character;
		RedGui::CRedGuiLabel*	m_physicalSounds;

		// middle info column
		RedGui::CRedGuiLabel*	m_terrainTiles;
		RedGui::CRedGuiLabel*	m_staticBodies;
		RedGui::CRedGuiLabel*	m_simpleBodies;
		RedGui::CRedGuiLabel*	m_ragdolls;
		RedGui::CRedGuiLabel*	m_destructions;
		RedGui::CRedGuiLabel*	m_cloth;

		// right info column
		RedGui::CRedGuiLabel*	m_activeConstraints;
		RedGui::CRedGuiLabel*	m_dynamicActiveBodies;
		RedGui::CRedGuiLabel*	m_activeKinematicBodies;
		RedGui::CRedGuiLabel*	m_solverConstraints;
		RedGui::CRedGuiLabel*	m_contactSize;
		RedGui::CRedGuiLabel*	m_pairs;
		RedGui::CRedGuiLabel*	m_addBroadphase;
		RedGui::CRedGuiLabel*	m_removeBroadphase;

		// logic
		TDynArray< RedGui::CRedGuiCheckBox* > m_checkboxes;
		TDynArray< RedGui::CRedGuiAdvancedSlider* > m_sliders;

	};

}	// namespace DebugWindows

#endif	// NO_DEBUG_WINDOWS
#endif	// NO_RED_GUI
