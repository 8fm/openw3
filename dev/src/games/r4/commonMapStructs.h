#include "build.h"

struct SCommonMapPinInstance
{
	DECLARE_RTTI_STRUCT( SCommonMapPinInstance );

	Int32							m_id;
	CName							m_tag;
	Int32							m_customNameId;
	CName							m_extraTag;
	CName							m_type;
	CName							m_visibleType;
	Int32							m_alternateVersion;
	Vector							m_position;
	Float							m_radius;
	Float							m_visibleRadius;
	CGUID							m_guid;

	TDynArray< THandle< CEntity > > m_entities;
	Bool							m_isDynamic;
	Bool							m_isKnown;
	Bool							m_isDiscovered;
	Bool							m_isDisabled;
	Bool							m_isHighlightable;
	Bool							m_isHighlighted;
	Bool							m_canBePointedByArrow;
	Bool							m_canBeAddedToMinimap;
	Bool							m_isAddedToMinimap;
	Bool							m_invalidated;
#ifndef NO_SECOND_SCREEN
	Bool							m_usedBySecondScreen;
#endif
public:
	SCommonMapPinInstance()
		: m_id( 0 )
		, m_alternateVersion( 0 )
		, m_customNameId( 0 )
		, m_position( Vector::ZEROS )
		, m_radius( 0 )
		, m_visibleRadius( 0 )
		, m_guid( CGUID::ZERO )
		, m_isDynamic( false )
		, m_isKnown( false )
		, m_isDiscovered( false )
		, m_isDisabled( false )
		, m_isHighlightable( false )
		, m_isHighlighted( false )
		, m_canBePointedByArrow( false )
		, m_canBeAddedToMinimap( true )
		, m_isAddedToMinimap( false )
		, m_invalidated( false )
#ifndef NO_SECOND_SCREEN
		, m_usedBySecondScreen( false )
#endif
	{}

	Bool IsValid();
	Bool GetEntityPosition( Vector& pos ) const;

#ifndef NO_SECOND_SCREEN
	void AddToSecondScreen();
	void MoveOnSecondScreen();
	void UpdateOnSecondScreen();
#endif
};

BEGIN_CLASS_RTTI( SCommonMapPinInstance );
PROPERTY( m_id );
PROPERTY( m_tag );
PROPERTY( m_customNameId );
PROPERTY( m_extraTag );
PROPERTY( m_type );
PROPERTY( m_visibleType );
PROPERTY( m_alternateVersion );
PROPERTY( m_position );
PROPERTY( m_radius );
PROPERTY( m_visibleRadius )
PROPERTY( m_guid );
PROPERTY( m_entities );
PROPERTY( m_isDynamic );
PROPERTY( m_isKnown );
PROPERTY( m_isDiscovered );
PROPERTY( m_isDisabled );
PROPERTY( m_isHighlightable );
PROPERTY( m_isHighlighted );
PROPERTY( m_canBePointedByArrow );
PROPERTY( m_canBeAddedToMinimap );
PROPERTY( m_isAddedToMinimap );
PROPERTY( m_invalidated );
END_CLASS_RTTI();