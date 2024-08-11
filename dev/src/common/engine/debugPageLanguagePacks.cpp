#include "build.h"

#ifndef NO_DEBUG_PAGES

#include "localizationManager.h"
#include "debugPageManagerBase.h"
#include "inputKeys.h"
#include "inputBufferedInputEvent.h"
#include "renderFrame.h"
#include "../core/memoryHelpers.h"

class CDebugPageLanguagePacks : public IDebugPage
{
private:
	LanguagePackDebugSummary m_debugData;
	Int32	m_highligthedPack;
	Int32	m_firstVisiblePack;
	
	String m_requestedTextLanguage;
	String m_requestedSpeechLanguage;
	TDynArray< String > m_availableTextLanguages;
	TDynArray< String > m_availableSpeechLanguages;

public:
	CDebugPageLanguagePacks()
		: IDebugPage( TXT( "Language Packs" ) )
	{
		m_highligthedPack = 0;
		m_firstVisiblePack = 0;
		
		SLocalizationManager::GetInstance().GetAllAvailableLanguages( m_availableTextLanguages, m_availableSpeechLanguages );
		SLocalizationManager::GetInstance().GetGameLanguageName( m_requestedSpeechLanguage, m_requestedTextLanguage );

	}

	virtual Bool OnViewportInput( IViewport* view, enum EInputKey key, enum EInputAction action, Float data ) 
	{
		if ( ( key == IK_Down || key == IK_Pad_DigitDown ) && action == IACT_Press )
		{
			m_highligthedPack = Min( m_highligthedPack + 1, (Int32) m_debugData.m_packsFromCache.Size() );
			return true;
		}

		// Go to next NPC
		if ( ( key == IK_Up || key == IK_Pad_DigitUp ) && action == IACT_Press )
		{
			m_highligthedPack = Max( m_highligthedPack - 1, 0 );
			return true;
		}

		if ( key == IK_PageDown && action == IACT_Press )
		{
			m_highligthedPack =  m_debugData.m_packsFromCache.Size() - 1;
			return true;
		}

		// Go to next NPC
		if ( key == IK_PageUp && action == IACT_Press )
		{
			m_highligthedPack = 0;
			return true;
		}

		// changing languages
		if ( key == IK_Pad_DigitLeft && action == IACT_Press )
		{
			Int32 langIndex = static_cast< Int32 >( m_availableTextLanguages.GetIndex( m_requestedTextLanguage ) );
			if ( langIndex >= 0 )
			{
				if ( langIndex == (Int32) m_availableTextLanguages.Size() - 1 )
				{
					langIndex = 0;
				}
				else
				{
					langIndex += 1;
				}
				m_requestedTextLanguage = m_availableTextLanguages[ langIndex ];
			}
			return true;
		}
		if ( key == IK_Pad_DigitRight && action == IACT_Press )
		{
			Int32 langIndex = static_cast< Int32 >( m_availableSpeechLanguages.GetIndex( m_requestedSpeechLanguage ) );
			if ( langIndex >= 0 )
			{
				if ( langIndex == (Int32) m_availableSpeechLanguages.Size() - 1 )
				{
					langIndex = 0;
				}
				else
				{
					langIndex += 1;
				}
				m_requestedSpeechLanguage = m_availableSpeechLanguages[ langIndex ];
			}

			return true;
		}
		if ( key == IK_Pad_LeftShoulder && action == IACT_Press )
		{
			SLocalizationManager::GetInstance().SwitchGameLanguage( m_requestedSpeechLanguage, m_requestedTextLanguage );
			return true;
		}
		if ( key == IK_Pad_RightShoulder && action == IACT_Press )
		{
			SLocalizationManager::GetInstance().SwitchGameLanguage( SLocalizationManager::GetInstance().GetSpeechLocale(), m_requestedTextLanguage );
			return true;
		}

		return false;
	}

	virtual void OnViewportGenerateFragments( IViewport *view, CRenderFrame *frame ) 
	{
		SLocalizationManager::GetInstance().FillDebugSummary( m_debugData );

		Float maxMemory = m_debugData.m_maxSize / 1024.f;
		Float usedMemory = m_debugData.m_usedSize / 1024.f ;
		Float usedPercentage = usedMemory / maxMemory * 100;
		Uint32 packsCreated = m_debugData.m_createdPacks;
		Uint32 packsCashed = m_debugData.m_packsFromCache.Size();
		Uint32 lipsyncsLocked = m_debugData.m_lockedLipsyncs;

		Uint32 maxTextLength = 40;
		Uint32 topLeftX = 50;
		
		const Uint32 screenWidth = frame->GetFrameOverlayInfo().m_width;
		const Uint32 screenHeight = frame->GetFrameOverlayInfo().m_height;

		Uint32 ySpacing = 15;
		Uint32 listXSpacing = 75;

		Uint32 pageTopLeftX = 50;
		Uint32 pageTopLeftY = 50;
		Uint32 pageWidth = screenWidth - 2 * pageTopLeftX;
		Uint32 pageHeight = screenHeight - pageTopLeftY - 80;

		Uint32 infoTopLeftX = pageTopLeftX + 20;
		Uint32 infoTopLeftY = pageTopLeftY + ySpacing;

		Uint32 listTopLeftX = topLeftX + 400;
		Uint32 listTopLeftY = infoTopLeftY;

		

		

		frame->AddDebugRect( pageTopLeftX, pageTopLeftY, pageWidth, pageHeight, Color( 0, 0, 0, 128 ) );
		frame->AddDebugFrame( pageTopLeftX, pageTopLeftY, pageWidth, pageHeight, Color::WHITE );

		frame->AddDebugScreenFormatedText( infoTopLeftX, infoTopLeftY + 1 * ySpacing, Color::WHITE, TXT( "Max language memory:\t%.2f  [KB]" ), maxMemory );
		frame->AddDebugScreenFormatedText( infoTopLeftX, infoTopLeftY + 2 * ySpacing, 
			( usedPercentage <= 50 ) ? Color::GREEN : Color::YELLOW, TXT( "Used language memory:\t%.2f [KB] (%.1f%%)" ), usedMemory, usedPercentage );

		frame->AddDebugScreenFormatedText( infoTopLeftX, infoTopLeftY + 4 * ySpacing, Color::WHITE, TXT( "Packs created:\t%d" ), packsCreated );
		frame->AddDebugScreenFormatedText( infoTopLeftX, infoTopLeftY + 5 * ySpacing, Color::WHITE, TXT( "Packs in cache:\t%d" ), packsCashed );
		frame->AddDebugScreenFormatedText( infoTopLeftX, infoTopLeftY + 6 * ySpacing, 
			( packsCreated == packsCashed ) ? Color::WHITE : Color::RED, TXT( "Orphaned packs:	%d" ), packsCreated - packsCashed );
		frame->AddDebugScreenFormatedText( infoTopLeftX, infoTopLeftY + 8 * ySpacing, Color::WHITE, TXT( "Lipsyncs locked:\t%d" ), lipsyncsLocked );

		


		frame->AddDebugScreenFormatedText( listTopLeftX - 1 * listXSpacing, listTopLeftY + 0 * ySpacing, Color::YELLOW, TXT( "String ID" ) );
		frame->AddDebugScreenFormatedText( listTopLeftX + 0 * listXSpacing, listTopLeftY + 0 * ySpacing, Color::YELLOW, TXT( "Pack text" ) );
		frame->AddDebugScreenFormatedText( listTopLeftX + 3 * listXSpacing, listTopLeftY + 0 * ySpacing, Color::YELLOW, TXT( "Text size" ) );
		frame->AddDebugScreenFormatedText( listTopLeftX + 4 * listXSpacing, listTopLeftY + 0 * ySpacing, Color::YELLOW, TXT( "VO size" ) );
		frame->AddDebugScreenFormatedText( listTopLeftX + 5 * listXSpacing, listTopLeftY + 0 * ySpacing, Color::YELLOW, TXT( "Lipsync size" ) );
		frame->AddDebugScreenFormatedText( listTopLeftX + 6 * listXSpacing, listTopLeftY + 0 * ySpacing, Color::YELLOW, TXT( "Total size" ) );


		if ( m_debugData.m_packsFromCache.Empty() == true )
		{
			return;
		}

		Int32 maxLines = ( pageHeight / ySpacing ) - 3;

		Int32 numberOfPacks = m_debugData.m_packsFromCache.Size();
		Int32 listLineIndex = 1;
		Int32 actualHighlightedPack = Min( m_highligthedPack, numberOfPacks - 1 );
		

		if ( actualHighlightedPack + 1 - maxLines > m_firstVisiblePack )
		{
			m_firstVisiblePack = actualHighlightedPack + 1 - maxLines;
		}
		else if ( actualHighlightedPack < m_firstVisiblePack )
		{
			m_firstVisiblePack = actualHighlightedPack;
		}

		if ( m_firstVisiblePack > 0 )
		{
			frame->AddDebugScreenFormatedText( listTopLeftX - 1 * listXSpacing, listTopLeftY + listLineIndex * ySpacing, Color::WHITE, TXT( "..." ) );
			frame->AddDebugScreenFormatedText( listTopLeftX + 0 * listXSpacing, listTopLeftY + listLineIndex * ySpacing, Color::WHITE, TXT( "..." ) );
			frame->AddDebugScreenFormatedText( listTopLeftX + 3 * listXSpacing, listTopLeftY + listLineIndex * ySpacing, Color::WHITE, TXT( "..." ) );
			frame->AddDebugScreenFormatedText( listTopLeftX + 4 * listXSpacing, listTopLeftY + listLineIndex * ySpacing, Color::WHITE, TXT( "..." ) );
			frame->AddDebugScreenFormatedText( listTopLeftX + 5 * listXSpacing, listTopLeftY + listLineIndex * ySpacing, Color::WHITE, TXT( "..." ) );
			frame->AddDebugScreenFormatedText( listTopLeftX + 6 * listXSpacing, listTopLeftY + listLineIndex * ySpacing, Color::WHITE, TXT( "..." ) );
			listLineIndex += 1;
		}

		for ( Int32 i = m_firstVisiblePack; i < numberOfPacks && i - m_firstVisiblePack < maxLines; ++i, ++listLineIndex )
		{
			String packText = m_debugData.m_packsFromCache[ i ]->GetText();
			if ( packText.GetLength() > maxTextLength )
			{
				packText = packText.LeftString( maxTextLength );
				packText += TXT( "..." );
			}
			Float packTextSize = m_debugData.m_packsFromCache[ i ]->GetTextSize() / 1024.f;
			Float packVoiceoverSize = m_debugData.m_packsFromCache[ i ]->GetVoiceoverSize() / 1024.f;
			Float packLipsyncSize = m_debugData.m_packsFromCache[ i ]->GetLipsyncSize() / 1024.f;
			Float packTotalSize = m_debugData.m_packsFromCache[ i ]->GetPackSize() / 1024.f;
			
			Uint32 packId = 999999;
			if ( i >= 0 && i < (Int32) m_debugData.m_cacheStringIds.Size() )
			{
				packId = m_debugData.m_cacheStringIds[ i ];
			}

			Color listLineColor = ( i == actualHighlightedPack ) ? Color::GREEN : Color::WHITE;
			Bool packSpeechBufferLoaded = m_debugData.m_packsFromCache[ i ]->GetSpeechBuffer().IsLoaded();

			frame->AddDebugScreenFormatedText( listTopLeftX - 1 * listXSpacing, listTopLeftY + listLineIndex * ySpacing, listLineColor, TXT( "%d" ), packId );
			frame->AddDebugScreenFormatedText( listTopLeftX + 0 * listXSpacing, listTopLeftY + listLineIndex * ySpacing, listLineColor, packText.AsChar() );
			frame->AddDebugScreenFormatedText( listTopLeftX + 3 * listXSpacing, listTopLeftY + listLineIndex * ySpacing, listLineColor, TXT( "%.2f [KB]" ), packTextSize );
			frame->AddDebugScreenFormatedText( listTopLeftX + 4 * listXSpacing, listTopLeftY + listLineIndex * ySpacing, ( packSpeechBufferLoaded == true ) ? listLineColor : Color::RED, TXT( "%.2f [KB]" ), packVoiceoverSize );
			frame->AddDebugScreenFormatedText( listTopLeftX + 5 * listXSpacing, listTopLeftY + listLineIndex * ySpacing, listLineColor, TXT( "%.2f [KB]" ), packLipsyncSize );
			frame->AddDebugScreenFormatedText( listTopLeftX + 6 * listXSpacing, listTopLeftY + listLineIndex * ySpacing, listLineColor, TXT( "%.2f [KB]" ), packTotalSize );
		}

		if ( m_firstVisiblePack + maxLines < numberOfPacks - 1 )
		{
			frame->AddDebugScreenFormatedText( listTopLeftX - 1 * listXSpacing, listTopLeftY + listLineIndex * ySpacing, Color::WHITE, TXT( "..." ) );
			frame->AddDebugScreenFormatedText( listTopLeftX + 0 * listXSpacing, listTopLeftY + listLineIndex * ySpacing, Color::WHITE, TXT( "..." ) );
			frame->AddDebugScreenFormatedText( listTopLeftX + 3 * listXSpacing, listTopLeftY + listLineIndex * ySpacing, Color::WHITE, TXT( "..." ) );
			frame->AddDebugScreenFormatedText( listTopLeftX + 4 * listXSpacing, listTopLeftY + listLineIndex * ySpacing, Color::WHITE, TXT( "..." ) );
			frame->AddDebugScreenFormatedText( listTopLeftX + 5 * listXSpacing, listTopLeftY + listLineIndex * ySpacing, Color::WHITE, TXT( "..." ) );
			frame->AddDebugScreenFormatedText( listTopLeftX + 6 * listXSpacing, listTopLeftY + listLineIndex * ySpacing, Color::WHITE, TXT( "..." ) );
			listLineIndex += 1;
		}


		Float totalTextSize = 0.0f;
		Float totalSpeechSize = 0.0f;
		Float totalLipsyncSize = 0.0f;

		for ( Uint32 i = 0; i < m_debugData.m_cacheStringIds.Size(); ++i )
		{
			totalTextSize += m_debugData.m_packsFromCache[ i ]->GetTextSize() / 1024.f;
			totalSpeechSize += m_debugData.m_packsFromCache[ i ]->GetVoiceoverSize() / 1024.f;
			totalLipsyncSize += m_debugData.m_packsFromCache[ i ]->GetLipsyncSize() / 1024.f;
		}

		frame->AddDebugScreenFormatedText( infoTopLeftX, infoTopLeftY + 10 * ySpacing, Color::WHITE, TXT( "Total text size:\t%.2f [KB]" ), totalTextSize );
		frame->AddDebugScreenFormatedText( infoTopLeftX, infoTopLeftY + 11 * ySpacing, Color::WHITE, TXT( "Total speech size:\t%.2f [KB]" ), totalSpeechSize );
		frame->AddDebugScreenFormatedText( infoTopLeftX, infoTopLeftY + 12 * ySpacing, Color::WHITE, TXT( "Total lipsync size:\t%.2f [KB]" ), totalLipsyncSize );

		Uint32 speechBufferBytes = static_cast< Uint32 > (Memory::GetAllocatedBytesPerMemoryClass( MC_BufferSpeech ) );

		const Float speechBufferUsed = speechBufferBytes / 1024.0f;
		frame->AddDebugScreenFormatedText( infoTopLeftX, infoTopLeftY + 14 * ySpacing, Color::WHITE, TXT( "Speech buffer used:\t%.2f [KB]" ), speechBufferUsed  );


		// Language choice
		
		String currentTextLanguage;
		String currentSpeechLanguage;
		SLocalizationManager::GetInstance().GetGameLanguageName( currentSpeechLanguage, currentTextLanguage );

		frame->AddDebugScreenFormatedText( infoTopLeftX, infoTopLeftY + 16 * ySpacing, Color::WHITE, TXT( "Text" ) );
		for ( Uint32 i = 0; i < m_availableTextLanguages.Size(); ++i )
		{
			frame->AddDebugScreenFormatedText( infoTopLeftX, infoTopLeftY + ( 17 + i ) * ySpacing, 
				( m_availableTextLanguages[ i ] == m_requestedTextLanguage ) ? Color::YELLOW 
				: ( ( m_availableTextLanguages[ i ] == currentTextLanguage ) ? Color::GREEN : Color::WHITE ), 
				m_availableTextLanguages[ i ].AsChar() );
		}

		frame->AddDebugScreenFormatedText( infoTopLeftX + listXSpacing, infoTopLeftY + 16 * ySpacing, Color::WHITE, TXT( "Speech" ) );
		for ( Uint32 i = 0; i < m_availableSpeechLanguages.Size(); ++i )
		{
			frame->AddDebugScreenFormatedText( infoTopLeftX + listXSpacing, infoTopLeftY + ( 17 + i ) * ySpacing, 
				( m_availableSpeechLanguages[ i ] == m_requestedSpeechLanguage ) ? Color::YELLOW 
				: ( ( m_availableSpeechLanguages[ i ] == currentSpeechLanguage ) ? Color::GREEN : Color::WHITE ), 
				m_availableSpeechLanguages[ i ].AsChar() );
		}

	}

	virtual void OnTick( Float timeDelta ) 
	{
		
	}

};


void CreateDebugPageLanguagePacks()
{
	IDebugPage* page = new CDebugPageLanguagePacks();
	IDebugPageManagerBase::GetInstance()->RegisterDebugPage( page );
}
#endif
