/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#pragma once

#include "idGraph.h"
#include "idSystem.h"


class CIDThreadInstance;
class CIDActivator;
enum EIDPriority;

//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
class CIDTopic : public CObject, public ILocalizableObject
{
	DECLARE_ENGINE_CLASS( CIDTopic, CObject, 0 );

protected:
	CName				m_name;
	CIDGraph*			m_graph;	
	InstanceDataLayout	m_dataLayout;

	CIDActivator*		m_activator;
	Bool				m_endOnInterrupted;
	Bool				m_canBeRestarted;
	Bool				m_conditionCheckedEachFrame;

	//Float				m_WaitingMaxTime;

public:
	CIDTopic();

	RED_INLINE		CIDGraph*			GetGraph				( )			{ return m_graph;						}
	RED_INLINE		CIDGraph*			GetGraph				( )	const	{ return m_graph;						}
	RED_INLINE const	CName&				GetName					( )	const	{ return m_name;						}
	RED_INLINE const	InstanceDataLayout&	GetDataLayout			( )	const	{ return m_dataLayout;					}
	RED_INLINE		CIDActivator*		GetActivator			( )	const	{ return m_activator;					}
	RED_INLINE		Bool				GetEndOnInterrupted		( )	const	{ return m_endOnInterrupted;			}
	RED_INLINE		Bool				GetCanBeRestarted		( )	const	{ return m_canBeRestarted;				}
	RED_INLINE		Bool				GetConditionIsContinous	( )	const	{ return m_conditionCheckedEachFrame;	}

	void CompileDataLayout();
	void SetupTheGraph();
	void OnPostChange();

	template< class TBlockType > class BlockConstIterator : public Red::System::NonCopyable
	{
	private:
		const TDynArray< CGraphBlock* >&	m_blocks;
		Int32								m_currentBlock;

	public:
		RED_INLINE BlockConstIterator( const CIDTopic* topic ) : m_blocks( topic->m_graph->GraphGetBlocks() ), m_currentBlock( -1 ) { Next(); }
		RED_INLINE const TBlockType* operator*() const { return m_currentBlock < m_blocks.SizeInt() ? Cast< const TBlockType > ( m_blocks[ m_currentBlock ] ) : NULL; }
		RED_INLINE operator Bool() const { return m_currentBlock < m_blocks.SizeInt(); }
		RED_INLINE void operator++() { Next(); }
		RED_INLINE void Next()
		{
			while ( ++m_currentBlock < m_blocks.SizeInt() )
			{
				if ( m_blocks[ m_currentBlock ]->IsA< TBlockType > () )
				{
					break;
				}
			}
		}
	};

	#ifndef NO_EDITOR
		template< class TBlockType > class BlockIterator : public Red::System::NonCopyable
		{
		private:
			const TDynArray< CGraphBlock* >&	m_blocks;
			Int32								m_currentBlock;

		public:
			RED_INLINE BlockIterator( CIDTopic* topic ) : m_blocks( topic->m_graph->GraphGetBlocks() ), m_currentBlock( -1 ) { Next(); }
			RED_INLINE TBlockType* operator*() { return m_currentBlock < m_blocks.SizeInt() ? Cast< TBlockType > ( m_blocks[ m_currentBlock ] ) : NULL; }
			RED_INLINE operator Bool() const { return m_currentBlock < m_blocks.SizeInt(); }
			RED_INLINE void operator++() { Next(); }
			RED_INLINE void Next()
			{
				while ( ++m_currentBlock < m_blocks.SizeInt() )
				{
					if ( m_blocks[ m_currentBlock ]->IsA< TBlockType > () )
					{
						break;
					}
				}
			}
		};

		// it's better to use iterator directly in most cases, this is just a convenience method for some rare cases
		template< class TBlockType > void GatherBlocks( TDynArray< const TBlockType* > &blockArray ) const
		{
			for ( BlockConstIterator< TBlockType > it( this ); it; ++it )
			{
				blockArray.PushBack( *it );
			}
		}

		// it's better to use iterator directly in most cases, this is just a convenience method for some rare cases
		template< class TTextBlockType > void GatherBlocks( TDynArray< const TTextBlockType* > &blockArray, Uint32 &totalLines ) const
		{
			for ( BlockConstIterator< TTextBlockType > it( this ); it; ++it )
			{
				blockArray.PushBack( *it );
				totalLines += ( *it )->GetNumLines();
			}
		}

		RED_INLINE void SetName( CName name ) { m_name = name; }

		virtual void OnCreatedInEditor();

		void GatherBlocksByGraphOrder( const CIDGraphSocket* outputSocket, TDynArray< const CIDGraphBlock* > &blockArray ) const;
		void GatherBlocksByGraphOrder( TDynArray< const CIDGraphBlock* > &blockArray ) const;
		void EnsureBlockNameUniqueness();
		void EnsureBlockNameUniqueness( CIDGraphBlock* block );
		const CIDGraphBlock* GetLastBlockByGraphXPosition();
	#endif // ifndef NO_EDITOR

	// ILocalizableObject interface
	virtual void GetLocalizedStrings( TDynArray< LocalizedStringEntry >& localizedStrings ) /*const */;
};

BEGIN_CLASS_RTTI( CIDTopic )
	PARENT_CLASS( CObject )
	PROPERTY_EDIT	( m_name, TXT("Topic name") )
	PROPERTY		( m_graph )
	PROPERTY_EDIT	( m_endOnInterrupted , TXT("if False, will try to continue the topic as soon as it stops being interrupted") )
	PROPERTY_EDIT	( m_canBeRestarted , TXT("If true, it can be activated again once it has been finished") )
	PROPERTY_EDIT	( m_conditionCheckedEachFrame , TXT("If true, if the condition of the activator is not fulfilled, the topic stops") )
	//PROPERTY_EDIT	( m_WaitingMaxTime , TXT("Time it will keep waiting while i can't be played yet") )
	PROPERTY_INLINED( m_activator, TXT("Condition and priority of the topic") )
END_CLASS_RTTI()

//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
class CIDTopicInstance : public CIDStateMachine
{
protected:
	const CIDTopic*							m_topic;
	InstanceBuffer*							m_data;
	CInteractiveDialogInstance*				m_parent;

	Uint32									m_currentlyTickingThread;
	TDynArray< CIDThreadInstance* >			m_runningThreads;
	TDynArray< CIDThreadInstance* >			m_finishedThreads;
	TDynArray< const CIDGraphBlockInput* >	m_threadStarts;

	static const Uint32						INVALID_THREAD = ( Uint32 ) -1;


public:
	CIDTopicInstance( CInteractiveDialogInstance* parent, const CIDTopic* topic );
	~CIDTopicInstance();

	RED_INLINE	const CIDTopic*				GetTopic			() const	{ return m_topic; }
	RED_INLINE	InstanceBuffer&				GetInstanceData		()			{ return *m_data; }
	RED_INLINE	const InstanceBuffer&		GetInstanceData		() const	{ return *m_data; }
	RED_INLINE	CInteractiveDialogInstance* GetDialogInstance	()			{ return m_parent; }
	
	
	CIDThreadInstance*	GetCurrentThread			( );

	EIDPriority			GetPriority					( )		const;
	Bool				GetWantsToPlay				( )		const;
	Uint32				GetDialogInstanceID			( )		const;

	void				OnStart						( Float& timeDelta );
	void				OnInterrupt					( );
	void				OnEnd						( );
	void				Tick						( Float timeDelta );
	void				Fork						( const CIDGraphSocket* forkSocket );
	void				OnChoiceSelected			( EHudChoicePosition position );
	//void				SetCheckpoint				( const CIDGraphBlock*	checkpoint );

	void				Debug_GatherThreadInfos		( TDynArray< Debug_SIDThreadInfo >& infos ) const;

	// State machine
	//virtual void		SetPlayState				(  EIDPlayState newState );

private:
	void				InitBlocks					( const CIDTopic* topic );
	void				TryCollectThreadStart		( const CIDGraphBlock* block );
	void				SortThreadStarts			( );
	void				TickThreads					( Float timeDelta );
	void				Resume						( Float& timeDelta );
	Bool				IsActivatorWantingToPlay	( )	const;
};


//------------------------------------------------------------------------------------------------------------------
// This struct is used to sort topics
//------------------------------------------------------------------------------------------------------------------
struct SIDTopicSortingPredicate
{
	Bool operator()( const CIDTopicInstance* t1, const CIDTopicInstance* t2 ) const
	{
		Int32	l_P1	= static_cast< Int32 >( t1->GetPriority() );
		Int32	l_P2	= static_cast< Int32 >( t2->GetPriority() );

		return 	l_P1 <= l_P2;
	}
};
