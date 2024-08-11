/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#ifndef NO_RED_GUI
#ifndef NO_DEBUG_WINDOWS
#ifdef USE_UMBRA

#include "redGuiWindow.h"
#include "redGuiLabel.h"

class CDebugOptionsTree;

namespace DebugWindows
{
	class CRedGuiUmbraStatLabel : public RedGui::CRedGuiLabel
	{
	public:
		CRedGuiUmbraStatLabel(Uint32 x, Uint32 y, Uint32 width, Uint32 height, const String& baseText )
			: RedGui::CRedGuiLabel( x, y, width, height )
			, m_baseText( baseText )
		{
			Update( String::EMPTY );
		}

		void Update( const String& text )
		{
			SetText( m_baseText + text );
		}

		void Update( Uint32 value )
		{
			Update( String::Printf( TXT("%u"), value ) );
		}

	private:
		String m_baseText;
	};


	class CDebugWindowUmbra : public RedGui::CRedGuiWindow
	{
	private:
		static EMemoryClass MemoryClassesToTrack[];
		struct MemoryInfo
		{
			EMemoryClass				memClass;
			Float						maxSoFar;
			RedGui::CRedGuiProgressBar* memoryBar;
		};

	public:
		CDebugWindowUmbra();
		~CDebugWindowUmbra();

	private:
		void NotifyOnTick( RedGui::CRedGuiEventPackage& eventPackage, Float timeDelta );
		void NotifyChangeAttachCamera( RedGui::CRedGuiEventPackage& eventPackage, Bool value );
		void NotifyViewportInput( RedGui::CRedGuiEventPackage& eventPackage, IViewport* view, enum EInputKey key, enum EInputAction action, Float data );
		void NotifyViewportClick( RedGui::CRedGuiEventPackage& eventPackage, IViewport* view, Int32 button, Bool state, Vector2 mousePosition );
		void NotifyViewportGenerateFragments( RedGui::CRedGuiEventPackage& eventPackage, IViewport* view, CRenderFrame* frame );
		void QueryThresholdChanged( RedGui::CRedGuiEventPackage& eventPackage, Float value );
		void DistanceMultiplierChanged( RedGui::CRedGuiEventPackage& eventPackage, Float value );
		void DumpVisibleMeshes( RedGui::CRedGuiEventPackage& eventPackage );
		void NotifyEventFileOK( RedGui::CRedGuiEventPackage& eventPackage );

		// helpers
		void CreateStatsControls();
		void CreateProgressControls();
		void FillStatsControls();
		void UpdateGenerateUmbraProgress();

		void UpdateMemInfo( Float currentMemory, Float maxMemory, RedGui::CRedGuiProgressBar* progressBar, const String& txt, Float& maxValue );

	private:
		RedGui::CRedGuiCheckBox*	m_attachOcclusionCamera;
		RedGui::CRedGuiButton*		m_dumpMeshesBtn;
		RedGui::CRedGuiGroupBox*	m_timeStatsGroupBox;
		RedGui::CRedGuiGroupBox*	m_reMapStatsGroupBox;
		RedGui::CRedGuiGroupBox*	m_shadowStatsGroupBox;
		RedGui::CRedGuiGroupBox*	m_memoryStatsGroupBox;

		// stats labels
		CRedGuiUmbraStatLabel*		m_visibleObjects;
		CRedGuiUmbraStatLabel*		m_occlusionTime;
		CRedGuiUmbraStatLabel*		m_occlusionQueryTime;
		CRedGuiUmbraStatLabel*		m_occlusionDynamicObjectsTime;
		CRedGuiUmbraStatLabel*		m_visibilityByDistanceTime;
		CRedGuiUmbraStatLabel*		m_furthestProxiesTime;

		// shadows labels
		CRedGuiUmbraStatLabel*		m_shadowLabel;
		CRedGuiUmbraStatLabel*		m_lblShadowQueryTime;

		CRedGuiUmbraStatLabel*		m_lblSTStatic;
		CRedGuiUmbraStatLabel*		m_lblSTStaticDistance;
		CRedGuiUmbraStatLabel*		m_lblSTStaticCollection;
		CRedGuiUmbraStatLabel*		m_lblSTStaticCulledByDistance;

		CRedGuiUmbraStatLabel*		m_lblSTDynamic;
		CRedGuiUmbraStatLabel*		m_lblSTDynamicDistance;
		CRedGuiUmbraStatLabel*		m_lblSTDynamicUmbra;
		CRedGuiUmbraStatLabel*		m_lblSTDynamicCollection;
		CRedGuiUmbraStatLabel*		m_lblShadowVisibleStatic;
		CRedGuiUmbraStatLabel*		m_lblShadowVisibleDynamic;
		
		// RenderElementMap
		CRedGuiUmbraStatLabel*		m_reMapStaticProxies;
		CRedGuiUmbraStatLabel*		m_reMapDynamicProxies;
		CRedGuiUmbraStatLabel*		m_reMapFurthestProxies;
		CRedGuiUmbraStatLabel*		m_reMapDynamicDecals;

		CRedGuiUmbraStatLabel*		m_labelReMapStatsStaticMeshes;
		CRedGuiUmbraStatLabel*		m_labelReMapStatsDynamicMeshes;
		CRedGuiUmbraStatLabel*		m_labelReMapStatsMeshesNotInObjectCache;
		CRedGuiUmbraStatLabel*		m_labelReMapStatsApex;
		CRedGuiUmbraStatLabel*		m_labelReMapStatsBakedDecals;
		CRedGuiUmbraStatLabel*		m_labelReMapStatsNonBakedDecals;
		CRedGuiUmbraStatLabel*		m_labelReMapStatsBakedDimmers;
		CRedGuiUmbraStatLabel*		m_labelReMapStatsNonBakedDimmers;
		CRedGuiUmbraStatLabel*		m_labelReMapStatsBakedStripes;
		CRedGuiUmbraStatLabel*		m_labelReMapStatsNonBakedStripes;
		CRedGuiUmbraStatLabel*		m_labelReMapStatsFlares;
		CRedGuiUmbraStatLabel*		m_labelReMapStatsFur;
		CRedGuiUmbraStatLabel*		m_labelReMapStatsParticles;
		CRedGuiUmbraStatLabel*		m_labelReMapStatsBakedPointLights;
		CRedGuiUmbraStatLabel*		m_labelReMapStatsNonBakedPointLights;
		CRedGuiUmbraStatLabel*		m_labelReMapStatsBakedSpotLights;
		CRedGuiUmbraStatLabel*		m_labelReMapStatsNonBakedSpotLights;

		RedGui::CRedGuiSaveFileDialog*	m_saveFileDialog;
		
		// progress controls
		TDynArray< MemoryInfo* >	m_memInfos;

		RedGui::CRedGuiProgressBar*	m_overallProgressBar;
		
		//
		CRedGuiUmbraStatLabel*		m_queryThresholdLabel;
		RedGui::CRedGuiSlider*		m_queryThresholdSlider;

		CRedGuiUmbraStatLabel*		m_distanceMultiplierLabel;
		RedGui::CRedGuiSlider*		m_distanceMultiplierSlider;

		// Umbra variables
		EulerAngles				m_cameraRotation;		//!< Camera rotation
		Vector					m_cameraPosition;		//!< Camera position
		Vector					m_cameraSpeed;			//!< Camera speed

		Float					m_cameraFov;			//!< Camera FOV
		Float					m_cameraAspect;
		Float					m_cameraNearPlane;
		Float					m_cameraFarPlane;

		Float 					m_cameraSpeedMultiplier;//!< Camera speed multiplier

		Uint32					m_mouseButtonFlags;		//!< Flags for mouse buttons
		Bool					m_moveKeys[7];			//!< Camera move keys

		Vector					m_lastRCPosition;
		EulerAngles				m_lastRCRotation;
		Float					m_lastRCFOV;
		Float					m_lastRCAspect;
		Float					m_lastRCNearPlane;
		Float					m_lastRCFarPlane;
		Bool					m_lastRCReversedProjection;

		Bool					m_attachOcclusionCameraToRenderCamera;
		Bool					m_resetOcclusionCamera;

		Float					m_maxMemOverall;

		Float					m_distanceMultiplier;

		Double					m_shadowTimeStaticObjects;
		Double					m_shadowTimeDynamicObjects;
		Double					m_maxShadowTimeStaticObjects;
		Double					m_maxShadowTimeDynamicObjects;
		Double					m_avgShadowTimeStaticObjects;
		Double					m_avgShadowTimeDynamicObjects;
		Uint32					m_shadowStaticObjectsSamplesCount;
		Uint32					m_shadowDynamicObjectsSamplesCount;
	};

}	// namespace DebugWindows

#endif	// USE_UMBRA
#endif	// NO_DEBUG_WINDOWS
#endif	// NO_RED_GUI
