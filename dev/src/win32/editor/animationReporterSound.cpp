
#include "build.h"
#include "animationReporter.h"
#include "animationReporterSound.h"
#include "../../common/game/extAnimSoundEvent.h"
#include "../../common/core/depot.h"
#include "../../common/core/feedback.h"

void CEdAnimationReporterWindow::RefreshSoundsPage()
{
	RefreshSoundTab( m_depotSoundEvents, wxT("htmlSoundDepot") );
	RefreshSoundTab( m_usedSoundEvents, wxT("htmlSoundUsed") );
	RefreshSoundTab( m_unusedSoundEvents, wxT("htmlSoundUnused") );
	RefreshSoundTab( m_tocheckSoundEvents, wxT("htmlSoundTocheck") );
	RefreshSoundTab( m_allSoundEvents, wxT("htmlSoundAll") );

	RefreshSoundTree( m_depotSoundEvents, wxT("treeSoundDepot") );
	RefreshSoundTree( m_usedSoundEvents, wxT("treeSoundUsed") );
	RefreshSoundTree( m_unusedSoundEvents, wxT("treeSoundUnused") );
	RefreshSoundTree( m_tocheckSoundEvents, wxT("treeSoundTocheck") );
	RefreshSoundTree( m_allSoundEvents, wxT("treeSoundAll") );
}

void CEdAnimationReporterWindow::RefreshSoundTab( const TDynArray< SAnimReportSound* >& list, const wxString& htmlName )
{
	wxString html;

	html += wxT("<table border=1>");

	wxString s1 = m_soundShowDep ? wxT("<a href=\"ShowSingleDep\">Single animation</a>") : wxT("<a href=\"ShowAllDep\">All dependences</a>");

	html += wxT("<tr>");
	html += wxString::Format( wxT("<th align=left><i>Nr</i></th>") );
	html += wxString::Format( wxT("<th align=left><i>Event name</i></th>") );
	html += wxString::Format( wxT("<th align=left><i>%s</i></th>"), s1.c_str() );
	html += wxT("</tr>");

	for ( Uint32 i=0; i<list.Size(); ++i )
	{
		const SAnimReportSound* sound = list[ i ];

		html += wxT("<tr>");

		html += wxString::Format( wxT("<td align=left>%d</td>"), i );
		html += wxString::Format( wxT("<td align=left>%s</td>"), sound->m_soundEventName.AsChar() );

		if ( m_soundShowDep )
		{
			html += wxT("<td align=left>");
			html += wxT("<table>");

			for ( Uint32 j=0; j<sound->m_dependences.Size(); ++j )
			{
				html += wxT("<tr>");
				html += wxString::Format( wxT("<th>%d.</th>"), j );
				html += wxString::Format( wxT("<th align=left>%s</th>"), sound->m_dependences[ j ].m_animation.AsString().AsChar() );
				html += wxString::Format( wxT("<th align=left>%s</th>"), sound->m_dependences[ j ].m_animset.AsChar() );
				html += wxString::Format( wxT("<th align=left>%d</th>"), sound->m_dependences[ j ].m_localNum );
				html += wxString::Format( wxT("<th align=left>%d</th>"), sound->m_dependences[ j ].m_externalNum );
				html += wxT("</tr>");
			}

			html += wxT("</table>");
			html += wxT("</td>");
		}
		else
		{
			if ( sound->m_dependences.Size() > 0 )
			{
				html += wxString::Format( wxT("<td align=left>%s</td>"), sound->m_dependences[ 0 ].m_animation.AsString().AsChar() );
			}
			else
			{
				html += wxString::Format( wxT("<td align=left>ERROR</td>") );
			}
		}

		html += wxT("</tr>");
	}

	html += wxT("</table>");

	wxHtmlWindow* htmlWindow = XRCCTRL( *this, htmlName, wxHtmlWindow );
	htmlWindow->SetPage( html );
}

namespace
{
	void AddTokensToTree( const CTokenizer& tok, Uint32 depth, wxTreeItemId root, wxTreeCtrl* tree )
	{
		if ( depth < tok.GetNumTokens() )
		{
			wxString tokenStr = tok.GetToken( depth ).AsChar();

			// Find str in tree current level
			wxTreeItemIdValue cookie;
			for ( wxTreeItemId cur = tree->GetFirstChild( root, cookie ); cur.IsOk(); cur = tree->GetNextChild( cur, cookie ) )
			{
				if ( tree->GetItemText( cur ) == tokenStr )
				{
					AddTokensToTree( tok, depth + 1, cur, tree );

					return;
				}
			}

			// Add new node
			 wxTreeItemId cur = tree->AppendItem( root, tokenStr );

			AddTokensToTree( tok, depth + 1, cur, tree );
		}
	}
}

void CEdAnimationReporterWindow::ParseAndAddSoundToTree( SAnimReportSound* sound, wxTreeItemId root, wxTreeCtrl* tree )
{
	CTokenizer tokenizer( sound->m_soundEventName, TXT("/") );
	AddTokensToTree( tokenizer, 0, root, tree );
}

void CEdAnimationReporterWindow::RefreshSoundTree( const TDynArray< SAnimReportSound* >& list, const wxString& treeName )
{
	wxTreeCtrl* tree = XRCCTRL( *this, treeName, wxTreeCtrl );
	tree->Freeze();
	tree->DeleteAllItems();

	wxTreeItemId root = tree->AddRoot( wxT("Sounds") );

	const Uint32 size = list.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		if ( list[ i ] )
		{
			ParseAndAddSoundToTree( list[ i ], root, tree );
		}
	}

	tree->Expand( root );
	tree->Thaw();
}

void CEdAnimationReporterWindow::OnSoundDepotLinkClicked( wxHtmlLinkEvent& event )
{
	const wxHtmlLinkInfo& linkInfo = event.GetLinkInfo();
	wxString href = linkInfo.GetHref();

	if ( href == wxT("ShowSingleDep") )
	{
		m_soundShowDep = false;
		RefreshSoundsPage();
	}
	else if ( href == wxT("ShowAllDep") )
	{
		m_soundShowDep = true;
		RefreshSoundsPage();
	}
}

SAnimReportSound* CEdAnimationReporterWindow::FindSoundEventReportByName( const TDynArray< SAnimReportSound* >& list, const String& eventName ) const
{
	const Uint32 size = list.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		SAnimReportSound* s = list[ i ];
		if ( s && s->m_soundEventName == eventName )
		{
			return s;
		}
	}
	return NULL;
}

SAnimReportSound::SParentAnimation* CEdAnimationReporterWindow::FindSoundEventDep( SAnimReportSound* sound, const CName& animation, const String& animset ) const
{
	const Uint32 size = sound->m_dependences.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		SAnimReportSound::SParentAnimation& temp = sound->m_dependences[ i ];

		if ( temp.m_animation == animation && temp.m_animset == animset )
		{
			return &temp;
		}
	}
	return NULL;
}

void CEdAnimationReporterWindow::FillSoundEventList( TDynArray< SAnimReportSound* >& list, const CSkeletalAnimationSetEntry* entry, const String& localPath ) const
{
	// Local
	TDynArray< CExtAnimEvent* > events;
	entry->GetAllEvents( events );

	for ( Uint32 k=0; k<events.Size(); ++k )
	{
		CExtAnimEvent* e = events[ k ];

		if ( e && IsType< CExtAnimSoundEvent >( e ) )
		{
			CExtAnimSoundEvent* soundEvent = static_cast< CExtAnimSoundEvent* >( e );

			FillSoundEventList( list, entry, soundEvent, localPath, true );
		}
	}

	// External
	TDynArray< CExtAnimSoundEvent* > externalEvents;
	entry->GetAnimSet()->GetExternalEventsOfType( entry->GetName(), externalEvents );

	for ( Uint32 k=0; k<externalEvents.Size(); ++k )
	{
		CExtAnimSoundEvent* soundEvent = externalEvents[ k ];

		FillSoundEventList( list, entry, soundEvent, localPath, false );
	}
}

void CEdAnimationReporterWindow::FillSoundEventList( TDynArray< SAnimReportSound* >& list, const String& soundEvent ) const
{
	SAnimReportSound* sound = FindSoundEventReportByName( list, soundEvent );
	if ( !sound )
	{
		sound = new SAnimReportSound();
		sound->m_soundEventName = soundEvent;

		list.PushBack( sound );
	}
}

void CEdAnimationReporterWindow::FillSoundEventList( TDynArray< SAnimReportSound* >& list, const CSkeletalAnimationSetEntry* entry, CExtAnimSoundEvent* soundEvent, const String& localPath, Bool local ) const
{
	SAnimReportSound* sound = FindSoundEventReportByName( list, ANSI_TO_UNICODE( soundEvent->GetSoundEventName().AsChar() ) );
	if ( !sound )
	{
		sound = new SAnimReportSound();
		sound->m_soundEventName = ANSI_TO_UNICODE( soundEvent->GetSoundEventName().AsChar() );

		list.PushBack( sound );
	}

	SAnimReportSound::SParentAnimation* dep = FindSoundEventDep( sound, entry->GetName(), localPath );
	if ( dep )
	{
		if ( local )
		{
			dep->m_localNum++;
		}
	}
	else
	{
		SAnimReportSound::SParentAnimation parentAnim;
		parentAnim.m_animation = entry->GetName();
		parentAnim.m_animset = localPath;
		parentAnim.m_localNum = 0;
		parentAnim.m_externalNum = 0;

		if ( local )
		{
			parentAnim.m_localNum++;
		}
		else
		{
			parentAnim.m_externalNum++;
		}

		sound->m_dependences.PushBack( parentAnim );
	}
}

void CEdAnimationReporterWindow::FillSoundEventList( TDynArray< SAnimReportSound* >& list, CSkeletalAnimationSet* set, const String& localPath ) const
{
	const TDynArray< CSkeletalAnimationSetEntry* >& anims = set->GetAnimations();

	const Uint32 animNum = anims.Size();
	for ( Uint32 j=0; j<animNum; ++j )
	{
		CSkeletalAnimationSetEntry* entry = anims[ j ];
		if ( entry )
		{
			FillSoundEventList( list, entry, localPath );
		}
	}
}

void CEdAnimationReporterWindow::FillSoundEventList( TDynArray< SAnimReportSound* >& list, EdAnimReportAnimset* set, Bool usedAnims ) const
{
	const Uint32 size = set->m_animations.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		EdAnimReportAnimation* anim = set->m_animations[ i ];
		if ( anim )
		{
			if ( ( usedAnims && anim->m_used > 0 ) || ( !usedAnims && anim->m_used == 0 ) )
			{
				const CSkeletalAnimationSetEntry* entry = anim->m_animation;
				if ( entry )
				{
					FillSoundEventList( list, entry, set->m_path );
				}
			}
		}
	}
}

void CEdAnimationReporterWindow::FillSoundDiffTab()
{
	m_soundTabsToDiff.PushBack( TSoundTabPair( TXT("All"), &m_allSoundEvents ) );
	m_soundTabsToDiff.PushBack( TSoundTabPair( TXT("Used"), &m_usedSoundEvents ) );
	m_soundTabsToDiff.PushBack( TSoundTabPair( TXT("Unused"), &m_unusedSoundEvents ) );
	m_soundTabsToDiff.PushBack( TSoundTabPair( TXT("Depot"), &m_depotSoundEvents ) );

	wxChoice* choiceOne = XRCCTRL( *this, "diffOne", wxChoice );
	wxChoice* choiceTwo = XRCCTRL( *this, "diffTwo", wxChoice );

	choiceOne->Freeze();
	choiceTwo->Freeze();

	for ( Uint32 i=0; i<m_soundTabsToDiff.Size(); ++i )
	{
		choiceOne->Append( m_soundTabsToDiff[ i ].m_first.AsChar() );
		choiceTwo->Append( m_soundTabsToDiff[ i ].m_first.AsChar() );
	}

	choiceOne->SetSelection( 2 );
	choiceTwo->SetSelection( 1 );

	choiceOne->Thaw();
	choiceTwo->Thaw();
}

void CEdAnimationReporterWindow::GenerateSoundReport()
{
	GFeedback->BeginTask( TXT("Generate sound report..."), true );

	m_usedSoundEvents.ClearPtr();
	m_unusedSoundEvents.ClearPtr();
	m_depotSoundEvents.ClearPtr();
	m_tocheckSoundEvents.ClearPtr();
	m_allSoundEvents.ClearPtr();

	Uint32 curr = 0;
	GFeedback->UpdateTaskProgress( curr, 100 );

	// All depot sound events
	{
		TDynArray< String > depotFilesAnimsets;

		String depotPath;
		GDepot->GetAbsolutePath( depotPath );

		curr += 20;
		GFeedback->UpdateTaskProgress( curr, 100 );

		String animsetEx = TXT("*."); 
		animsetEx += CSkeletalAnimationSet::GetFileExtension();

		GFileManager->FindFiles( depotPath, animsetEx, depotFilesAnimsets, true );

		const Uint32 size = depotFilesAnimsets.Size();
		for ( Uint32 i=0; i<size; ++i )
		{
			if ( GFeedback->IsTaskCanceled() )
			{
				break;
			}

			GFeedback->UpdateTaskProgress( curr + ( 20 * i ) / size, 100 );

			const String& path = depotFilesAnimsets[ i ];
			String localPath;

			GDepot->ConvertToLocalPath( path, localPath );

			CSkeletalAnimationSet* set = LoadResource< CSkeletalAnimationSet >( localPath );
			if ( set )
			{
				FillSoundEventList( m_depotSoundEvents, set, localPath );
			}
			else
			{
				ASSERT( 0 );
			}
		}
	}

	curr += 20;

	// All sound events
	{
		TDynArray< String > soundNames;
//		GSoundSystem->GetAllEventDefinitionsNamesWithPaths( soundNames );

		const Uint32 size = soundNames.Size();
		for ( Uint32 i=0; i<size; ++i )
		{
			if ( GFeedback->IsTaskCanceled() )
			{
				break;
			}

			GFeedback->UpdateTaskProgress( curr + ( 20 * i ) / size, 100 );

			const String& soundName = soundNames[ i ];

			FillSoundEventList( m_allSoundEvents, soundName );
		}
	}

	curr += 20;

	// Used sound events
	{
		const Uint32 size = m_animsetRecords.Size();
		for ( Uint32 i=0; i<size; ++i )
		{
			GFeedback->UpdateTaskProgress( curr + ( 20 * i ) / size, 100 );

			EdAnimReportAnimset* animsetRec = m_animsetRecords[ i ];
			if ( animsetRec )
			{
				FillSoundEventList( m_usedSoundEvents, animsetRec, true );
			}
			else
			{
				ASSERT( 0 );
			}
		}
	}

	curr += 20;

	// Unused sound events
	{
		const Uint32 size = m_animsetRecords.Size();
		for ( Uint32 i=0; i<size; ++i )
		{
			GFeedback->UpdateTaskProgress( curr + ( 20 * i ) / size, 100 );

			EdAnimReportAnimset* animsetRec = m_animsetRecords[ i ];
			if ( animsetRec )
			{
				FillSoundEventList( m_unusedSoundEvents, animsetRec, false );
			}
			else
			{
				ASSERT( 0 );
			}
		}
	}

	curr += 20;

	RefreshSoundDiffTab();

	GFeedback->EndTask();

	RefreshSoundsPage();
}

void CEdAnimationReporterWindow::RefreshSoundDiffTab()
{
	m_tocheckSoundEvents.ClearPtr();

	wxChoice* choiceOne = XRCCTRL( *this, "diffOne", wxChoice );
	wxChoice* choiceTwo = XRCCTRL( *this, "diffTwo", wxChoice );

	Int32 selOne = choiceOne->GetSelection();
	Int32 selTwo = choiceTwo->GetSelection();

	if ( selOne == -1 || selTwo == -1 )
	{
		return;
	}

	TDynArray< SAnimReportSound* >* arrayOne = m_soundTabsToDiff[ selOne ].m_second;
	TDynArray< SAnimReportSound* >* arrayTwo = m_soundTabsToDiff[ selTwo ].m_second;

	const Uint32 size = arrayOne->Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		SAnimReportSound* unusedSound = (*arrayOne)[ i ];

		Bool found = false;

		for ( Uint32 j=0; j<arrayTwo->Size(); ++j )
		{
			SAnimReportSound* usedSound = (*arrayTwo)[ j ];

			if ( unusedSound->IsEqual( usedSound ) )
			{
				found = true;
				break;
			}
		}

		if ( !found )
		{
			SAnimReportSound* s = new SAnimReportSound();
			unusedSound->CopyTo( s );
			m_tocheckSoundEvents.PushBack( s );
		}
	}
}

void CEdAnimationReporterWindow::OnSoundDiffClicked( wxCommandEvent& event )
{
	RefreshSoundDiffTab();

	RefreshSoundTab( m_tocheckSoundEvents, wxT("htmlSoundTocheck") );
	RefreshSoundTree( m_tocheckSoundEvents, wxT("treeSoundTocheck") );
}
