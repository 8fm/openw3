/*
 * Copyright © 2007 CD Projekt Red. All Rights Reserved.
 */
#include "build.h"

#ifndef NO_DEBUG_PAGES

#include "storyScenePlayer.h"
#include "storySceneSystem.h"
#include "questThread.h"
#include "questsSystem.h"
#include "../engine/debugPage.h"
#include "../engine/debugPageManagerBase.h"
#include "../engine/renderFrame.h"

/// Debug page with active quests
class CDebugPageQuests : public IDebugPage
{
private:
	enum EScrollType
	{
		ST_None,
		ST_Down,
		ST_Up,
	};

	Int32			m_width;
	Int32			m_height;
	Int32			m_firstLineNum;
	Int32			m_currentLineNum;
	Int32			m_linesCount;
	EScrollType	m_currScroll;
	Float		m_scrollTimer;

public:
	CDebugPageQuests()
		: IDebugPage( TXT("Quests") )
		, m_firstLineNum( 0 )
		, m_currentLineNum( 0 )
		, m_linesCount( 0 )
		, m_currScroll( ST_None )
		, m_scrollTimer( 0 )
	{}

	void AddDebugScreenText( CRenderFrame* frame, Int32 X, Int32 Y, const String& text, const Color& colorText = Color( 255, 255, 255 ) )
	{
		if ( m_currentLineNum >= m_firstLineNum && Y < m_height + m_firstLineNum * 15 )
		{
			frame->AddDebugScreenText( X , Y - m_firstLineNum * 15, text, colorText );
		}
	}

	void NextLine( Int32& x, Int32& y )
	{
		y += 15;
		//if( y > m_height )
		//{
			// next 'page'
			//y = 80;
			//x += m_width;
		//}

		++m_currentLineNum;
	}

	void DumpHeader( CRenderFrame* frame, Int32 x, Int32& y )
	{
		frame->AddDebugScreenText( x, y, TXT( "Legend:") );
		frame->AddDebugScreenText( x + 50, y, TXT( "quests" ), Color::YELLOW );
		frame->AddDebugScreenText( x + 100, y, TXT( "blocks" ) );
		frame->AddDebugScreenText( x + 150, y, TXT( "inputs" ), Color::CYAN );
		frame->AddDebugScreenText( x + 200, y, TXT( "comments" ), Color::GRAY );
		y += 30;
	}

	void DumpThread( CRenderFrame* frame, Int32& x, Int32& y, const CQuestThread* qThread )
	{
		ASSERT( qThread != NULL );

		AddDebugScreenText( frame, x, y, qThread->GetName(), Color::YELLOW );
		NextLine( x, y );

		// show active blocks
		TDynArray< const CQuestGraphBlock* > blocks;
		qThread->GetActiveBlocks( blocks );
		for( TDynArray< const CQuestGraphBlock* >::const_iterator
			blockIter = blocks.Begin();
			blockIter != blocks.End();
			++blockIter)
		{
			x += 15;
			DumpBlock( frame, x, y, qThread, *blockIter );
			x -= 15;
		}

		// show children threads
		const TDynArray< CQuestThread* >& threads = qThread->GetChildrenThreads();
		for( TDynArray< CQuestThread* >::const_iterator
			questIter = threads.Begin();
			questIter != threads.End();
			++questIter)
		{
			x += 15;
			DumpThread( frame, x, y, *questIter );
			x -= 15;
		}
	}

	void DumpBlock( CRenderFrame* frame, Int32& x, Int32& y, const CQuestThread* qThread, 
		const CQuestGraphBlock* block )
	{
		ASSERT( block != NULL );
		const Int32 letterWidth = 5;

		// show name
		int xoffset = x;
		AddDebugScreenText( frame, xoffset, y, block->GetBlockName(), Color::WHITE );
		xoffset += letterWidth * block->GetBlockName().Size();

		// show comment
		if ( ! block->GetComment().Empty() )
		{
			AddDebugScreenText( frame, xoffset, y, block->GetComment(), Color::GRAY );
			xoffset += letterWidth * block->GetComment().Size();
		}

		// show activated inputs
		String inputStr;
		const TDynArray< CName >* inputs = qThread->GetActivatedInputs( block );
		if( inputs != NULL )
		{
			inputStr.PushBack( '(' );

			for( TDynArray< CName >::const_iterator
				inputIter = inputs->Begin();
				inputIter != inputs->End();
				++inputIter)
			{
				if( inputIter != inputs->Begin() )
				{
					inputStr.PushBack( ',' );
				}
				inputStr += inputIter->AsString();
			}

			inputStr.PushBack( ')' );
		}

		if( ! inputStr.Empty() )
		{
			AddDebugScreenText( frame, xoffset, y, inputStr, Color::CYAN );
			xoffset += letterWidth * inputStr.Size();
		}

		NextLine( x, y );
	}

	virtual void OnTick( Float timeDelta )
	{
		// Scrolling

		m_scrollTimer += timeDelta;
		if ( m_scrollTimer > 0.1f )
		{
			m_scrollTimer = 0;

			if ( m_currScroll == ST_Down )
			{
				ScrollDown();
			}
			else if ( m_currScroll == ST_Up )
			{
				ScrollUp();
			}
		}
	}

	//! Generate debug viewport fragments
	virtual void OnViewportGenerateFragments( IViewport *view, CRenderFrame *frame )
	{
		const Uint32 width = frame->GetFrameOverlayInfo().m_width;
		const Uint32 height = frame->GetFrameOverlayInfo().m_height;

		// Draw info background
		frame->AddDebugRect( 50, 20, width-100, height-100, Color( 0, 0, 0, 200 ) );
		frame->AddDebugFrame( 50, 20, width-100, height-100, Color::WHITE );

		m_width = width / 3;
		m_height = height * 6 / 7;

		Int32 y = 75;
		Int32 x = 65;

		frame->AddDebugRect( 50, 52, 300, 40, Color( 50, 50, 50, 255 ) );
		frame->AddDebugFrame( 50, 52, 300, 40, Color( 0, 0, 0, 128 ) );
		DumpHeader( frame, x, y );

		// show running quest threads
		CQuestsSystem* qSystem = GCommonGame->GetSystem< CQuestsSystem >();
		ASSERT( qSystem != NULL );

		m_currentLineNum = 0;

		const TDynArray< CQuestThread* >& threads = qSystem->GetRunningThreads();
		for( TDynArray< CQuestThread* >::const_iterator
			questIter = threads.Begin();
			questIter != threads.End();
			++questIter)
		{
			DumpThread( frame, x, y, *questIter );
		}

		m_linesCount = m_currentLineNum;
	}

	virtual Bool OnViewportInput( IViewport* view, enum EInputKey key, enum EInputAction action, Float data )
	{
		// Send the event
		if ( action == IACT_Press )
		{
			switch ( key )
			{
			case IK_Down:
			case IK_Pad_DigitDown:
				m_currScroll = ST_Down;
				break;
			case IK_Up:
			case IK_Pad_DigitUp:
				m_currScroll = ST_Up;
				break;
			case IK_PageDown:
			case IK_Pad_DigitRight:
				ScrollPageDown();
				break;
			case IK_PageUp:
			case IK_Pad_DigitLeft:
				ScrollPageUp();
				break;
			case IK_Home:
			case IK_Pad_RightThumb:
				ResetScroll();
				break;
			}
		}
		else if ( action == IACT_Release )
		{
			switch ( key )
			{
			case IK_Down:
			case IK_Pad_DigitDown:
				m_currScroll = ST_None;
				break;
			case IK_Up:
			case IK_Pad_DigitUp:
				m_currScroll = ST_None;
				break;
			}
		}

		// Not processed
		return false;
	}

private:
	void ScrollDown()
	{
		if ( ++m_firstLineNum > m_linesCount )
		{
			m_firstLineNum = m_linesCount;
		}
	}

	void ScrollUp()
	{
		if ( --m_firstLineNum < 0 )
		{
			m_firstLineNum = 0;
		}
	}

	void ScrollPageDown()
	{
		const Int32 ScrollPageSize = 5;
		if ( m_firstLineNum + ScrollPageSize > m_linesCount )
		{
			m_firstLineNum = m_linesCount;
		}
		else
		{
			m_firstLineNum += ScrollPageSize;
		}
	}
	
	void ScrollPageUp()
	{
		const Int32 ScrollPageSize = 5;
		if ( m_firstLineNum - ScrollPageSize < 0 )
		{
			m_firstLineNum = 0;
		}
		else
		{
			m_firstLineNum -= ScrollPageSize;
		}
	}

	void ResetScroll()
	{
		m_firstLineNum = 0;
	}
};

void CreateDebugPageQuests()
{
	IDebugPage* page = new CDebugPageQuests();
	IDebugPageManagerBase::GetInstance()->RegisterDebugPage( page );
}

/// Debug page with active quests
class CDebugPageScenes : public IDebugPage
{
private:
	Int32 m_width;
	Int32 m_height;
public:
	CDebugPageScenes()
		: IDebugPage( TXT("Scenes") )
	{};

	void NextLine( Int32& x, Int32& y )
	{
		y += 15;
		if( y > m_height )
		{
			// next 'page'
			y = 80;
			x += m_width;
		}
	}

	void DumpScene( CRenderFrame* frame, Int32& x, Int32& y, const CStoryScenePlayer* player )
	{
		if ( player == NULL )
		{
			return;
		}

		String sceneNameAndSection = player->GetName();
		if ( player->GetCurrentSection()  )
		{
			sceneNameAndSection.Append( TXT(" "), 1 );
			sceneNameAndSection.Append( player->GetCurrentSection()->GetName().AsChar(), player->GetCurrentSection()->GetName().GetLength() );
		}
		frame->AddDebugScreenText( x, y, sceneNameAndSection, Color::YELLOW );
		
		if ( const_cast< CStoryScenePlayer* >( player )->GetSceneDirector()->AreActorPositionsValid() == true )
		{
			frame->AddDebugScreenText( x + 500, y, TXT( "Actor positions are valid" ), Color::WHITE );
		}
		else
		{
			frame->AddDebugScreenText( x + 500, y, TXT( "ACTOR POSITIONS ARE NOT VALID" ), Color::RED );
		}
		
			
		NextLine( x, y );

		if ( player->GetCurrentSection()  )
		{
			// show actors
			TDynArray< THandle< CEntity > > actors;
			const_cast< CStoryScenePlayer* > ( player )->GetActorsUsedInCurrentSection( actors );
			Uint32 actorIndex = 0;
			for( TDynArray< THandle< CEntity > >::const_iterator it = actors.Begin(); it != actors.End(); ++it )
			{
				const CEntity* act = ( *it ).Get();
				if ( act == NULL )
				{
					continue;
				}

				frame->AddDebugScreenText( x + actorIndex * 50, y, act->GetName(), Color::GRAY );
				actorIndex += 1;
			}
		}
	}

	//! Generate debug viewport fragments
	virtual void OnViewportGenerateFragments( IViewport *view, CRenderFrame *frame )
	{
		m_width = frame->GetFrameOverlayInfo().m_width / 3;
		m_height = frame->GetFrameOverlayInfo().m_height * 6 / 7;

		// show running scenes
		CStorySceneSystem* system = GCommonGame->GetSystem< CStorySceneSystem >();

		if ( !system )
		{
			return;
		}

		TDynArray< THandle< CStoryScenePlayer > > array = system->GetScenePlayers();
		Int32 y = 65;
		Int32 x = 55;

		for( TDynArray< THandle< CStoryScenePlayer > >::const_iterator it = array.Begin(); it != array.End(); ++it )
		{
			DumpScene( frame, x, y, ( *it ).Get() );
		}
	}
};

void CreateDebugPageScenes()
{
	IDebugPage* page = new CDebugPageScenes();
	IDebugPageManagerBase::GetInstance()->RegisterDebugPage( page );
}

#endif