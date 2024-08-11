/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#ifndef NO_RED_GUI
#ifndef NO_DEBUG_WINDOWS

#include "../engine/redGuiWindow.h"

class CVehicleComponent;

namespace DebugWindows
{
	class CDebugWindowVehicleViewer : public RedGui::CRedGuiWindow
	{
		struct SVehicleEntry
		{
			SVehicleEntry();
			SVehicleEntry( CVehicleComponent *stickerComponent, String entityName, CEntity *stickerEntity );

			CVehicleComponent*	m_vehicleComponent;
			CEntity*			m_vehicleEntity;
			String				m_entityName;
		};

	public:
		CDebugWindowVehicleViewer();
		~CDebugWindowVehicleViewer();

	private:
		void CollectVehicles();
		void RefreshInformation();
		void SyncToNewVehicle(Int32 npcIndex);
		void ShowInfoAboutVehicle(Int32 npcIndex);
		Vector CalculateCameraOrbit( CEntity* vehicle );

		virtual void OnWindowClosed(CRedGuiControl* control);
		virtual void OnWindowOpened(CRedGuiControl* control);

		// red gui callback functions
		void NotifyOnFrameTick( RedGui::CRedGuiEventPackage& eventPackage, Float timeDelta);
		void NotifyOnButtonClicked( RedGui::CRedGuiEventPackage& package );
		void NotifyOnButtonDoubleClicked( RedGui::CRedGuiEventPackage& eventPackage, Int32 value);
		void NotifyEventSelectedItemChanged( RedGui::CRedGuiEventPackage& eventPackage, Int32 value);
		void NotifyEventToggleButtonChange( RedGui::CRedGuiEventPackage& eventPackage, Bool value);
		void NotifyViewportCalculateCamera( RedGui::CRedGuiEventPackage& eventPackage, IViewport* view, CRenderCamera& camera );
		void NotifyViewportInput( RedGui::CRedGuiEventPackage& eventPackage, IViewport* view, enum EInputKey key, enum EInputAction action, Float data );

		void SetGeneralInformation(const SVehicleEntry& entry);

		RedGui::CRedGuiList*		m_vehiclesList;
		RedGui::CRedGuiButton*		m_refreshList;
		RedGui::CRedGuiButton*		m_stopRotate;
		RedGui::CRedGuiButton*		m_teleportToVehicle;
		RedGui::CRedGuiButton*		m_releaseCamera;
		RedGui::CRedGuiPanel*		m_vehicleInfoPanel;
		RedGui::CRedGuiTab*			m_vehicleInfoCategories;

		// general
		RedGui::CRedGuiLabel*		m_generalTemplateName;
		RedGui::CRedGuiLabel*		m_generalMemoryUsageByTemplate;
		RedGui::CRedGuiLabel*		m_generalTags;
		RedGui::CRedGuiLabel*		m_generalAppearance;
		RedGui::CRedGuiLabel*		m_generalState;

		TDynArray< SVehicleEntry >	m_vehicleComponents;
		TDynArray< String >			m_vehicleNames;			//!< vehicle names
		Vector						m_cameraTarget;			//!< Current camera target
		Vector						m_cameraPosition;		//!< Current camera position
		String						m_activeVehicleName;	//!< Name of the selected NPC
		Int32						m_active;				//!< Active NPC
		Float						m_cameraRotation;		//!< NPC camera rotation
		Int32						m_verticalOffset;		//!< Verical offset of text
		Bool						m_pauseRotation;		//!< 
		Bool						m_teleportPlayer;		//!< 
	};

}	// namespace DebugWindows

#endif	// NO_DEBUG_WINDOWS
#endif	// NO_RED_GUI
