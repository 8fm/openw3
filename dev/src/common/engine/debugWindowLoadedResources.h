/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#ifndef NO_RED_GUI
#ifndef NO_DEBUG_WINDOWS

#include "redGuiWindow.h"

class CFXState;

namespace DebugWindows
{
	class CDebugWindowLoadedResources : public RedGui::CRedGuiWindow
	{
		struct ResourceInfo
		{
			THandle< CResource >	m_resource;
			String					m_info;
			TDynArray< String >		m_additionalParams;
			Float					m_size;
			Float					m_partSize;
			Bool					m_active;
			Bool					m_new;

			ResourceInfo( CResource* res, const String& info, Float size, Float partSize )
			{
				m_resource = res;
				m_info = info;
				m_size = size;
				m_partSize = partSize;
				m_active = true;
				m_new = true;
			}

			ResourceInfo( CResource* res, const String& info, Float size )
			{
				m_resource = res;
				m_info = info;
				m_size = size;
				m_partSize = 0.0f;
				m_active = true;
				m_new = true;
			}

			void OnUsed()
			{
				m_active = false;
				m_new = false;
			}
		};

		enum ELoadedResourceType
		{
			LRT_Mesh,
			LRT_Texture,
			LRT_Effect,

			LRT_Count
		};

	public:
		CDebugWindowLoadedResources();
		~CDebugWindowLoadedResources();

	private:
		// overloaded functions
		virtual void OnWindowOpened(RedGui::CRedGuiControl* control);

		// general
		void NotifyEventFileOK( RedGui::CRedGuiEventPackage& eventPackage );
		void NotifyOnTick( RedGui::CRedGuiEventPackage& eventPackage, Float timeDelta );
		void NotifyButtonClicked( RedGui::CRedGuiEventPackage& eventPackage );
		void NotifyEventDoubleClickItem( RedGui::CRedGuiEventPackage& eventPackage, Int32 selectedIndex );

		// mesh tab
		void CreateMeshesTab();
		
		void RefreshMeshesTab();

		void CollectAllMeshes();
		void CollectAllTextures();
		void DumpLoadedMeshes( const String& path );
		void DumpLoadedTextures( const String& path );
		void UpdateMeshesSizeCounters( const String& firendlyName, const Float sizeInMB );
		void UpdateTexturesSizeCounters( const String& textureGroup, const Float sizeInkB );

		void UpdateListContents( RedGui::CRedGuiList* list,  THashMap< String, ResourceInfo >& content, Bool includeAdditionalInfo = false );
		void EreaseNonActive( THashMap< String, ResourceInfo >& resources );

		// texture tab
		void CreateTexturesTab();
		
		void RefreshTexturesTab();
		

		// effect tab
		void CreateEffectsTab();
		void UpdateEffectTab();
		Bool ValidateEffectOptmialization( CFXState* fxState, TDynArray< String >* comments );
		void NotifyOnClickedOnEffect( RedGui::CRedGuiEventPackage& eventPackage, Int32 selectedIndex );
		void ClearEffectInfo();
		void UpdateInfoAboutEffect( CFXState* effect );

	private:
		// general gui
		RedGui::CRedGuiTab*					m_tabs;
		RedGui::CRedGuiSaveFileDialog*		m_saveFileDialog;

		// gui for textures
		RedGui::CRedGuiButton*				m_makeTexturesDumpFile;
		RedGui::CRedGuiList*				m_notLoadedTexturesList;
		RedGui::CRedGuiList*				m_texturesList;
		RedGui::CRedGuiLabel*				m_characterTextures;
		RedGui::CRedGuiLabel*				m_environmentTextures;
		RedGui::CRedGuiLabel*				m_otherTextures;
		RedGui::CRedGuiLabel*				m_allTextures;
		// logic for textures
		THashMap< String, ResourceInfo >	m_corruptedTextures;
		THashMap< String, ResourceInfo >	m_textures;
		Float								m_characterTexturesMB;
		Float								m_environmentTexturesMB;
		Float								m_otherTexturesMB;

		// gui for meshes
		RedGui::CRedGuiButton*				m_makeMeshesDumpFile;
		RedGui::CRedGuiList*				m_notLoadedMeshesList;
		RedGui::CRedGuiList*				m_meshesList;
		RedGui::CRedGuiLabel*				m_characterMeshes;
		RedGui::CRedGuiLabel*				m_environmentMeshes;
		RedGui::CRedGuiLabel*				m_itemMeshes;
		RedGui::CRedGuiLabel*				m_otherMeshes;
		// logic for meshes				
		THashMap< String, ResourceInfo >	m_corruptedMeshes;
		THashMap< String, ResourceInfo >	m_meshes;
		Float								m_characterMeshesMB;
		Float								m_environmentMeshesMB;
		Float								m_itemMeshesMB;
		Float								m_otherMeshesMB;
										
		// gui for effects				
		RedGui::CRedGuiList*				m_effectsList;
		RedGui::CRedGuiLabel*				m_allActiveEffects;
		RedGui::CRedGuiLabel*				m_entityTemplateName;
		RedGui::CRedGuiLabel*				m_entityWorldPosition;
		RedGui::CRedGuiLabel*				m_pathToEffect;
		RedGui::CRedGuiLabel*				m_currentTime;
		RedGui::CRedGuiLabel*				m_effectState;
		RedGui::CRedGuiList*				m_optymizationList;
		RedGui::CRedGuiList*				m_playDataList;
		// logic
		CFXState*							m_selectedEffect;
		Vector								m_camPos;

	};
}

#endif	// NO_DEBUG_WINDOWS
#endif	// NO_RED_GUI
