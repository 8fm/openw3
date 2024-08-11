/*
 * Copyright © 2007 CD Projekt Red. All Rights Reserved.
 */
#include "build.h"
#include "questsSystem.h"
#include "questThread.h"
#include "../engine/debugCheckBox.h"
#include "../engine/debugPage.h"
#include "../engine/debugPageManagerBase.h"
#include "../engine/renderFrame.h"

#ifndef NO_DEBUG_PAGES

/// Debug page with active quests
class CDebugPageQuests2 : public IDebugPage, public IQuestSystemListener
{
public:
	using IDebugPage::operator new;
	using IDebugPage::operator delete;

private:

	enum EDPObjectType
	{
		DPOT_Thread,
		DPOT_Block,
		DPOT_Output
	};

	class CDPObject : public IDebugCheckBox, Red::System::NonCopyable
	{
	private:
		const EDPObjectType m_type;

	public:
		CDPObject( EDPObjectType type, IDebugCheckBox* parent, const String& name, Bool canExpand, Bool canCheck )
			: IDebugCheckBox( parent, name, canExpand, canCheck )
			, m_type( type )
		{};

		RED_INLINE EDPObjectType GetType() const { return m_type; }
	};

	class CDPQuestThread : public CDPObject
	{
	private:
		CQuestThread* m_thread;

	public:
		CDPQuestThread( IDebugCheckBox* parent, CQuestThread* thread )
			: CDPObject( DPOT_Thread, parent, thread->GetName(), true, false )
			, m_thread( thread )
		{
		}

		RED_INLINE Bool operator==( const CQuestThread* thread ) const
		{
			return m_thread == thread;
		}

		virtual void CalculateColor( Color& color, Bool isSelected )
		{
			if( isSelected )
			{
				color = Color::WHITE;
			}
			else
			{
				color = Color( 247, 159, 30, 255 );
			}
		}

		RED_INLINE void Proggress( const CQuestGraphBlock* block, const CName& output )
		{
			m_thread->ForceBlockExit( block, output );
		}

		RED_INLINE void AttachListener( IQuestSystemListener& listener ) const
		{
			m_thread->AttachListener( listener );
		}

		RED_INLINE void DetachListener( IQuestSystemListener& listener ) const
		{
			m_thread->DetachListener( listener );
		}
	};

	class CDPQuestBlock : public CDPObject
	{
	private:
		const CQuestGraphBlock* m_block;

	public:
		CDPQuestBlock( IDebugCheckBox* parent, const CQuestGraphBlock* block )
			: CDPObject( DPOT_Block, parent, block->GetBlockName(), true, false )
			, m_block( block )
		{
		}

		RED_INLINE Bool operator==( const CQuestGraphBlock* block ) const
		{
			return m_block == block;
		}

		virtual void CalculateColor( Color& color, Bool isSelected )
		{
			if( isSelected )
			{
				color = Color::WHITE;
			}
			else
			{
				color = Color::GREEN;
			}
		}

		void Proggress( const CName& output )
		{
			CDPQuestThread* parent = static_cast< CDPQuestThread* >( m_parent );
			if( parent )
			{
				parent->Proggress( m_block, output );
			}
		}
	};

	class CDPQuestOutput : public CDPObject
	{
	private:
		const CName	m_output;

	public:
		CDPQuestOutput( IDebugCheckBox* parent, const CName& output )
			: CDPObject( DPOT_Output, parent, output.AsString(), false, false )
			, m_output( output )
		{
		}

		virtual void CalculateColor( Color& color, Bool isSelected )
		{
			if( isSelected )
			{
				color = Color::WHITE;
			}
			else
			{
				color = Color( 20, 255, 255, 255 );
			}
		}

		virtual Bool OnInput( enum EInputKey key, enum EInputAction action, Float data )
		{
			if( action == IACT_Press && ( key == IK_Pad_A_CROSS || key == IK_Space ) )
			{
				CDPQuestBlock* parent = static_cast< CDPQuestBlock* >( m_parent );
				if( parent )
				{
					parent->Proggress( m_output );
				}

				return true;
			}

			return false;
		}
	};

private:

	CDebugOptionsTree*	m_tree;
	TDynArray< CDPQuestThread* > m_roots;

public:
	CDebugPageQuests2()
		: IDebugPage( TXT("Quest Forcer") )
		, m_tree( NULL )
	{}

	~CDebugPageQuests2()
	{
		delete m_tree; m_tree = NULL;
	}

	virtual void OnPageShown()
	{
		if ( !m_tree )
		{
			m_tree = new CDebugOptionsTree( 50, 50, 300, 500, this );
		}

		CQuestsSystem* qSystem = GCommonGame->GetSystem< CQuestsSystem >();
		if( qSystem )
		{
			const TDynArray< CQuestThread* >& roots = qSystem->GetRunningThreads();
			for( TDynArray< CQuestThread* >::const_iterator it = roots.Begin(); it != roots.End(); ++it )
			{
				CDPQuestThread* dpThread = new CDPQuestThread( NULL, *it );
				m_tree->AddRoot( dpThread );
				m_roots.PushBack( dpThread );
				dpThread->AttachListener( *this );
			}
		}
	}

	virtual void OnPageHidden()
	{
		for( TDynArray< CDPQuestThread* >::const_iterator it = m_roots.Begin(); it != m_roots.End(); ++it )
		{
			(*it)->DetachListener( *this );
		}

		m_roots.Clear();
		m_tree->Clear();
	}

	virtual void OnViewportGenerateFragments( IViewport *view, CRenderFrame *frame )
	{
		const Uint32 width = frame->GetFrameOverlayInfo().m_width;
		const Uint32 height = frame->GetFrameOverlayInfo().m_height;

		// Draw info background
		frame->AddDebugRect( 50, 50, width-100, height-130, Color( 0, 0, 0, 200 ) );
		frame->AddDebugFrame( 50, 50, width-100, height-130, Color::WHITE );

		if ( m_tree )
		{
			m_tree->m_height = height-130;
			m_tree->OnRender( frame );
		}
	};

	virtual Bool OnViewportInput( IViewport* view, enum EInputKey key, enum EInputAction action, Float data )
	{
		if ( m_tree )
		{
			m_tree->OnInput( key, action, data );
		}

		return true;
	};

	virtual void OnTick( Float timeDelta )
	{
		if ( m_tree )
		{
			m_tree->OnTick( timeDelta );
		}
	};

	CDPQuestThread* FindDPThread( CDPQuestThread* rootDPThread, const CQuestThread* searchedThread )
	{
		if( rootDPThread )
		{
			if( *rootDPThread == searchedThread )
			{
				return rootDPThread;
			}

			const TDynArray< IDebugCheckBox* >& children = rootDPThread->GetChildren();
			for( TDynArray< IDebugCheckBox* >::const_iterator it = children.Begin(); it != children.End(); ++it )
			{
				CDPObject* dpObject = static_cast< CDPObject* >( *it );
				if( dpObject->GetType() == DPOT_Thread )
				{
					CDPQuestThread* dpThread = static_cast< CDPQuestThread* >( dpObject );
					ASSERT( dpThread );
					
					dpThread = FindDPThread( dpThread, searchedThread );
					if( dpThread )
					{
						return dpThread;
					}
				}
			}
		}
		else
		{
			for( TDynArray< CDPQuestThread* >::const_iterator it = m_roots.Begin(); it != m_roots.End(); ++it )
			{
				CDPQuestThread* dpThread = FindDPThread( *it, searchedThread );
				if( dpThread )
				{
					return dpThread;
				}
			}
		}

		return NULL;
	}

	CDPQuestBlock* FindDPBlock( const CDPQuestThread* rootDPThread, const CQuestGraphBlock* searchedBlock )
	{
		if( rootDPThread )
		{
			const TDynArray< IDebugCheckBox* >& children = rootDPThread->GetChildren();
			for( TDynArray< IDebugCheckBox* >::const_iterator it = children.Begin(); it != children.End(); ++it )
			{
				CDPObject* dpObject = static_cast< CDPObject* >( *it );
				if( dpObject->GetType() == DPOT_Thread )
				{
					CDPQuestThread* dpThread = static_cast< CDPQuestThread* >( dpObject );
					ASSERT( dpThread );

					CDPQuestBlock* dpBlock = FindDPBlock( dpThread, searchedBlock );
					if( dpBlock )
					{
						return dpBlock;
					}
				}
				else if( dpObject->GetType() == DPOT_Block )
				{
					CDPQuestBlock* dpBlock = static_cast< CDPQuestBlock* >( dpObject );
					ASSERT( dpBlock );

					if( *dpBlock == searchedBlock )
					{
						return dpBlock;
					}
				}
			}
		}
		else
		{
			for( TDynArray< CDPQuestThread* >::const_iterator it = m_roots.Begin(); it != m_roots.End(); ++it )
			{
				CDPQuestBlock* dpBlock = FindDPBlock( *it, searchedBlock );
				if( dpBlock )
				{
					return dpBlock;
				}
			}
		}

		return NULL;
	}

	// IQuestSystemListener implementation
	virtual void OnQuestStarted( CQuestThread* thread, CQuest& quest ) {};
	virtual void OnQuestStopped( CQuestThread* thread ) {};
	virtual void OnSystemPaused( bool paused ) {};

	virtual void OnThreadPaused( CQuestThread* thread, bool paused ) {};

	virtual void OnAddThread( CQuestThread* parentThread, CQuestThread* thread )
	{
		// Search for corresponding CDPQuestThread containing the parent thread and create new child
		CDPQuestThread* dpThread = FindDPThread( NULL, parentThread );
		if( dpThread )
		{
			new CDPQuestThread( dpThread, thread );
		}
	}

	virtual void OnRemoveThread( CQuestThread* parentThread, CQuestThread* thread )
	{
		CDPQuestThread* dpThread = FindDPThread( NULL, thread );
		if( dpThread )
		{
			m_tree->SelectItem( dpThread->GetParent() );
			delete dpThread;
		}
	}

	virtual void OnAddBlock( CQuestThread* thread, const CQuestGraphBlock* block )
	{
		CDPQuestBlock* newBlock = NULL;

		// Search for corresponding CDPQuestThread containing the parent thread and create new child
		CDPQuestThread* dpThread = FindDPThread( NULL, thread );
		if( dpThread )
		{
			newBlock = new CDPQuestBlock( dpThread, block );
		}

		if( newBlock )
		{
			TDynArray< CName > outputs;
			block->GetAllOutputNames( outputs );
			for( TDynArray< CName >::const_iterator it = outputs.Begin(); it != outputs.End(); ++it )
			{
				new CDPQuestOutput( newBlock, *it );
			}
		}
	}

	virtual void OnRemoveBlock( CQuestThread* thread, const CQuestGraphBlock* block )
	{
		CDPQuestBlock* dpBlock = FindDPBlock( NULL, block );

		if( dpBlock )
		{
			m_tree->SelectItem( dpBlock->GetParent() );
			delete dpBlock;
		}
	}

	virtual void OnBlockInputActivated( CQuestThread* thread, const CQuestGraphBlock* block ) {};
};

void CreateDebugPageQuests2()
{
	IDebugPage* page = new CDebugPageQuests2();
	IDebugPageManagerBase::GetInstance()->RegisterDebugPage( page );
}

#endif