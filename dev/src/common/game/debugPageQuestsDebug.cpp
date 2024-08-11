/*
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"

// tempshit
#include "../../common/core/doubleList.h"
#include "questsSystem.h"
#include "quest.h"
#include "questThread.h"
#include "questGraphSocket.h"
#include "../engine/debugPage.h"
#include "../engine/debugPageManagerBase.h"
#include "../engine/renderFrame.h"

#ifndef NO_DEBUG_PAGES
RED_WARNING_PUSH()
RED_DISABLE_WARNING_MSC( 4512 )
RED_MESSAGE( "CDebugPageQuestListener needs to address warning 4512 ( No assignment operator can be generated )" )
class CDebugPageQuestListener : public IQuestSystemListener
{
public:
	CDebugPageQuestListener( TDoubleList< String >& log )
		: m_log( log )
	{

	}

	virtual ~CDebugPageQuestListener() {}

	virtual void OnQuestStarted( CQuestThread* thread, CQuest& quest )
	{
		m_log.PushBack( String::Printf( TXT( "Quest '%ls' started" ), quest.GetDepotPath().AsChar() ) );
	}

	virtual void OnQuestStopped( CQuestThread* thread )
	{
		m_log.PushBack( String::Printf( TXT( "Quest '%ls' stopped" ), ( thread ? thread->GetName().AsChar() : TXT( "'NULL'" ) ) ) );
	}

	virtual void OnSystemPaused( bool paused )
	{
		m_log.PushBack( String::Printf( TXT( "Quest %s" ), ( paused ? TXT( "paused" ) : TXT( "resumed" ) ) ) );
	}

	virtual void OnThreadPaused( CQuestThread* thread, bool paused )
	{
		String desc = TXT( "[" ) + thread->GetName() + TXT( "]" ) + (paused ? TXT( " thread paused ") : TXT( " thread resumed"));
		m_log.PushBack( desc );
	}

	virtual void OnAddThread( CQuestThread* parentThread, CQuestThread* thread )
	{
		String desc = GetThreadDesc( parentThread, thread ) + TXT( " thread started" );
		m_log.PushBack( desc );
	}

	virtual void OnRemoveThread( CQuestThread* parentThread, CQuestThread* thread )
	{
		String desc = GetThreadDesc( parentThread, thread ) + TXT( " thread killed" );
		m_log.PushBack( desc );
	}

	virtual void OnAddBlock( CQuestThread* thread, const CQuestGraphBlock* block )
	{
		String desc = GetBlockDesc( thread, block ) + TXT( " started in thread " );
		m_log.PushBack( desc );
	}

	virtual void OnRemoveBlock( CQuestThread* thread, const CQuestGraphBlock* block )
	{
		// check what outputs were activated
		InstanceBuffer& data = thread->GetInstanceData();
		if ( block->WasOutputActivated( data ) )
		{
			// block's output was activated

			CName outputName = block->GetActivatedOutputName( data );

#ifndef NO_EDITOR_GRAPH_SUPPORT

			CQuestGraphSocket* socket = block->FindSocket< CQuestGraphSocket >( outputName );

			if ( !socket )
			{
				// non-existing output was activated!!!
				m_log.PushBack( GetBlockDesc( thread, block ) + TXT( " exited using an invalid output" ) );
			}
			else if ( socket->GetDirection() != LSD_Output )
			{
				// this is not an output socket
				m_log.PushBack( GetBlockDesc( thread, block ) + TXT( " exited with a non-output socket [" ) + outputName.AsString() + TXT( "]" ) );
			}
			else if ( !block->IsBlockEnabled( data ) )
			{
				// the block is inactive - this should NEVER happen
				m_log.PushBack( GetBlockDesc( thread, block ) + TXT( " has been deactivated while exiting" ) );
			}
			else
			{
				// everything seems ok, just make sure the output the block activated
				// is connected to something
				if ( socket->GetConnections().Empty() )
				{
					m_log.PushBack( GetBlockDesc( thread, block ) + TXT( " exited with [" ) + outputName.AsString() + TXT( "], which is NOT CONNECTED to anything" ) );
				}
				else
				{
					m_log.PushBack( GetBlockDesc( thread, block ) + TXT( " correctly exited with [" ) + outputName.AsString() + TXT( "]" ) );
				}
			}

#else
			m_log.PushBack( GetBlockDesc( thread, block ) + TXT( " exited using an invalid output" ) );
#endif
		}
		else
		{
			// block's output WASN'T activated

			// check if any errors are set
			if ( !block->GetErrorMsg( data ).Empty() )
			{
				m_log.PushBack( GetBlockDesc( thread, block ) + TXT( " has thrown an exception: " ) + block->GetErrorMsg( data ) );
			}
			else if ( !block->IsBlockEnabled( data ) )
			{
				// the block was cut controlled
				m_log.PushBack( GetBlockDesc( thread, block ) + TXT( " was CUT-CONTROLLED" ) );
			}
			else
			{
				// an output wasn't cut controlled, this indicates that the thread the block
				// was in died - it probably was cut controlled
				m_log.PushBack( GetBlockDesc( thread, block ) + TXT( " died, because the phase it was in was CUT_CONTROLLED" ) );
			}
		}
	}

	virtual void OnBlockInputActivated( CQuestThread* thread, const CQuestGraphBlock* block )
	{
		String desc = GetBlockDesc( thread, block ) + TXT( " input activated" );
		m_log.PushBack( desc );
	}

private:
	TDoubleList< String >& m_log;

	String GetThreadDesc( CQuestThread* parentThread, CQuestThread* thread ) const
	{
		String parentThreadName = parentThread ? parentThread->GetName() : TXT( "'NULL'" );
		String threadName = thread ? thread->GetName() : TXT( "'NULL'" );
		return TXT( "[" ) + parentThreadName + TXT( ":" ) + threadName + TXT( "]" );
	}


	String GetBlockDesc( CQuestThread* thread, const CQuestGraphBlock* block ) const
	{
		String threadName = thread ? thread->GetName() : TXT( "'NULL'" );
		String blockName = block ? block->GetCaption() : TXT( "'NULL'" );
		return TXT( "[" ) + threadName + TXT( "->" ) + blockName + TXT( "]" );
	}
};
RED_WARNING_POP()
/// Debug page with log from quest system
class CDebugPageQuestsDebug : public IDebugPage
{
private:
	enum EScrollType
	{
		ST_None,
		ST_Down,
		ST_Up,
	};

	static const Uint32  m_logSize = 100;

	Int32			m_width;
	Int32			m_height;
	Int32			m_firstLineNum;
	Int32			m_currentLineNum;
	Int32			m_linesCount;
	EScrollType	m_currScroll;
	Float		m_scrollTimer;
	TDoubleList< String > m_log;
	Bool		m_isEnabled;
	CDebugPageQuestListener* m_listener;

public:
	CDebugPageQuestsDebug()
		: IDebugPage( TXT("Quests Debug") )
		, m_firstLineNum( 0 )
		, m_currentLineNum( 0 )
		, m_linesCount( 0 )
		, m_currScroll( ST_None )
		, m_scrollTimer( 0 )
		, m_isEnabled( false )
		, m_listener( new CDebugPageQuestListener( m_log ) )
	{
	}

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

		// Keep log in size
		while( m_log.Size() > m_logSize )
		{
			m_log.PopFront();
		}
	}

	//! Generate debug viewport fragments
	virtual void OnViewportGenerateFragments( IViewport *view, CRenderFrame *frame )
	{
		m_width = frame->GetFrameOverlayInfo().m_width / 3;
		m_height = frame->GetFrameOverlayInfo().m_height * 6 / 7;

		Int32 y = 75;
		Int32 x = 55;

		frame->AddDebugScreenText( x, y, String::Printf( TXT( "( press 'M' to %s listening )" ),
			m_isEnabled ? TXT( "DISABLE" ) : TXT( "ENABLE" ) ), Color( 255, 255, 0 ) );
		y += 15;

		m_currentLineNum = 0;

		for( TDoubleList< String >::const_iterator lineIter = m_log.Begin();
			lineIter != m_log.End(); ++lineIter )
		{
			AddDebugScreenText( frame, x, y, *lineIter );

			++m_currentLineNum;
			y += 15;
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
				m_currScroll = ST_Down;
				break;
			case IK_Up:
				m_currScroll = ST_Up;
				break;
			case IK_PageDown:
				ScrollPageDown();
				break;
			case IK_PageUp:
				ScrollPageUp();
				break;
			case IK_Home:
				ResetScroll();
				break;
			case IK_End:
				m_firstLineNum = Max< Int32 >( 0, m_log.Size() - 15 );
				break;
			case IK_M:
				ToggleListener();
				break;
			}
		}
		else if ( action == IACT_Release )
		{
			switch ( key )
			{
			case IK_Down:
				m_currScroll = ST_None;
				break;
			case IK_Up:
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

	void ToggleListener()
	{
		CQuestsSystem* qSystem = GCommonGame->GetSystem< CQuestsSystem >();
		ASSERT( qSystem != NULL );

		if( m_isEnabled )
		{
			// Disable listener
			m_isEnabled = false;

			qSystem->DetachListener( *m_listener );
		}
		else
		{
			// Enable listener
			m_isEnabled = true;

			qSystem->AttachListener( *m_listener );
		}
	}
};

void CreateDebugPageQuestsDebug()
{
	IDebugPage* page = new CDebugPageQuestsDebug();
	IDebugPageManagerBase::GetInstance()->RegisterDebugPage( page );
}

#endif