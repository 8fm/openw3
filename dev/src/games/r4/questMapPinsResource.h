#pragma once

struct SQuestMapPinInfo
{
	DECLARE_RTTI_STRUCT( SQuestMapPinInfo );

	CName	m_tag;
	CName	m_type;
	Float	m_radius;
	TDynArray< Vector > m_positions;

	SQuestMapPinInfo( const CName& tag = CName::NONE, const CName& type = CName::NONE, Float radius = 0 )
		: m_tag( tag )
		, m_type( type )
		, m_radius( radius )
	{}
};

BEGIN_CLASS_RTTI( SQuestMapPinInfo );
	PROPERTY_EDIT( m_tag, TXT( "Tag" ) );
	PROPERTY_EDIT( m_type, TXT( "Type" ) );
	PROPERTY_EDIT( m_radius, TXT( "Radius" ) );
	PROPERTY_EDIT( m_positions, TXT( "Positions" ) );
END_CLASS_RTTI();

class CQuestMapPinsResource : public CResource
{
	DECLARE_ENGINE_RESOURCE_CLASS( CQuestMapPinsResource, CResource, "w2qm", "Quest Mappins" );

public:
	CQuestMapPinsResource();
	~CQuestMapPinsResource();

#ifndef NO_EDITOR
	Bool AddEntry( const SQuestMapPinInfo& info );
#endif //NO_EDITOR

	RED_INLINE const TDynArray< SQuestMapPinInfo >& GetMapPinsInfo() const	{ return m_mappinsInfo; } 
	RED_INLINE TDynArray< SQuestMapPinInfo >& GetMapPinsInfo()				{ return m_mappinsInfo; } 
	RED_INLINE void ClearMapPinsInfo()										{ m_mappinsInfo.Clear(); } 

private:
	TDynArray< SQuestMapPinInfo > m_mappinsInfo;
};

BEGIN_CLASS_RTTI( CQuestMapPinsResource )
	PARENT_CLASS( CResource )
	PROPERTY_RO( m_mappinsInfo, TXT("Mappins info") )
END_CLASS_RTTI()