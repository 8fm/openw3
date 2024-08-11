/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "dialogEditor.h"
#include "dialogEditorLocCtrl.h"
#include "storyScenePreviewPlayer.h"
#include "voice.h"
#include "popupNotification.h"

#include "../../common/game/storySceneEvent.h"
#include "../../common/game/storySceneEventOverridePlacement.h"
#include "../../common/game/storySceneEventScenePropPlacement.h"
#include "../../common/game/storySceneEventWorldPropPlacement.h"
#include "../../common/game/storySceneEventEnhancedCameraBlend.h"

#include "../../common/game/storySceneLine.h"

#include "../../common/core/depot.h"
#include "../../common/core/feedback.h"
#include "../../common/engine/localizationManager.h"
#include "../../common/game/commonGame.h"
#include "../../common/game/gameWorld.h"
#include "../../common/game/storySceneUtils.h"

#ifdef DEBUG_SCENES_2
#pragma optimize("",off)
#endif

//////////////////////////////////////////////////////////////////////////

CEdSceneLocalizationCtrl::CEdSceneLocalizationCtrl()
	: m_section( nullptr )
	, m_enabled( false )
	, m_window( nullptr )
	, m_percentThr1( 0.2f )
	, m_percentThr2( 0.5f )
{

}

CEdSceneLocalizationCtrl::~CEdSceneLocalizationCtrl()
{
	ReleaseAll();
}

void CEdSceneLocalizationCtrl::Enable( Bool flag )
{
	m_enabled = flag;
}

Bool CEdSceneLocalizationCtrl::IsEnabled() const
{
	return m_enabled;
}

void CEdSceneLocalizationCtrl::BindToWindow( CEdSceneEditor* ed, wxPanel* window )
{
	SCENE_ASSERT( !m_window );
	m_window = window;

	wxHtmlWindow* html = XRCCTRL( *m_window, "locHtml", wxHtmlWindow );
	html->Connect( wxEVT_COMMAND_HTML_LINK_CLICKED, wxHtmlLinkEventHandler( CEdSceneEditor::OnToolLocLinkClicked ), NULL, ed );
	html->Connect( wxEVT_COMMAND_HTML_CELL_HOVER, wxHtmlCellEventHandler( CEdSceneEditor::OnToolLocLinkHover ), NULL, ed );
}

Bool CEdSceneLocalizationCtrl::IsBindedToWindow() const
{
	return m_window != nullptr;
}

void CEdSceneLocalizationCtrl::ParseScene( const CStoryScene* scene, IFeedbackSystem* f )
{
	for ( Uint32 i=0; i<scene->GetNumberOfSections(); ++i )
	{
		const CStorySceneSection* section = scene->GetSection( i );
		ParseSection( section, f );
	}
}

Bool CEdSceneLocalizationCtrl::ParseSection( const CStorySceneSection* section, IFeedbackSystem* f )
{
	if ( m_section != section )
	{
		m_section = section;

		if ( m_section )
		{
			DoParse( f );

			return true;
		}
	}

	return false;
}

void CEdSceneLocalizationCtrl::ReleaseAll()
{

}

Bool CEdSceneLocalizationCtrl::HasLocDataForSection( const String& sectionName ) const
{
	const Uint32 num = m_sectionsData.Size();
	for ( Uint32 i=0; i<num; ++i )
	{
		if ( m_sectionsData[ i ].m_name == sectionName )
		{
			return true;
		}
	}

	return false;
}

Bool CEdSceneLocalizationCtrl::HasLocData( Uint32 stringId ) const
{
	const Uint32 numData = m_sectionLinesData.Size();
	for ( Uint32 i=0; i<numData; ++i )
	{
		if ( m_sectionLinesData[ i ].m_id == stringId )
		{
			return true;
		}
	}

	return false;
}

Bool CEdSceneLocalizationCtrl::HasLocForLang( const String& lang ) const
{
	const Uint32 numData = m_sectionLinesData.Size();
	for ( Uint32 i=0; i<numData; ++i )
	{
		for ( Uint32 j=0; j<m_sectionLinesData[ i ].m_data.Size(); ++j )
		{
			if ( m_sectionLinesData[ i ].m_data[ j ].m_lang == lang )
			{
				if ( m_sectionLinesData[ i ].m_data[ j ].m_duration > 0.f )
				{
					return true;
				}

				break;
			}
		}
	}

	return false;
}

Uint32 CEdSceneLocalizationCtrl::GetLangId( const String& lang ) const
{
	SCENE_ASSERT( m_langs.Size() > 0 );

	for ( Uint32 i=0; i<m_langs.Size(); ++i )
	{
		if ( m_langs[ i ] == lang )
		{
			return i;
		}
	}

	SCENE_ASSERT( 0 );
	return 0;
}

Uint32 CEdSceneLocalizationCtrl::GetCurrLangId() const
{
	return GetLangId( SLocalizationManager::GetInstance().GetCurrentLocale() );
}

Uint32 CEdSceneLocalizationCtrl::GetRefLangId() const
{
	const String& langRef = GCommonGame->GetSystem< CStorySceneSystem >()->GetLangRefName();
	return GetLangId( langRef );
}

void CEdSceneLocalizationCtrl::DoParse( IFeedbackSystem* f )
{
	if ( f )
	{
		f->BeginTask( TXT("Localization"), false );
	}

	SCENE_ASSERT( m_section );

	TDynArray< String > tLangs;
	SLocalizationManager::GetInstance().GetAllAvailableLanguages( tLangs, m_langs );
	const Uint32 numLangs = m_langs.Size();
	m_langsFlag.Resize( numLangs );

	Int32 firstLineIndex = -1;
	Int32 lastLineIndex = -1;

	const TDynArray< CStorySceneElement* >& elements = m_section->GetElements();

	const Uint32 numElements = elements.Size();
	for ( Uint32 i=0; i<numElements; ++i )
	{
		if ( f )
		{
			f->UpdateTaskProgress( i, numElements );
		}

		const CStorySceneElement* elem = elements[ i ];
		SCENE_ASSERT( elem );

		if ( const CStorySceneLine* line = Cast< const CStorySceneLine >( elem ) )
		{
			const Uint32 lineStringId = line->GetLocalizedContent()->GetIndex();
			const String& lineVO = line->GetVoiceFileNameRef();

			if ( HasLocData( lineStringId ) )
			{
				continue;
			}

			if ( f )
			{
				f->UpdateTaskInfo( TXT("Line - %s"), lineVO.AsChar() );
			}

			LocLineData record;
			record.m_id = lineStringId;
			record.m_stringId = line->GetVoiceFileNameRef();
			record.m_data.Resize( numLangs );
			record.m_section = m_section;
			record.m_line = line;

			for ( Uint32 j=0; j<numLangs; ++j )
			{
				const String& lang = m_langs[ j ];

				const LanguagePack* pack = SLocalizationManager::GetInstance().GetLanguagePackSync( lineStringId, false, lang );

				LocLineDataElem dataRecord;
				dataRecord.m_lang = lang;
				dataRecord.m_duration = pack ? pack->GetSpeechBuffer().GetDuration() : 0.f;
				dataRecord.m_text = pack ? pack->GetText() : TXT("<None>");
				dataRecord.m_fakeDuration = pack ? line->CalcFakeDuration( dataRecord.m_text ) : 0.f;

				record.m_data[ j ] = dataRecord;

				//SLocalizationManager::GetInstance().ReleaseLanguagePack( lineStringId );
				//dataRecord.m_pack = nullptr;
			}

			{
				const Uint32 refId = GetRefLangId();
				const Float refDuration = record.m_data[ refId ].m_duration;

				Float maxDiff = 0.f;
				Uint32 maxId = 0;

				for ( Uint32 j=0; j<numLangs; ++j )
				{
					if ( record.m_data[ j ].m_duration > 0.f )
					{
						record.m_data[ j ].m_percent = refDuration / record.m_data[ j ].m_duration;

						const Float diff = MAbs( refDuration - record.m_data[ j ].m_duration );
						if ( diff > maxDiff )
						{
							maxDiff = diff;
							maxId = j;
						}
					}
				}

				record.m_maxDiffId = maxId;
				record.m_maxDiffValTime = maxDiff;
				record.m_maxValPercent = record.m_data[ maxId ].m_duration / refDuration;
			}

			if ( firstLineIndex == -1 )
			{
				firstLineIndex = m_sectionLinesData.SizeInt();
			}

			m_sectionLinesData.PushBack( record );

			lastLineIndex = m_sectionLinesData.SizeInt();
		}
	}

	for ( Uint32 i=0; i<numLangs; ++i )
	{
		m_langsFlag[ i ] = HasLocForLang( m_langs[ i ] );
	}

	if ( firstLineIndex != -1 && lastLineIndex != -1 && !HasLocDataForSection( m_section->GetName() ) )
	{
		const Uint32 refId = GetRefLangId();

		LocSectionData data;
		data.m_name = m_section->GetName();
		data.m_sectionRefDuration = 0.f;

		TDynArray< Float > times( numLangs );
		for ( Uint32 i=0; i<numLangs; ++i )
		{
			times[ i ] = 0.f;
		}
		TDynArray< Float > times2( times );
		TDynArray< Float > times_b( times );
		TDynArray< Float > times2_b( times );

		for ( Int32 i=firstLineIndex; i<lastLineIndex; ++i )
		{
			const LocLineData& lineData = m_sectionLinesData[ i ];

			const Float refDuration = lineData.m_data[ refId ].m_duration;
			data.m_sectionRefDuration += refDuration;

			for ( Uint32 j=0; j<numLangs; ++j )
			{
				const Float currDuration = lineData.m_data[ j ].m_duration;

				if ( currDuration > 0.f )
				{
					times[ j ] += currDuration;
					times2[ j ] += currDuration > refDuration ? currDuration : refDuration;
				}
				else
				{
					times[ j ] += refDuration;
					times2[ j ] += refDuration;
				}

				if ( currDuration > 0.f )
				{
					times_b[ j ] += currDuration;
					times2_b[ j ] += currDuration > refDuration ? currDuration : refDuration;
				}
				else
				{
					const Float fakeDuration = lineData.m_data[ j ].m_fakeDuration;

					times_b[ j ] += fakeDuration;
					times2_b[ j ] += fakeDuration > refDuration ? fakeDuration : refDuration;
				}
			}
		}

		Uint32 bestIdx = 0;
		for ( Uint32 i=bestIdx+1; i<numLangs; ++i )
		{
			if ( times2_b[ i ] == 0.f )
			{
				continue;
			}

			SCENE_ASSERT( times2[ i ] >= data.m_sectionRefDuration );
			if ( m_langsFlag[ i ] && MAbs( times2_b[ i ] - data.m_sectionRefDuration ) > MAbs( times2_b[ bestIdx ] - data.m_sectionRefDuration ) )
			{
				bestIdx = i;
			}
		}

		const Float bestDuration = times2_b[ bestIdx ];
		data.m_sectionMaxDiffSec = bestDuration - data.m_sectionRefDuration;
		data.m_sectionMaxDiffName = m_langs[ bestIdx ];
		data.m_sectionMaxDiffPercent = bestDuration / data.m_sectionRefDuration;
		data.m_sectionMaxDuration = bestDuration;
		data.m_sectionMaxDurationAll = times_b[ bestIdx ];
		data.m_sectionMaxDuration_loc = times2[ bestIdx ];
		data.m_sectionMaxDurationAll_loc = times[ bestIdx ];

		m_sectionsData.PushBack( data );
	}

	if ( f )
	{
		f->EndTask();
	}
}

void CEdSceneLocalizationCtrl::RefreshWindow()
{
	SCENE_ASSERT( m_window );

	if ( m_window )
	{
		const Uint32 numLangs = m_langs.Size();
		const Uint32 refId = GetRefLangId();

		wxHtmlWindow* html = XRCCTRL( *m_window, "locHtml", wxHtmlWindow );
		SCENE_ASSERT( html );

		wxString code;

		code += wxT("<table border=1>");
		code += wxT("<tr>");
		code += wxT("<th width=100 align=left>Section</th>");
		code += wxT("<th width=100 align=center>Reference</th>");
		code += wxT("<th width=100 align=center>Max sec diff</th>");
		code += wxT("<th width=100 align=center>Max percent diff</th>");
		code += wxT("<th width=100 align=center>Max diff lang</th>");
		code += wxT("<th width=100 align=center>Max sec</th>");
		code += wxT("<th width=100 align=center>Max sec raw</th>");
		code += wxT("<th width=100 align=center>Max sec loc</th>");
		code += wxT("<th width=100 align=center>Max sec loc raw</th>");
		const Uint32 numSectionsData = m_sectionsData.Size();
		for ( Uint32 i=0; i<numSectionsData; ++i )
		{
			const LocSectionData& locData = m_sectionsData[ i ];

			code += wxT("<tr>");

			String colorCode;

			if ( MAbs( 1.f - locData.m_sectionMaxDiffPercent ) > m_percentThr2 )
			{
				colorCode = TXT("color=\"#FF0000\"");
			}
			else if ( MAbs( 1.f - locData.m_sectionMaxDiffPercent ) > m_percentThr1 )
			{
				colorCode = TXT("color=\"#FF6600\"");
			}

			code += wxString::Format( wxT("<th width=100 align=center><font %s size=\"3\">%s</font></th>"), colorCode.AsChar(), locData.m_name.AsChar() );
			code += wxString::Format( wxT("<th width=100 align=center><font %s size=\"3\">%1.2f sec</font></th>"), colorCode.AsChar(), locData.m_sectionRefDuration );
			code += wxString::Format( wxT("<th width=100 align=center><font %s size=\"3\">%1.2f sec</font></th>"), colorCode.AsChar(), locData.m_sectionMaxDiffSec );
			code += wxString::Format( wxT("<th width=100 align=center><font %s size=\"3\">%1.2f</font></th>"), colorCode.AsChar(), locData.m_sectionMaxDiffPercent );
			code += wxString::Format( wxT("<th width=100 align=center><font %s size=\"3\">%s</font></th>"), colorCode.AsChar(), locData.m_sectionMaxDiffName.AsChar() );	
			code += wxString::Format( wxT("<th width=100 align=center><font %s size=\"3\">%1.2f sec (%1.2f)</font></th>"), colorCode.AsChar(), locData.m_sectionMaxDuration, locData.m_sectionMaxDuration - locData.m_sectionRefDuration );
			code += wxString::Format( wxT("<th width=100 align=center><font %s size=\"3\">%1.2f sec (%1.2f)</font></th>"), colorCode.AsChar(), locData.m_sectionMaxDurationAll, locData.m_sectionMaxDurationAll - locData.m_sectionRefDuration );
			code += wxString::Format( wxT("<th width=100 align=center><font %s size=\"3\">%1.2f sec (%1.2f)</font></th>"), colorCode.AsChar(), locData.m_sectionMaxDuration_loc, locData.m_sectionMaxDuration_loc - locData.m_sectionRefDuration );
			code += wxString::Format( wxT("<th width=100 align=center><font %s size=\"3\">%1.2f sec (%1.2f)</font></th>"), colorCode.AsChar(), locData.m_sectionMaxDurationAll_loc, locData.m_sectionMaxDurationAll_loc - locData.m_sectionRefDuration );

			code += wxT("</tr>");
		}
		code += wxT("</table>");
		code += wxT("<br><br>");

		code += wxT("<table border=1>");

		// Main row
		code += wxT("<tr>");
		code += wxT("<th width=200 align=left>Line</th>");
		code += wxT("<th width=100 align=center>Max time diff</th>");
		code += wxT("<th width=100 align=center>Max percent diff</th>");
		code += wxT("<th width=100 align=center>Max diff lang</th>");
		for ( Uint32 l=0; l<numLangs; ++l )
		{
			if ( !m_langsFlag[ l ] )
			{
				continue;
			}

			const String& lang = m_langs[ l ];
			code += wxString::Format( wxT("<th width=60 align=center>[%s]</th>"), lang.AsChar() );
		}
		code += wxT("</tr>");

		// Data rows
		const Uint32 numLineData = m_sectionLinesData.Size();
		for ( Uint32 i=0; i<numLineData; ++i )
		{
			const LocLineData& locData = m_sectionLinesData[ i ];

			code += wxT("<tr>");

			String colorCode;

			if ( MAbs( 1.f - locData.m_maxValPercent ) > m_percentThr2 )
			{
				colorCode = TXT("color=\"#FF0000\"");
			}
			else if ( MAbs( 1.f - locData.m_maxValPercent ) > m_percentThr1 )
			{
				colorCode = TXT("color=\"#FF6600\"");
			}

			code += wxString::Format( wxT("<th width=200 align=left><font %s size=\"2\">%s</font></th>"), colorCode.AsChar(), locData.m_data[ refId ].m_text.AsChar() );
			code += wxString::Format( wxT("<th width=100 align=center><font %s size=\"3\">%1.2f sec</font></th>"), colorCode.AsChar(), locData.m_maxDiffValTime );
			code += wxString::Format( wxT("<th width=100 align=center><font %s size=\"3\">%1.2f</font></th>"), colorCode.AsChar(), locData.m_maxValPercent );
			code += wxString::Format( wxT("<th width=100 align=center><font %s size=\"3\">%s</font></th>"), colorCode.AsChar(), locData.m_data[ locData.m_maxDiffId ].m_lang.AsChar() );

			for ( Uint32 l=0; l<numLangs; ++l )
			{
				if ( !m_langsFlag[ l ] )
				{
					continue;
				}

				const String& lang = m_langs[ l ];
				const String& langId = locData.m_data[ l ].m_lang;
				const String& text = locData.m_data[ l ].m_text;
				const Float persent = locData.m_data[ l ].m_percent;
				const Float duration = locData.m_data[ l ].m_duration;

				SCENE_ASSERT( lang == langId );

				code += wxString::Format( wxT("<th width=60 align=center><font %s size=\"3\">%1.2f (%1.2f)</font><font %s size=\"1\"><br><a href=\"Play-%d-%d\">play</a>,<a href=\"Text-%d-%d\">text</a></font></th>"), colorCode.AsChar(), persent*100.f, duration, colorCode.AsChar(), i, l, i, l );
			}

			code += wxT("</tr>");
		}

		code += wxT("</table>");

		html->SetPage( code );
	}
}

namespace
{
	String GetVOPath( const String& stringId, const String& languageId, const String& extension )
	{
		String outPath;
		GDepot->GetAbsolutePath( outPath );
		outPath += TXT("speech\\");
		outPath += languageId + TXT("\\audio\\") + stringId + extension;
		return outPath;
	}
}

void CEdSceneLocalizationCtrl::OnLinkClicked( wxHtmlLinkEvent& event )
{
	const wxHtmlLinkInfo& linkInfo = event.GetLinkInfo();
	String href = linkInfo.GetHref().wc_str();

	if ( href.BeginsWith( TXT("Play") ) )
	{
		TDynArray< String > arr = href.Split( TXT("-") );
		SCENE_ASSERT( arr.Size() == 3 );

		Int32 i = 0;
		Int32 l = 0;

		FromString( arr[ 1 ], i );
		FromString( arr[ 2 ], l );

		const String& lang = m_sectionLinesData[ i ].m_data[ l ].m_lang;
		const String path = GetVOPath( m_sectionLinesData[ i ].m_stringId, lang, TXT(".wav") );

		Voice::PlayDebugSound( path );
	}
}

void CEdSceneLocalizationCtrl::OnLinkHover( wxHtmlCellEvent& event )
{
	const wxHtmlLinkInfo* linkInfo = event.GetCell()->GetLink();
	if ( !linkInfo )
	{
		return;
	}

	String href = linkInfo->GetHref().wc_str();

	if ( href.BeginsWith( TXT("Text") ) )
	{
		TDynArray< String > arr = href.Split( TXT("-") );
		SCENE_ASSERT( arr.Size() == 3 );

		Int32 i = 0;
		Int32 l = 0;

		FromString( arr[ 1 ], i );
		FromString( arr[ 2 ], l );

		wxTextCtrl* txt = XRCCTRL( *m_window, "locMsg", wxTextCtrl );
		txt->SetLabelText( m_sectionLinesData[ i ].m_data[ l ].m_text.AsChar() );
	}
}

wxColor CEdSceneLocalizationCtrl::FindColorForLocString( Uint32 stringId ) const
{
	wxColor normal( 126, 209, 124 );
	wxColor hilighted1( 255, 200, 0 );
	wxColor hilighted2( 255, 0, 0 );

	const Uint32 numData = m_sectionLinesData.Size();
	for ( Uint32 i=0; i<numData; ++i )
	{
		const LocLineData& locData = m_sectionLinesData[ i ];
		if ( locData.m_id == stringId )
		{
			if ( MAbs( 1.f - locData.m_maxValPercent ) > m_percentThr2 )
			{
				return hilighted2;
			}
			else if ( MAbs( 1.f - locData.m_maxValPercent ) > m_percentThr1 )
			{
				return hilighted1;
			}

			return normal;
		}
	}

	SCENE_ASSERT( 0 );

	return normal;
}

void CEdSceneLocalizationCtrl::CollectUsedLangs( TDynArray< String >& langs ) const
{
	const Uint32 num = m_langs.Size();
	for ( Uint32 i=0; i<num; ++i )
	{
		if ( HasLocForLang( m_langs[ i ] ) )
		{
			langs.PushBack( m_langs[ i ] );
		}
	}
}

//////////////////////////////////////////////////////////////////////////

CEdSceneScreenshotCtrl::CEdSceneScreenshotCtrl()
	: m_enabled( false )
	, m_window( nullptr )
{

}

CEdSceneScreenshotCtrl::~CEdSceneScreenshotCtrl()
{
	
}

void CEdSceneScreenshotCtrl::Enable( Bool flag )
{
	m_enabled = flag;

	SCENE_ASSERT( m_mediator );
	const Bool ret = m_mediator->OnScreenshotPanel_ForceMainWorld();
	if ( !ret )
	{
		m_enabled = false;
	}

	SCENE_ASSERT( m_window );
	if ( m_window )
	{
		m_window->Enable( m_enabled );
	}
}

Bool CEdSceneScreenshotCtrl::IsEnabled() const
{
	return m_enabled;
}

void CEdSceneScreenshotCtrl::BindToWindow( CEdSceneEditor* ed, wxPanel* window )
{
	SCENE_ASSERT( !m_mediator );
	m_mediator = ed;

	SCENE_ASSERT( !m_window );
	m_window = window;

	wxToggleButton* scBtnRec = XRCCTRL( *m_window, "scBtnRec", wxToggleButton );
	scBtnRec->Connect( wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, wxCommandEventHandler( CEdSceneScreenshotCtrl::OnRec ), NULL, this );

	wxButton* scBtnPrev = XRCCTRL( *m_window, "scBtnPrev", wxButton );
	scBtnPrev->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdSceneScreenshotCtrl::OnPrevFrame ), NULL, this );

	wxButton* scBtnNext = XRCCTRL( *m_window, "scBtnNext", wxButton );
	scBtnNext->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdSceneScreenshotCtrl::OnNextFrame ), NULL, this );

	wxButton* scBtnAddTag = XRCCTRL( *m_window, "scBtnAddTag", wxButton );
	scBtnAddTag->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdSceneScreenshotCtrl::OnAddTag ), NULL, this );

	wxButton* scBtnAddRange = XRCCTRL( *m_window, "scBtnAddRange", wxButton );
	scBtnAddRange->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdSceneScreenshotCtrl::OnAddRange ), NULL, this );

	m_textFrame = XRCCTRL( *m_window, "scTextFrame", wxStaticText );
	m_textTag = XRCCTRL( *m_window, "scTextTag", wxTextCtrl );
	m_textRange = XRCCTRL( *m_window, "scTextRange", wxTextCtrl );

	m_actorList = XRCCTRL( *m_window, "scActorList", wxCheckListBox );
	m_actorList->Connect( wxEVT_COMMAND_CHECKLISTBOX_TOGGLED, wxCommandEventHandler( CEdSceneScreenshotCtrl::OnActorSelected ), NULL, this );
	//for (int i = 0; i < m_actorList->GetCount(); ++i) m_actorList->GetItem(i)->SetBackgroundColour(*wxWHITE);
}

Bool CEdSceneScreenshotCtrl::IsBindedToWindow() const
{
	return m_window != nullptr;
}

void CEdSceneScreenshotCtrl::AddExtraActorsToScene( TDynArray< THandle< CEntity > >& actors, TDynArray< THandle< CEntity > >& props ) const
{
	actors.PushBack( m_entities );
}

void CEdSceneScreenshotCtrl::AddEntity( CEntity* e )
{
	Bool found = false;

	for ( Uint32 i=0; i<m_entities.Size(); ++i )
	{
		const THandle< CEntity >& eH = m_entities[ i ];
		if ( const CEntity* eHPtr = eH.Get() )
		{
			if ( eHPtr == e )
			{
				found = true;
				break;
			}
		}
	}

	if ( !found )
	{
		m_entities.PushBack( e );

		OnAddEntityToScene( e );
	}
}

void CEdSceneScreenshotCtrl::RemoveEntity( const CEntity* e )
{
	// TODO
}

namespace
{
	Uint32 FindNextSlotNum( CStorySceneDialogsetInstance* dialogset )
	{
		Uint32 n = 0;

		for ( CStorySceneDialogsetSlot* s : dialogset->GetSlots() )
		{
			n = Max< Uint32 >( n, s->GetSlotNumber() );
		}

		return n+1;
	}
}

void CEdSceneScreenshotCtrl::OnAddEntityToScene( CEntity* e )
{
	if ( CStoryScene* s = m_mediator->OnScreenshotPanel_GetScene() )
	{
		if ( CStorySceneDialogsetInstance* dialogset = m_mediator->OnScreenshotPanel_GetCurrentDialogset() )
		{
			if ( CActor* a = Cast< CActor >( e ) )
			{
				CName voicetag = a->GetVoiceTag();
				CEntityTemplate* templ = a->GetEntityTemplate();
				if ( voicetag && templ )
				{
					CStorySceneActor* actorSetting = CreateObject< CStorySceneActor >( s );
					actorSetting->m_entityTemplate = templ;
					actorSetting->m_actorTags = a->GetTags();
					actorSetting->m_id = voicetag;
					s->AddActorDefinition( actorSetting );

					const EngineTransform dialogTrans = m_mediator->OnScreenshotPanel_GetCurrentDialogsetTransform();
					EngineTransform actorTrans( a->GetLocalToWorld() );
					EngineTransform slotTrans = StorySceneUtils::CalcSSFromWS( actorTrans, dialogTrans );

					CStorySceneDialogsetSlot* slot = ::CreateObject< CStorySceneDialogsetSlot >( dialogset );
					slot->OnCreatedInEditor();
					slot->SetSlotNumber( FindNextSlotNum( dialogset ) );
					slot->SetSlotName( CName( String::Printf( TXT("Slot %d"), slot->GetSlotNumber() ) ) );
					slot->SetActorName( voicetag );
					slot->SetSlotPlacement( slotTrans );

					dialogset->AddSlot( slot );

					m_mediator->OnScreenshotPanel_AddEntityToScene();
				}
			}
		}
	}
}

void CEdSceneScreenshotCtrl::AddEntityToActorList( CEntity* e )
{
	String nameStr;
	
	if ( CActor* a = Cast< CActor >( e ) )
	{
		if ( a->GetVoiceTag() )
		{
			nameStr += TXT("<");
			nameStr += a->GetVoiceTag().AsString();
			nameStr += TXT(">");
		}
	}

	if ( nameStr.Empty() )
	{
		nameStr += TXT("<> ");
		nameStr += e->GetFriendlyName();
	}

	if ( !nameStr.Empty() )
	{
		Int32 index = m_actorList->Append( nameStr.AsChar(), new wxDialogScreenshotActorClientData( e ) );
	}
}

void CEdSceneScreenshotCtrl::OnRec( wxCommandEvent& event )
{

}

void CEdSceneScreenshotCtrl::OnPrevFrame( wxCommandEvent& event )
{

}

void CEdSceneScreenshotCtrl::OnNextFrame( wxCommandEvent& event )
{

}

void CEdSceneScreenshotCtrl::OnAddTag( wxCommandEvent& event )
{
	String tagStr = m_textTag->GetValue().wc_str();
	CName tag( tagStr );

	SCENE_ASSERT( m_mediator );
	if ( CWorld* w = m_mediator->OnScreenshotPanel_GetWorld() )
	{
		if ( w == GCommonGame->GetActiveWorld() )
		{
			if ( CEntity* e = w->GetTagManager()->GetTaggedEntity( tag ) )
			{
				AddEntityToActorList( e );
			}
		}
	}
}

void CEdSceneScreenshotCtrl::OnAddRange( wxCommandEvent& event )
{
	Float radius = 10.f;
	String tagStr = m_textRange->GetValue().wc_str();
	FromString( tagStr, radius );

	SCENE_ASSERT( m_mediator );
	if ( CWorld* w = m_mediator->OnScreenshotPanel_GetWorld() )
	{
		if ( w == GCommonGame->GetActiveWorld() && GCommonGame->GetPlayer() )
		{
			TDynArray< TPointerWrapper< CGameplayEntity > > nearbyEntities;
			Box nearbyAreaBoundingBox = Box( Vector::ZERO_3D_POINT, radius );
			GCommonGame->GetGameplayStorage()->GetClosestToEntity( *(GCommonGame->GetPlayer()), nearbyEntities, nearbyAreaBoundingBox, 1000, NULL, 0 );

			for ( Uint32 i = 0; i < nearbyEntities.Size(); i++ )
			{
				CGameplayEntity* gameplayEntity = Cast< CGameplayEntity >( nearbyEntities[ i ].Get() );
				if ( gameplayEntity )
				{
					AddEntityToActorList( gameplayEntity );
				}
			}
		}
	}
}

void CEdSceneScreenshotCtrl::OnActorSelected( wxCommandEvent& event )
{
	const Int32 index = event.GetInt();
	const Bool isChecked = m_actorList->IsChecked( index );

	wxDialogScreenshotActorClientData* data = static_cast< wxDialogScreenshotActorClientData* >( m_actorList->GetClientObject( index ) );
	const THandle< CEntity >& entH = data->m_data;
	if ( CEntity* e = entH.Get() )
	{
		if ( isChecked )
		{
			AddEntity( e );
		}
		else
		{
			RemoveEntity( e );
		}
	}
}

#ifdef DEBUG_SCENES_2
#pragma optimize("",on)
#endif
