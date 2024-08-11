// Copyright © 2014 CD Projekt Red. All Rights Reserved.

#pragma once

typedef Uint32 CStorySceneSectionVariantId;

class CStorySceneSectionVariantElementInfo
{
public:
	CStorySceneSectionVariantElementInfo();
	~CStorySceneSectionVariantElementInfo();
	
	// compiler generated cctor is ok
	// compiler generated op= is ok

	String m_elementId;
	Float m_approvedDuration;

	DECLARE_RTTI_STRUCT( CStorySceneSectionVariantElementInfo )
};

BEGIN_CLASS_RTTI( CStorySceneSectionVariantElementInfo )
	PROPERTY( m_elementId )
	PROPERTY( m_approvedDuration )
END_CLASS_RTTI();

class CStorySceneSectionVariant
{
public:
	CStorySceneSectionVariant();
	~CStorySceneSectionVariant();

	// compiler generated cctor is ok
	// compiler generated op= is ok

	Float GetApprovedDuration( const String& elementId ) const;

	void InsertElement( const CStorySceneElement* element, Uint32 index = -1 );
	void RemoveElement( const String& elementId );

	CStorySceneSectionVariantId m_id;
	Uint32 m_localeId;
	TDynArray< CGUID > m_events;										// Events belonging to this variant. CStorySceneSectionVariant doesn't own them (section is the owner).

	TDynArray< CStorySceneSectionVariantElementInfo > m_elementInfo;	// Note that it's not true that i-th element corresponds to i-th element of m_sceneElements (that's because section keeps choice separately while we keep it together we other elements).
	THashMap< String, Float > m_elementIdToApprovedDuration;			// Maps element ID to approved duration. Choice element is also here (if one exists).

	// TODO: Consider adding:
	// - variant name (editor only),
	// - name of user that updated this variant (editor only),
	// - time when this variant was updated (editor only).

	DECLARE_RTTI_STRUCT( CStorySceneSectionVariant )
};

BEGIN_CLASS_RTTI( CStorySceneSectionVariant )
	PROPERTY( m_id )
	PROPERTY( m_localeId )
	PROPERTY( m_events )
	PROPERTY( m_elementInfo )
END_CLASS_RTTI();

/*
Ctor.
*/
RED_INLINE CStorySceneSectionVariantElementInfo::CStorySceneSectionVariantElementInfo()
: m_approvedDuration( -1.0f )
{}

/*
Dtor.
*/
RED_INLINE CStorySceneSectionVariantElementInfo::~CStorySceneSectionVariantElementInfo()
{}

/*
Ctor.
*/
RED_INLINE CStorySceneSectionVariant::CStorySceneSectionVariant()
: m_id( -1 )
, m_localeId( -1 )
{}

/*
Dtor.
*/
RED_INLINE CStorySceneSectionVariant::~CStorySceneSectionVariant()
{}
