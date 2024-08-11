#pragma once

#ifndef NO_LOG
	#define JOURNAL_LOG( format, ... ) RED_LOG( Journal, format, ## __VA_ARGS__ )
#else
	#define JOURNAL_LOG( ... )
#endif

#define JOURNAL_STRINGDB_PROPERTY_NAME TXT( "Journal" )

//-----------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------

class CJournalBase : public CObject
{
	DECLARE_ENGINE_ABSTRACT_CLASS( CJournalBase, CObject )

public:
	CJournalBase();
	virtual ~CJournalBase();

	// Initialisers
	void Initialize( Uint32 order, const Char* baseName = NULL );

	// Accessors
	const CGUID&		GetGUID() const									{ return m_guid; }
	void				SetOrder( Uint32 order )						{ m_order = order; }
	Uint32				GetOrder() const								{ return m_order; }
	const String&		GetName() const									{ return m_baseName; }
	void				SetUniqueScriptIdentifier( const CName& tag )	{ m_uniqueScriptIdentifier = tag; }
	const CName&		GetUniqueScriptIdentifier() const				{ return m_uniqueScriptIdentifier; }
	Bool				IsActive() const								{ return m_active; }

	virtual Bool IsParentClass( CJournalBase* other ) const = 0;

	void				RecreateGUID()						{ m_guid = CGUID::Create(); }

	template< typename TClass >
	RED_INLINE const TClass* GetParentAs() const
	{
		CObject* parentObject = GetParent();
		ASSERT( parentObject != NULL, TXT( "Journal Entry %s ('%ls') has no parent" ), GetClass()->GetName().AsString().AsChar(), m_baseName.AsChar() );
		ASSERT( parentObject->IsA< TClass >(), TXT( "Journal Entry %s ('%ls') is parented to a '%ls', not a '%ls'" ), GetClass()->GetName().AsString().AsChar(), m_baseName.AsChar(), parentObject->GetClass()->GetName().AsString().AsChar(), ClassID< TClass >()->GetName().AsString().AsChar() );
		return static_cast< TClass* >( parentObject );
	}

	template< typename TClass >
	RED_INLINE TClass* GetParentAs()
	{
		return const_cast< TClass* >( static_cast< const CJournalBase* >( this )->GetParentAs< TClass >() );
	}

	void funcGetUniqueScriptTag( CScriptStackFrame& stack, void* result );
	void funcGetOrder( CScriptStackFrame& stack, void* result );

	void funcIsActive( CScriptStackFrame& stack, void* result );

protected:
	CGUID m_guid;
	String m_baseName;
	Uint32 m_order;
	CName m_uniqueScriptIdentifier;
	Bool m_active;
};

RED_DECLARE_NAME( baseName );

BEGIN_ABSTRACT_CLASS_RTTI( CJournalBase )
	PARENT_CLASS( CObject )
	PROPERTY_RO( m_guid, TXT( "GUID" ) )
	PROPERTY_EDIT( m_baseName, TXT( "Editor Name" ) )
	PROPERTY( m_order )
	PROPERTY_EDIT( m_uniqueScriptIdentifier, TXT( "Unique Script Tag" ) )
	NATIVE_FUNCTION( "GetUniqueScriptTag", funcGetUniqueScriptTag )
	NATIVE_FUNCTION( "GetOrder", funcGetOrder )
	NATIVE_FUNCTION( "IsActive", funcIsActive )
END_CLASS_RTTI()

//-----------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------

class CJournalChildBase : public CJournalBase
{
	DECLARE_ENGINE_ABSTRACT_CLASS( CJournalChildBase, CJournalBase )

public:
	CJournalChildBase();
	virtual ~CJournalChildBase();

	void Initialize( const CGUID& parentGuid, Uint32 order, const Char* baseName = NULL );
	
	void SetParentGUID( const CGUID& parentGuid ) { m_parentGuid = parentGuid; }
	const CGUID& GetParentGUID() const { return m_parentGuid; }
	void SetLinkedParentGUID( const CGUID& linkedParentGuid ) { m_linkedParentGuid = linkedParentGuid; }
	const CGUID& GetLinkedParentGUID() const
	{
		if ( !m_linkedParentGuid.IsZero() )
		{
			return m_linkedParentGuid;
		}
		return m_parentGuid;
	}

protected:
	virtual void DefaultValues() {}

protected:
	CGUID m_parentGuid;

	//! in case parent object is of CJournalLink type, we store "linked" parent GUID there
	CGUID m_linkedParentGuid;

private:

	void funcGetLinkedParentGUID( CScriptStackFrame& stack, void* result );
};

BEGIN_ABSTRACT_CLASS_RTTI( CJournalChildBase )
	PARENT_CLASS( CJournalBase )
	PROPERTY_RO( m_parentGuid, TXT( "Parent GUID" ) )
	PROPERTY_RO( m_linkedParentGuid, TXT( "Linekd parent GUID" ) )
	NATIVE_FUNCTION( "GetLinkedParentGUID", funcGetLinkedParentGUID )
END_CLASS_RTTI()

//-----------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------

class CJournalContainerEntry : public CJournalChildBase
{
	DECLARE_ENGINE_ABSTRACT_CLASS( CJournalContainerEntry, CJournalChildBase )

public:
	CJournalContainerEntry();
	virtual ~CJournalContainerEntry();

	Uint8 GetIndex() { return m_index; }
	void SetIndex( Uint8 index ) { m_index = index; }

private:
	Uint8 m_index;
};

BEGIN_ABSTRACT_CLASS_RTTI( CJournalContainerEntry )
	PARENT_CLASS( CJournalChildBase )
	PROPERTY( m_index )
END_CLASS_RTTI()

//-----------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------

class CJournalContainer : public CJournalContainerEntry
{
	DECLARE_ENGINE_ABSTRACT_CLASS( CJournalContainer, CJournalContainerEntry )

public:
	CJournalContainer();
	virtual ~CJournalContainer();

	void AddChildEntry( CJournalContainerEntry* child );
	void RemoveChildEntry( CJournalContainerEntry* child );
	void RemoveChildEntry( Uint32 childIndex );

	RED_INLINE Uint32 GetNumChildren() const { return m_children.Size(); }

	RED_INLINE CJournalContainerEntry* GetChild( Uint32 index ) { return m_children[ index ]; }
	RED_INLINE const CJournalContainerEntry* GetChild( Uint32 index ) const { return m_children[ index ]; }

	template< typename TChildType >
	RED_INLINE const TChildType* GetFirstChildOfType() const
	{
		for( Uint32 i = 0; i < m_children.Size(); ++i )
		{
			if( m_children[ i ]->IsA< TChildType >() )
			{
				return static_cast< const TChildType* >( m_children[ i ] );
			}
		}

		return NULL;
	}

	template< typename TChildType >
	RED_INLINE TChildType* GetFirstChildOfType()
	{
		return const_cast< TChildType* >( static_cast< const CJournalContainer* >( this )->GetFirstChildOfType< TChildType >() );
	}

public:
	void funcGetChild( CScriptStackFrame& stack, void* result );
	void funcGetNumChildren( CScriptStackFrame& stack, void* result );

private:
	TDynArray< CJournalContainerEntry* > m_children;
};

BEGIN_ABSTRACT_CLASS_RTTI( CJournalContainer )
	PARENT_CLASS( CJournalContainerEntry )
	PROPERTY( m_children )
	NATIVE_FUNCTION( "GetChild", funcGetChild )
	NATIVE_FUNCTION( "GetNumChildren", funcGetNumChildren )
END_CLASS_RTTI()

//-----------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------

class CJournalLink : public CJournalBase
{
	DECLARE_ENGINE_ABSTRACT_CLASS( CJournalLink, CJournalBase )

public:
	CJournalLink();
	virtual ~CJournalLink();

	CJournalResource* GetLinkedResource();
	CJournalBase* GetLinkedObject();

	RED_INLINE void SetLinkedObjectPath( const String& path ) { m_linkedObjectPath = path; }
	RED_INLINE const String& GetLinkedObjectPath() const { return m_linkedObjectPath; }

protected:

	String m_linkedObjectPath;
};

BEGIN_ABSTRACT_CLASS_RTTI( CJournalLink )
	PARENT_CLASS( CJournalBase )
	PROPERTY_CUSTOM_EDIT( m_linkedObjectPath, TXT( "Path to the \"real\" journal entry" ), TXT( "LinkedObjectPath" ) );
END_CLASS_RTTI()
