/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#pragma once

class CIDGraphSocket;
class CIDTopicInstance;
class CIDActivator;
class CEventSender;

//------------------------------------------------------------------------------------------------------------------
// Base class 
//------------------------------------------------------------------------------------------------------------------
class CIDGraphBlock : public CGraphBlock, public ILocalizableObject
{
	DECLARE_ENGINE_ABSTRACT_CLASS( CIDGraphBlock, CGraphBlock )

protected:
	CName					m_name;
	CEventSender*			m_events;
	Bool					m_isCheckpoint;

	#ifndef NO_EDITOR
		CGUID				m_guid;
		String				m_comment;
	#endif // ifndef NO_EDITOR

	TInstanceVar< Int16 >	i_active;
	TInstanceVar< Uint8 >	i_activeInputSocketIndex;	

public:
	CIDGraphBlock();

	RED_INLINE CName	GetName				( ) const							{ return m_name ? m_name : GetDefaultName(); }

	#ifndef NO_EDITOR
		RED_INLINE void SetName			( CName newName ) 					{ R6_ASSERT( GIsEditor ); m_name = newName; }
		RED_INLINE const CGUID& GetGuid 	( ) const							{ return m_guid; }
		RED_INLINE const String& GetComment() const							{ return m_comment; }
		RED_INLINE void SetComment( const String& comment )					{ m_comment = comment; }
		virtual void OnPropertyPostChange( IProperty* prop ); 
		virtual void OnEditorPostLoad		( );
		virtual void OnCreatedInEditor		( );
	#endif // ifndef NO_EDITOR
	
	virtual CName		GetDefaultName() const { return CNAME( Unnamed ); }

	virtual Bool		IsCheckpoint		( )	const							{ return m_isCheckpoint; };

	virtual void		SetActive			( CIDTopicInstance* topicInstance, Bool activate ) const;
	virtual Bool		IsActivated			( const CIDTopicInstance* topicInstance ) const;
	virtual Float		GetActivationAlpha	( ) const							{ return 1.0f; }

	//! Get the name of the block
	virtual String		GetBlockName		( ) const;

	virtual void		OnInitInstance		( InstanceBuffer& data ) const;
	virtual void		OnBuildDataLayout	( InstanceDataLayoutCompiler& compiler );

	// Graph evaluation
	virtual const CIDGraphBlock* ActivateInput( CIDTopicInstance* topicInstance, Float& timeDelta, const CIDGraphSocket* input, Bool evaluate = true ) const;
	virtual const CIDGraphBlock* ActivateOutput( CIDTopicInstance* topicInstance, Float& timeDelta, const CIDGraphSocket* output, Bool evaluate = true ) const;
	virtual const CIDGraphBlock* Evaluate( CIDTopicInstance* topicInstance, Float& timeDelta ) const;
	virtual Bool IsRegular() const { return true; }

	// ILocalizableObject interface
	virtual void GetLocalizedStrings( TDynArray< LocalizedStringEntry >& localizedStrings ) /*const */ {};

	virtual String GetFriendlyName() const;

	const CIDGraphSocket* FindOutputSocketByIndex( Int32 outputIndex ) const;
	Int32 FindIndexOfOutputSocket( const CIDGraphSocket* socket ) const;  
	const CIDGraphSocket* GetCurrentInput( InstanceBuffer& data ) const;

	template< class TBlockType >
	void FindPreviousBlocksOfType( Uint32 maxRecLevel, const TBlockType* testBlock, TDynArray< const TBlockType* >& outArray ) const;

	static String TrimCaptionLine( const String& captionLine );
};

template< class TBlockType >
void CIDGraphBlock::FindPreviousBlocksOfType( Uint32 maxRecLevel, const TBlockType* testBlock, TDynArray< const TBlockType* >& outArray ) const
{
	const TBlockType* thisBlock = Cast < const TBlockType > ( this );
	if ( thisBlock && thisBlock != testBlock )
	{
		outArray.PushBack( thisBlock );
		return; // do not go further
	}

	if ( maxRecLevel == 0 )
	{
		// no more recurrent calls, sorry
		return;
	}

	// Go through all the inputs
	for ( Uint32 i = 0; i < m_sockets.Size(); ++i )
	{
		if ( m_sockets[ i ]->GetDirection() == LSD_Input )
		{
			const TDynArray< CGraphConnection* >& connections = m_sockets[ i ]->GetConnections();
			for ( Uint32 k = 0; k < connections.Size(); ++k )
			{
				const CIDGraphBlock* block = Cast< const CIDGraphBlock > ( connections[ k ]->GetDestination()->GetBlock() );
				if ( block )
				{
					// recurrent call ( limited to some sane value, to prevent stack overflow )
					block->FindPreviousBlocksOfType< TBlockType > ( maxRecLevel - 1, testBlock, outArray );
				}
			}
		}
	}
}

BEGIN_ABSTRACT_CLASS_RTTI( CIDGraphBlock )
	PARENT_CLASS( CGraphBlock )
	PROPERTY_EDIT( m_name, TXT("") )
	PROPERTY_INLINED( m_events, TXT("You can call some events in here") )
	PROPERTY_EDIT( m_isCheckpoint, TXT("") );
	#ifndef NO_EDITOR
		PROPERTY_RO_NOT_COOKED( m_guid, TXT("") )
		PROPERTY_EDIT_NOT_COOKED( m_comment, TXT("") )
	#endif // ifndef NO_EDITOR
END_CLASS_RTTI()

//------------------------------------------------------------------------------------------------------------------
// Input point to the dialog thread
//------------------------------------------------------------------------------------------------------------------
class CIDGraphBlockInput : public CIDGraphBlock
{
	DECLARE_ENGINE_CLASS( CIDGraphBlockInput, CIDGraphBlock, 0 )

private:
	CIDActivator*		m_activator;

public:
	CIDGraphBlockInput() {}

	virtual CName GetDefaultName() const { return CNAME( Input ); }

	virtual void OnInitInstance( InstanceBuffer& data ) const;
	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );

	//! Rebuild block layout
	virtual void OnRebuildSockets();

	//! Get title bar color
	virtual Color GetTitleColor() const;

	//! Get client color
	virtual Color GetClientColor() const;

	//! Get block shape
	virtual EGraphBlockShape GetBlockShape() const;

	//! GetThe priority to start
	virtual	Uint32 GetPriority() const;

	//! Is the condition fulfilled to be launched ?
	virtual Bool GetWantsToPlay( InstanceBuffer& data, Uint32 SceneId ) const;

	virtual const CIDGraphBlock* Evaluate( CIDTopicInstance* topicInstance, Float& timeDelta ) const;
};

BEGIN_CLASS_RTTI( CIDGraphBlockInput );
	PARENT_CLASS( CIDGraphBlock );
	PROPERTY_INLINED( m_activator, TXT("Condition to trigger the topic") )
END_CLASS_RTTI();



//------------------------------------------------------------------------------------------------------------------
// This struct is used to sort thread starts
//------------------------------------------------------------------------------------------------------------------
struct SIDGraphBlockInputSortingPredicate
{
	Bool operator()( const CIDGraphBlockInput* t1, const CIDGraphBlockInput* t2 ) const
	{
		return 	t1->GetPriority() <= t2->GetPriority();
	}
};

//------------------------------------------------------------------------------------------------------------------
// Output point of the dialog thread
//------------------------------------------------------------------------------------------------------------------
class CIDGraphBlockOutput : public CIDGraphBlock
{
	DECLARE_ENGINE_CLASS( CIDGraphBlockOutput, CIDGraphBlock, 0 )

public:
	CIDGraphBlockOutput() {}

	virtual CName GetDefaultName() const { return CNAME( Output ); }

	//! Rebuild block layout
	virtual void OnRebuildSockets();

	//! Get title bar color
	virtual Color GetTitleColor() const;

	//! Get client color
	virtual Color GetClientColor() const;

	//! Get block shape
	virtual EGraphBlockShape GetBlockShape() const;

	virtual const CIDGraphBlock* Evaluate( CIDTopicInstance* topicInstance, Float& timeDelta ) const;
};

BEGIN_CLASS_RTTI( CIDGraphBlockOutput );
	PARENT_CLASS( CIDGraphBlock );
END_CLASS_RTTI();

//------------------------------------------------------------------------------------------------------------------
// Forks the thread
//------------------------------------------------------------------------------------------------------------------
class CIDGraphBlockFork : public CIDGraphBlock
{
	DECLARE_ENGINE_CLASS( CIDGraphBlockFork, CIDGraphBlock, 0 )

protected:
	Uint32	m_numOutputs;

public:
	CIDGraphBlockFork() : m_numOutputs( 2 ) {}

	virtual void OnPropertyPostChange( IProperty* prop ); 

	//! Get the name of the block
	virtual CName GetDefaultName() const { return CNAME( Fork ); }

	//! Rebuild block layout
	virtual void OnRebuildSockets();

	// Graph evaluation
	virtual const CIDGraphBlock* Evaluate( CIDTopicInstance* topicInstance, Float& timeDelta ) const;

	//! Get block shape
	virtual EGraphBlockShape GetBlockShape() const;

	//! Get client color
	virtual Color GetClientColor() const;
};

BEGIN_CLASS_RTTI( CIDGraphBlockFork )
	PARENT_CLASS( CIDGraphBlock )
	PROPERTY_EDIT_RANGE( m_numOutputs, TXT("Forks the execution thread into several ones (signal goes out from every output)"), 2, 10 )
END_CLASS_RTTI()
