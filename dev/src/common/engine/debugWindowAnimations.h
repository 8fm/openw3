/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#ifndef NO_RED_GUI
#ifndef NO_DEBUG_WINDOWS

#include "redGuiWindow.h"
#include "skeletalAnimationSet.h"
#include "skeleton.h"
#include "poseProviderStats.h"
#include "../core/diskFile.h"
#include "../core/2darray.h"


namespace DebugWindows
{
	class CDebugWindowAnimations : public RedGui::CRedGuiWindow
	{
		enum ETabName
		{
			TN_Poses,
			TN_Components,
			TN_RestDetails,
			TN_Animsets,
			TN_Cutscenes,

			TN_Count
		};

		enum EComponentType
		{
			CT_All,
			CT_Actor,
			CT_Head,
			CT_Item,
			CT_StaticCamera,
			CT_Camera,
			CT_Rest,

			CT_Count
		};

		struct SComponentInfo
		{
			Uint32	m_num;
			Uint32	m_frozenNum;
			Uint32	m_budgetedNum;

			Uint32	m_inCs;
			Uint32	m_inBehContr;
			Uint32	m_inWork;
		};

		struct SSkeletonInfo
		{
			THandle< CSkeleton >		m_skeleton;
			String						m_name;
			SPoseProviderStats			m_stats;

			SSkeletonInfo( CSkeleton* skeleton )
				: m_skeleton( skeleton )
			{
				m_name = skeleton->GetDepotPath().StringAfter( TXT("\\"), true );
			}

		};

		struct SAnimSetInfo
		{
			THandle< CSkeletalAnimationSet >		m_set;						//!< Handle to animation set
			String									m_name;						//!< Name of the loaded animation set
			Uint32									m_numActiveAnimations;		//!< Number of active animations in the animation set
			Uint32									m_totalUsedMemory;			//!< Total used memory for animations
			Uint32									m_totalUsedMemoryForAnimation;//!< Total used memory for animations motion extraction
			Uint32									m_totalUsedMemoryForAnimationStreamed;//!< Total used memory for animations streamed in
			Uint32									m_wholeAnimationDataMemory;	//!< Total animation data memory, even not streamed in
			Uint32									m_totalUsedMemoryForMotionEx;//!< Total used memory for animations motion extraction
			Uint32									m_totalUsedMemoryForCompressedFrame;//!< Total used memory for animations motion extraction
			Uint32									m_totalUsedMemoryForCompressedFrameData;//!< Total used memory for animations motion extraction
			Uint32									m_activeUsedMemory;			//!< Memory used for active animations
			Color									m_highLightColor;			//!< Highlight color
			Color									m_color;					//!< Normal display color

			SAnimSetInfo( CSkeletalAnimationSet* animSet )
				: m_set( animSet )
				, m_numActiveAnimations( 0 )
				, m_highLightColor( Color::RED )
				, m_color( Color::WHITE )
			{
				CDiskFile* file = animSet->GetFile();
				if ( file != nullptr )
				{
					CFilePath path( file->GetDepotPath() );
					m_name = path.GetFileName();
				}
				else
				{
					m_name = TXT("Unknown AnimSet");
				}

				SAnimationBufferStreamingOption streamingOption;
				Uint32 nonStreamableBones;
				animSet->GetStreamingOption(streamingOption, nonStreamableBones);
				String streamingInfo;
				if (streamingOption == ABSO_FullyStreamable)
				{
					streamingInfo = TXT("fully");
					m_color = Color::LIGHT_BLUE;
				}
				if (streamingOption == ABSO_PartiallyStreamable)
				{
					streamingInfo = TXT("partially");
					m_color = Color::LIGHT_GREEN;
				}
				if (streamingOption == ABSO_NonStreamable)
				{
					streamingInfo = TXT("NON");
					m_color = Color::LIGHT_RED;
				}

				if (! streamingInfo.Empty())
				{
					m_name += TXT(" [")+streamingInfo+TXT("]");
				}
			}
		};


	public:
		CDebugWindowAnimations();
		~CDebugWindowAnimations();

	private:
		void OnWindowOpened( CRedGuiControl* control );

		void NotifyOnTick( RedGui::CRedGuiEventPackage& eventPackage, Float deltaTime );
		void CalcNumStreamingAndLoadedAnim( Uint32& numStr, Uint32& numLoaded, Float& loadedAnimationSize ) const;
		void UpdateProgressBar( RedGui::CRedGuiProgressBar* progress, Float value, Float range, const String& text, const String& unit );

		String EnumToString( ETabName value ) const;
		
		// create tabs
		void CreateTabs();
		void CreatePosesTab();
		void CreateComponentsTab();
		void CreateAnimsetsTab();
		void CreateCutscenesTab();

		// update tabs
		void UpdateTabs();
		void UpdatePosesTab();
		void UpdateComponentsTab();
		void UpdateAnimsetsTab();
		void UpdateCutscenesTab();

		// dump info
		void DumpAnimsetsInfo(RedGui::CRedGuiEventPackage& eventPackage);

	private:
		RedGui::CRedGuiTab*		m_tabs;

		// Poses
		RedGui::CRedGuiLabel*	m_allocators;
		RedGui::CRedGuiLabel*	m_totalPoses;
		RedGui::CRedGuiLabel*	m_cachedPoses;
		RedGui::CRedGuiLabel*	m_allocPoses;
		RedGui::CRedGuiList*	m_posesList;
		// Poses - logic
		TDynArray< SSkeletonInfo* > m_skeletons;
		Uint32						m_totalPosesMemory;

		// Components
		RedGui::CRedGuiLabel*	m_animatedComponents;
		RedGui::CRedGuiLabel*	m_activeComponents;
		RedGui::CRedGuiLabel*	m_frozenComponents;
		RedGui::CRedGuiLabel*	m_budgetedComponents;
		RedGui::CRedGuiList*	m_componentsList;
		RedGui::CRedGuiList*	m_restList;
		RedGui::CRedGuiList*	m_restTamplates;
		// Components - logic
		SComponentInfo			m_components[CT_Count];

		// Animsets
		RedGui::CRedGuiLabel*	m_animsetCount;
		RedGui::CRedGuiList*	m_animSetList;
		RedGui::CRedGuiButton*	animSetDumpButton;
		// Animsets - logic
		TDynArray< SAnimSetInfo* >	m_animSets;

		// Cutscenes
		RedGui::CRedGuiList*	m_cutsceneResourcesList;
		RedGui::CRedGuiList*	m_cutsceneInstancesList;

		// progress bars in fast view
		RedGui::CRedGuiProgressBar*	m_totalSizeProgressBar;
		RedGui::CRedGuiProgressBar*	m_streamingAnimsProgressBar;
		RedGui::CRedGuiProgressBar*	m_loadedCutscenesProgressBar;
								
		RedGui::CRedGuiProgressBar*	m_animSizeProgressBar;
		RedGui::CRedGuiProgressBar*	m_streamedInAnimsProgressBar;
		RedGui::CRedGuiProgressBar*	m_loadedExDialogsProgressBar;
								
		RedGui::CRedGuiProgressBar*	m_motionExtractionSizeProgressBar;
		RedGui::CRedGuiProgressBar*	m_usedStreamedInAnimsProgressBar;
		RedGui::CRedGuiProgressBar*	m_streamedAnimSizeProgressBar;
								
		RedGui::CRedGuiProgressBar*	m_compresedFramesSizeProgressBar;
		RedGui::CRedGuiProgressBar*	m_activeAnimCountProgressBar;
		RedGui::CRedGuiProgressBar*	m_nonStreamableAnimSizeProgressBar;

		RedGui::CRedGuiProgressBar*	m_totalPosesMemoryProgressBar;
		RedGui::CRedGuiProgressBar*	m_wholeAnimDataMemoryProgressBar;
		RedGui::CRedGuiProgressBar*	m_totalAnimMemoryProgressBar;

		//
		Uint32						m_totalMemory;
		Uint32						m_totalAnimationsMemory;
		Uint32						m_totalAnimationsStreamedMemory;
		Uint32						m_wholeAnimationsMemory; // even not streamed in!
		Uint32						m_totalAnimationsMotionExMemory;
		Uint32						m_totalAnimationsCompressedFrameMemory;
		Uint32						m_totalAnimationsCompressedFrameDataMemory;

		Uint32						m_csSize;
		Uint32						m_exDialogSize;

		Uint32						m_usedAnimationsMemory;
		Uint32						m_numActiveAnimations;

	};

}	// namespace DebugWindows

#endif	// NO_DEBUG_WINDOWS
#endif	// NO_RED_GUI
