/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#ifndef NO_RED_GUI
#ifndef NO_DEBUG_WINDOWS

#include "..\engine\redGuiWindow.h"

namespace DebugWindows
{

	class CDebugWindowDynamicTextures : public RedGui::CRedGuiWindow
	{
		enum EChannel
		{
			EChannel_R,
			EChannel_G,
			EChannel_B,
			EChannel_A
		};

		class CDynamicTexturesImage : public RedGui::CRedGuiControl
		{
		public:
			CDynamicTexturesImage(Uint32 x, Uint32 y, Uint32 width, Uint32 height);
			virtual ~CDynamicTexturesImage();

			RedGui::Event1_Package EventChangeState;

			virtual void Draw();
			virtual void OnMouseWheel( Int32 delta ) override;
			virtual void OnMouseButtonClick(const Vector2& mousePosition, enum RedGui::EMouseButton button) override;

			void SetTextureRef( GpuApi::TextureRef textureRef );

			RED_INLINE void SetMaxMips( Int32 value ) { m_maxMips = value; }
			RED_INLINE void SetMaxSlices( Int32 value ) { m_maxSlices = value; EventChangeState( this ); }
			RED_INLINE void SetCurrentMip( Int32 value ) { m_currentMip = value; }
			RED_INLINE void SetMinColorFactor( Float value ) { m_minColorFactor = value; }
			RED_INLINE void SetMaxColorFactor( Float value ) { m_maxColorFactor = value; }

			RED_INLINE Int32 GetCurrentSlice() const { return m_currentSlice; }
			RED_INLINE Int32 GetMaxSlices() const { return m_maxSlices; }

			RED_INLINE Int32 GetCurrentMip() const { return m_currentMip; }
			RED_INLINE Int32 GetMaxMips() const { return m_maxMips; }

			RED_INLINE Float GetColorFactor() const { return m_minColorFactor; }
			RED_INLINE Float GetMinColorFactor() const { return m_minColorFactor; }
			RED_INLINE Float GetMaxColorFactor() const { return m_maxColorFactor; }
			
			void ChangeChannelSelector( EChannel channel, Bool value );
			void Reset();

		private:
			GpuApi::TextureRef	m_textureRef;

			// logic
			Int32		m_currentSlice;
			Int32		m_maxSlices;
			Int32		m_currentMip;
			Int32		m_maxMips;
			Float		m_minColorFactor;
			Float		m_maxColorFactor;
			Vector		m_channelSelector;
		};

	public:
		CDebugWindowDynamicTextures();
		~CDebugWindowDynamicTextures();

	private:
		virtual void OnWindowOpened( CRedGuiControl* control );
		virtual void OnWindowClosed( CRedGuiControl* control );

		void NotifyDynamicTextureChanged( RedGui::CRedGuiEventPackage& eventPackage, Int32 value );
		void NotifyMipChanged( RedGui::CRedGuiEventPackage& eventPackage, Int32 value );
		void NotifyTextureStateChanged( RedGui::CRedGuiEventPackage& eventPackage );
		void NotifySliderChanged( RedGui::CRedGuiEventPackage& eventPackage, Float value );
		void NotifyEventValueChangedMinColorFactor( RedGui::CRedGuiEventPackage& eventPackage, Int32 value );
		void NotifyEventValueChangedMaxColorFactor( RedGui::CRedGuiEventPackage& eventPackage, Int32 value );
		void NotifyEventChannelCheckBoxClicked( RedGui::CRedGuiEventPackage& eventPackage, Bool value );

		void CreateControls();

		String GpiApiFormatToStr( GpuApi::eTextureFormat format );
		String GpiApiTexTypeToStr( GpuApi::eTextureType type );

	private:
		RedGui::CRedGuiComboBox*		m_textureType;
		CDynamicTexturesImage*			m_textureImage;
		RedGui::CRedGuiLabel*			m_formatLabel;
		RedGui::CRedGuiLabel*			m_typeLabel;
		RedGui::CRedGuiLabel*			m_sizeLabel;
		RedGui::CRedGuiLabel*			m_mipLabel;
		RedGui::CRedGuiAdvancedSlider*	m_minColorFactor;
		RedGui::CRedGuiAdvancedSlider*	m_maxColorFactor;
		RedGui::CRedGuiSpin*			m_minColorFactorSpin;
		RedGui::CRedGuiSpin*			m_maxColorFactorSpin;
		RedGui::CRedGuiComboBox*		m_mips;
		RedGui::CRedGuiCheckBox*		m_channelRCheckBox;
		RedGui::CRedGuiCheckBox*		m_channelGCheckBox;
		RedGui::CRedGuiCheckBox*		m_channelBCheckBox;
		RedGui::CRedGuiCheckBox*		m_channelACheckBox;

		THashMap< RedGui::CRedGuiControl*, EChannel > m_controlsToChannelsMap;
	};

}	// namespace DebugWindows

#endif	// NO_DEBUG_WINDOWS
#endif	// NO_RED_GUI
