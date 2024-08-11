/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#pragma once


class IIDContition;


//------------------------------------------------------------------------------------------------------------------
// Defines an interlocutor for the dialog
//------------------------------------------------------------------------------------------------------------------
struct SIDInterlocutorDefinition
{
	DECLARE_RTTI_STRUCT( SIDInterlocutorDefinition );

	CName	m_interlocutorId;
	CName	m_voiceTagToMatch;
	CName	m_tagToMatch;

	RED_INLINE Bool operator==( const SIDInterlocutorDefinition& other ) const { return m_interlocutorId == other.m_interlocutorId; } 
};

BEGIN_NODEFAULT_CLASS_RTTI( SIDInterlocutorDefinition )
	PROPERTY_EDIT( m_interlocutorId, TXT("Script name of the interlocutor") )
	PROPERTY_CUSTOM_EDIT( m_voiceTagToMatch, TXT("Voice tag, an interlocutor must have to match this ID"), TXT( "EntityVoiceTagSelect" ) )
	PROPERTY_EDIT( m_tagToMatch, TXT("Tag used to find the actor during game, if there are many actors with same voiceTag") )
END_CLASS_RTTI()

// for backwards compatiblity, please do not use this struct
struct SIDActorDefinition : public SIDInterlocutorDefinition
{
	DECLARE_RTTI_STRUCT( SIDActorDefinition );
}; 

// for backwards compatiblity, please do not use this struct
BEGIN_NODEFAULT_CLASS_RTTI( SIDActorDefinition )
	PROPERTY( m_interlocutorId )
	PROPERTY( m_voiceTagToMatch )
	PROPERTY( m_tagToMatch )
END_CLASS_RTTI()

//------------------------------------------------------------------------------------------------------------------
// Dialog resource (a definition of the dialog)
//------------------------------------------------------------------------------------------------------------------
class CInteractiveDialog : public CResource, public ILocalizableObject
{	
	DECLARE_ENGINE_RESOURCE_CLASS( CInteractiveDialog, CResource, "idialog", "Interactive dialog" );

protected:
	TDynArray< SIDInterlocutorDefinition >	m_interlocutors;		//!< List of interlocutors
	TDynArray< CIDTopic* >					m_topics;				//!< List of topics
	IIDContition*							m_condition;			//!< Activator to allow the dialog itself to start
	Bool									m_autoFinish;			//!< Makes the dialog stop when no topic is playing

public:
	CInteractiveDialog();

	const CIDTopic* FindTopicByName( CName name ) const; 

	RED_INLINE const TDynArray< SIDInterlocutorDefinition >&	GetInterlocutorDefinitions()	const	{ return m_interlocutors;		} 
	RED_INLINE const TDynArray< CIDTopic* >&			GetTopics()						const	{ return m_topics;		} 
	RED_INLINE const IIDContition*					GetCondition()					const	{ return m_condition;	} 
	RED_INLINE const Bool								IsAutoFinish()					const	{ return m_autoFinish;	}

	CName GetVoiceTagByInterlocutorId( CName interlocutorId ) const;

	virtual void OnPostLoad();
	virtual void OnPreSave();
	virtual Bool OnPropertyMissing( CName propertyName, const CVariant& readValue );

	// ILocalizableObject interface
	virtual void GetLocalizedStrings( TDynArray< LocalizedStringEntry >& localizedStrings ) /*const */;

	class TopicConstIterator : public Red::System::NonCopyable
	{
	private:
		const CInteractiveDialog*	m_dialog;
		Uint32						m_currentTopic;

	public:
		RED_INLINE TopicConstIterator( const CInteractiveDialog* dialog ) : m_dialog( dialog ), m_currentTopic( 0 ) {}
		RED_INLINE const CIDTopic* operator*() const { return m_currentTopic < m_dialog->m_topics.Size() ? m_dialog->m_topics[ m_currentTopic ] : NULL; }
		RED_INLINE operator Bool() const { return m_currentTopic < m_dialog->m_topics.Size(); }
		RED_INLINE void operator++() { ++m_currentTopic; }
	};

	template < class TBlockType > class BlockConstIterator : public Red::System::NonCopyable
	{
	private:
		TopicConstIterator	m_it;
		Int32				m_currentBlock;

	public:
		RED_INLINE BlockConstIterator( const CInteractiveDialog* dialog ) : m_it( dialog ), m_currentBlock( -1 ) { Next(); }
		RED_INLINE const TBlockType* operator*() const { return m_it ? Cast< const TBlockType > ( ( *m_it )->GetGraph()->GraphGetBlocks()[ m_currentBlock ] ) : NULL; }
		RED_INLINE operator Bool() const { return m_it; }
		RED_INLINE void operator++() { Next(); }

	private:
		RED_INLINE void Next()
		{
			for ( ; m_it; ++m_it )
			{
				while ( ++m_currentBlock < ( *m_it )->GetGraph()->GraphGetBlocks().SizeInt() )
				{
					if ( ( *m_it )->GetGraph()->GraphGetBlocks()[ m_currentBlock ]->IsA< TBlockType > () )
					{
						return;
					}
				}
				m_currentBlock = -1;
			}
		}
	};

#ifndef NO_EDITOR
	class TopicIterator : public Red::System::NonCopyable
	{
	private:
		CInteractiveDialog* m_dialog;
		Uint32				m_currentTopic;

	public:
		RED_INLINE TopicIterator( CInteractiveDialog* dialog ) : m_dialog( dialog ), m_currentTopic( 0 ) {}
		RED_INLINE CIDTopic* operator*() { return m_currentTopic < m_dialog->m_topics.Size() ? m_dialog->m_topics[ m_currentTopic ] : NULL; }
		RED_INLINE operator Bool() const { return m_currentTopic < m_dialog->m_topics.Size(); }
		RED_INLINE void operator++() { ++m_currentTopic; }
	};

	template < class TBlockType > class BlockIterator : public Red::System::NonCopyable
	{
	private:
		TopicIterator	m_it;
		Int32			m_currentBlock;

	public:
		RED_INLINE BlockIterator( CInteractiveDialog* dialog ) : m_it( dialog ), m_currentBlock( -1 ) { Next(); }
		RED_INLINE TBlockType* operator*() { return m_it ? Cast< TBlockType > ( ( *m_it )->GetGraph()->GraphGetBlocks()[ m_currentBlock ] ) : NULL; }
		RED_INLINE operator Bool() const { return m_it; }
		RED_INLINE void operator++() { Next(); }

	private:
		RED_INLINE void Next()
		{
			for ( ; m_it; ++m_it )
			{
				while ( ++m_currentBlock < ( *m_it )->GetGraph()->GraphGetBlocks().SizeInt() )
				{
					if ( ( *m_it )->GetGraph()->GraphGetBlocks()[ m_currentBlock ]->IsA< TBlockType > () )
					{
						return;
					}
				}
				m_currentBlock = -1;
			}
		}
	};

	void SwapTopics( Uint32 currPosition, Uint32 newPosition );
	CIDTopic* NewTopic( CName topicName, Uint32 topicIndex );
	void RemoveTopic( Uint32 topicIndex );
	void AddInterlocutor( const SIDInterlocutorDefinition& def );
#endif // ifndef NO_EDITOR
};


BEGIN_CLASS_RTTI( CInteractiveDialog )
	PARENT_CLASS( CResource )	
	PROPERTY_EDIT( m_autoFinish, TXT("If true, the dialog will end when no topics are active") );
	PROPERTY_EDIT( m_interlocutors, TXT("") );
	PROPERTY_INLINED( m_topics, TXT("") );
END_CLASS_RTTI()
