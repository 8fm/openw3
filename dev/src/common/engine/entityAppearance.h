/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "entityTemplateParams.h"


class IGameSaver;
class IGameLoader;

/// Reference to component
struct CComponentReference
{
	DECLARE_RTTI_STRUCT( CComponentReference );

private:
	String  m_name;					//!< Name of the component
	CName   m_className;			//!< Component class

public:
	//! Get name of the referenced component
	RED_INLINE const String& GetName() const { return m_name; }

	//! Get name of the class of the component
	RED_INLINE const CName& GetNameOfClass() const { return m_className; }

public:
	//! Default constructor
	RED_INLINE CComponentReference()
	{}

	//! Initialize from component
	CComponentReference( const CComponent& component );
	
	//! Is this a reference to given component ?
	Bool IsReferenceTo( const CComponent &component ) const;

public:
	//! Compare
	RED_INLINE bool operator==( const CComponentReference& entry ) const
	{
		return m_name == entry.m_name && m_className == entry.m_className;
	}
};

BEGIN_CLASS_RTTI( CComponentReference );
	PROPERTY( m_name );
	PROPERTY( m_className );
END_CLASS_RTTI();

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// Body part state - a group of components in the body part
struct CEntityBodyPartState
{
	DECLARE_RTTI_STRUCT( CEntityBodyPartState );

protected:
	CName								m_name;							//!< Name of the state
	TDynArray< CComponentReference >	m_componentsInUse;				//!< Included components

public:
	//! Get name of the body part state
	RED_INLINE const CName& GetName() const { return m_name; }

	//! Get referenced components
	RED_INLINE const TDynArray< CComponentReference > & GetComponentsInUse() const { return m_componentsInUse; }

	//! Get n-th component
	RED_INLINE CComponentReference &GetComponentReference( Uint32 index ) { return m_componentsInUse[ index ]; }

public:
	//! Default constructor
	RED_INLINE CEntityBodyPartState()
	{}

	//! Create named body part
	RED_INLINE CEntityBodyPartState( const CName& name )
		: m_name( name )
	{}

	//! Change name of the body part state
	void SetName( const CName & name );

	//! Does this body part state uses given component ?
	Bool UseComponent( const CComponent* component ) const;

	//! Add component to the body part state
	void AddComponent( const CComponent& component );

	//! Remove component from the body part state
	void RemoveComponent( const CComponent& component );

	//! Remove component by index from the body part state
	void RemoveComponent( Uint32 index );

public:
	//! Compare
	RED_INLINE bool operator==( const CEntityBodyPartState& entry ) const
	{
		return m_name == entry.m_name && m_componentsInUse == entry.m_componentsInUse;
	}
};

BEGIN_CLASS_RTTI( CEntityBodyPartState );
	PROPERTY( m_name )
	PROPERTY( m_componentsInUse );
END_CLASS_RTTI();

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// Body part
struct CEntityBodyPart
{
	DECLARE_RTTI_STRUCT( CEntityBodyPart );

protected:
	CName								m_name;
	TDynArray< CEntityBodyPartState >	m_states;
	Bool								m_wasIncluded;

public:
	//! Get the name of this body part
	RED_INLINE const CName& GetName() const { return m_name; }

	//! Was this body part included ?
	RED_INLINE Bool WasIncluded() const { return m_wasIncluded; }

	//! Get list of body part states
	RED_INLINE const TDynArray< CEntityBodyPartState > & GetStates() const { return m_states; }

	//! Get state
	RED_INLINE CEntityBodyPartState &GetState( Uint32 index ) { return m_states[ index ]; }

public:
	//! Default constructor
	CEntityBodyPart();

	// Merging constructor, using during entity template merge
	CEntityBodyPart( const CEntityBodyPart& source, CEntityTemplate* baseTemplate );

	//! Add body part state to this body part
	void AddState( CEntityBodyPartState& state );

	//! Remove body part state from this body part
	void RemoveState( CEntityBodyPartState& state );

	//! Change name of this body part
	void SetName( const CName &name );

	//! Mark this body part as included from some base template
	void SetWasIncluded();

public:
	//! Compare
	RED_INLINE bool operator==( const CEntityBodyPart& entry ) const
	{
		return m_name == entry.m_name && m_states == entry.m_states;
	}
};

BEGIN_CLASS_RTTI( CEntityBodyPart );
	PROPERTY( m_name )
	PROPERTY( m_states );
	PROPERTY( m_wasIncluded );
END_CLASS_RTTI();

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// Entity appearance definition
struct CEntityAppearance
{
	DECLARE_RTTI_STRUCT( CEntityAppearance );

protected:
	CName									m_name;					//!< Name of the appearance
	CName									m_voicetag;				//!< Voicetag of this appearance
	TDynArray< CEntityTemplateParam* >		m_appearanceParams;		//!< Custom gameplay parameters defined only for this apperance
	Bool									m_wasIncluded;			//!< Was this apperance included from base template ?
	Bool									m_useVertexCollapse;	//!< Use ear vertex collapsing in this apperance. OBSOLETE AND NO LONGER USED. Just leaving in so data doesn't change from removing a property!
	Bool									m_usesRobe;				//!< A robed appearance will have special considerations when using an AP
	TDynArray< THandle< CEntityTemplate > >	m_includedTemplates;
	TDynArray<CName>						m_collapsedComponents;

public:
	//! Get the name of this appearance
	RED_INLINE const CName& GetName() const { return m_name; }

	//! Get the voicetag
	RED_INLINE const CName& GetVoicetag() const { return m_voicetag; }

	//! Was this appearance included
	RED_INLINE Bool WasIncluded() const { return m_wasIncluded; }

	//! Get additional parameters of the appearance
	RED_INLINE const TDynArray< CEntityTemplateParam* > GetAppearanceParams() const { return m_appearanceParams; }

	//! Returns whether the appearance uses a robe
	RED_INLINE Bool GetUsesRobe() const { return m_usesRobe; }

	//!< Return the currently included entity templates
	RED_INLINE const TDynArray< THandle<CEntityTemplate> >& GetIncludedTemplates() const { return m_includedTemplates; };

	//!< Return the currently collapsed component names (all of them exist in the included templates)
	RED_INLINE const TDynArray<CName>& GetCollapsedComponents() const { return m_collapsedComponents; };

public:
	//! Default constructor
	RED_INLINE CEntityAppearance() 
		: m_wasIncluded( false )
		, m_useVertexCollapse( false )
	{}

	//! Constructor
	RED_INLINE CEntityAppearance( const CName &name )
		: m_name( name )
		, m_wasIncluded( false )
		, m_useVertexCollapse( false )
	{}

	//! Merging constructor, used in entity template merge
	CEntityAppearance( const CEntityAppearance& source, CEntityTemplate* baseTemplate );

	//! Change name of the appearance
	void SetName( const CName& name );

	//! Change the voicetag of this appearance
	void SetVoicetag( const CName & voicetag );

	//! Sets whether the appearance uses a robe
	void SetUsesRobe( Bool usesRobe ) { m_usesRobe = usesRobe; }

	//! Mark this appearance as included
	void SetWasIncluded();

	//! Adds a new template parameter
	Bool AddParameter( CEntityTemplateParam* param );

	//! Removes new template parameter
	Bool RemoveParameter( CEntityTemplateParam* param );

	//! Includes the given template
	void IncludeTemplate( CEntityTemplate* appearanceTemplate );

	//! Removes the given template
	void RemoveTemplate( CEntityTemplate* appearanceTemplate );

	bool ValidateIncludedTemplates(const CResource* resource);

	//! Adds the components in the given template to the collapsed components list
	void CollapseTemplate( CEntityTemplate* appearanceTemplate );

	//! Removes the components in given template from the collapsed components list
	void DecollapseTemplate( CEntityTemplate* appearanceTemplate );

	//! Returns true if the given component is in the collapsed components list
	Bool IsCollapsed( CComponent* component ) const;

public:
	//! Find parameter by type
	template< class T >	T* FindParameter() const
	{
		// Search by type
		Uint32 count = m_appearanceParams.Size();
		for ( Uint32 i = 0; i < count; ++i )
		{
			CEntityTemplateParam* param = m_appearanceParams[i];
			ASSERT ( param );

			if ( param && param->IsA< T >() )
			{
				return Cast< T >( param );
			}
		}

		// Not found
		return NULL;
	}

public:
	//! Compare
	RED_INLINE bool operator==( const CEntityAppearance& entry ) const
	{
		return m_name == entry.m_name && m_includedTemplates == entry.m_includedTemplates;
	}
};

BEGIN_CLASS_RTTI( CEntityAppearance );
	PROPERTY( m_name );
	PROPERTY( m_voicetag );
	PROPERTY( m_appearanceParams );
	PROPERTY( m_useVertexCollapse );
	PROPERTY( m_usesRobe );
	PROPERTY( m_wasIncluded );
	PROPERTY( m_includedTemplates );
	PROPERTY( m_collapsedComponents );
END_CLASS_RTTI();

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// Runtime body part list
class CMBodyPartStates
{
private:
	struct BodyPart
	{
		CName	m_part;			//!< Body part name
		CName	m_state;		//!< Selected body part state
	};

private:
	TDynArray< BodyPart >*		m_bodyPartStates;

public:
	CMBodyPartStates();
	~CMBodyPartStates();

	//! Get body part state for given body part
	CName Get( const CName& bodyPartName ) const;

	//! Set body part state for given body part
	void Set( const CName& bodyPartName, const CName& bodyPartStateName );

public:
	//! Save state
	void SaveState( IGameSaver* saver );

	//! Restore state
	void RestoreState( IGameLoader* loader );
};
