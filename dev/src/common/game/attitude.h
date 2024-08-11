/**
 * Copyright © 2009 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "../core/intsBitField.h"
#include "../core/2darray.h"

#define ATTITUDE_GROUPS_CSV TXT("gameplay\\globals\\attitude_groups.csv")
#define ATTITUDES_XML TXT("gameplay\\globals\\attitudes.xml")

enum EAIAttitude
{
	AIA_Friendly,
	AIA_Neutral,
	AIA_Hostile,
};

BEGIN_ENUM_RTTI( EAIAttitude )
	ENUM_OPTION( AIA_Friendly );
	ENUM_OPTION( AIA_Neutral );
	ENUM_OPTION( AIA_Hostile );
END_ENUM_RTTI()

template<> RED_INLINE String ToString( const EAIAttitude& value )
{
	switch ( value )
	{
	case AIA_Friendly:
		return TXT( "Friendly" );
	case AIA_Neutral:
		return TXT( "Neutral" );
	case AIA_Hostile:
		return TXT( "Hostile" );
	default:
		ASSERT( !TXT( "ToString(): Unknown attitude type." ) );
		return TXT( "Unknown attitude" );
	}
}

// Dummy struct, needed so EAIAttitude will be visible for scripts when compiled without penthouse
struct SAIAttitudeDummy
{
	EAIAttitude m_atttitude;
	DECLARE_RTTI_STRUCT( SAIAttitudeDummy );
};

BEGIN_CLASS_RTTI( SAIAttitudeDummy )
	PROPERTY( m_atttitude );
END_CLASS_RTTI()

class IAttitudesManager
{
public:

	virtual Bool SetAttitude( const CName& srcGroup, const CName& dstGroup, EAIAttitude attitude, bool updateTree ) = 0;
	virtual Bool GetAttitude( const CName& srcGroup, const CName& dstGroup, EAIAttitude &attitude, Bool& isCustom ) const = 0;
	virtual Bool GetAttitudeWithParents( const CName& srcGroup, const CName& dstGroup, EAIAttitude &attitude, Bool& isCustom,
				                         CName& srcGroupParent, CName& dstGroupParent ) const = 0;
	virtual Bool RemoveAttitude( const CName& srcGroup, const CName& dstGroup, bool updateTree ) = 0;

	virtual Bool GetParentForGroup( const CName& group, CName& parent ) const = 0;
	virtual Bool SetParentForGroup( const CName& group, const CName& parent, bool updateTree ) = 0;
	virtual Bool RemoveParentForGroup( const CName& group, bool updateTree ) = 0;
	virtual Bool IsParentForGroup( const CName& child, const CName& parent ) const = 0;
	virtual Bool GetAllParents( const CName& group, TDynArray< CName > & parents ) const = 0;
	virtual Bool GetAllChildren( const CName& group, TDynArray< CName > & children ) const = 0;

	virtual Bool CanAttitudeCreateConflict( const CName& srcGroup, const CName& dstGroup, CName& srcConflictGroup, CName& dstConflictGroup ) = 0;
	virtual Bool CanParenthoodCreateConflict( const CName& child, const CName& parent, CName& childConflictGroup, CName& parentConflictGroup ) = 0;
};

class CAttitudesResourcesManager: public C2dArraysResourcesManager
{
public:
	CAttitudesResourcesManager();
		
#ifndef NO_EDITOR
	Uint32 GetAttitudeGroupsFormFile( const String& pathToGroupsFile, TDynArray< String > & attitudeGroups );
	void Sync();
#endif // !NO_EDITOR

};

typedef TSingleton< CAttitudesResourcesManager, TDefaultLifetime, TCreateUsingNew > SAttitudesResourcesManager;

class CAttitudes : public IAttitudesManager
{
public:
	static EAIAttitude GetDefaultAttitude();
	static EAIAttitude AttitudeFromString( const String& attitude );

	// Attitudes
	Bool SetAttitude( const CName& srcGroup, const CName& dstGroup, EAIAttitude attitude, bool updateTree ) override;
	Bool GetAttitude( const CName& srcGroup, const CName& dstGroup, EAIAttitude &attitude, Bool& isCustom ) const override;
	Bool GetAttitudeWithParents( const CName& srcGroup, const CName& dstGroup, EAIAttitude &attitude, Bool& isCustom,
								 CName& srcGroupParent, CName& dstGroupParent ) const override;
	Uint32 GetAttitudes( TDynArray< CName > & srcGroups, TDynArray< CName > & dstGroups, TDynArray< EAIAttitude > & attitudes ) const;
	Bool RemoveAttitude( const CName& srcGroup, const CName& dstGroup, bool updateTree ) override;
	void ClearAllAttitudes();
	Bool IsCustomAttitude( const CName& srcGroup, const CName& dstGroup ) const;

	// Groups
	Uint32 GetAttitudeGroups( TDynArray< CName > & attitudeGroups );
	Bool AttitudeGroupExists( const CName& group ) const;

	// Group parents
	Bool GetParentForGroup( const CName& group, CName& parent ) const override;
	Bool SetParentForGroup( const CName& group, const CName& parent, bool updateTree ) override;
	Bool RemoveParentForGroup( const CName& group, bool updateTree ) override;
	Uint32 GetParentGroups( TDynArray< CName > & groups , TDynArray< CName > & parents ) const;
	Bool IsParentForGroup( const CName& child, const CName& parent ) const override;
	Bool GetAllParents( const CName& group, TDynArray< CName > & parents ) const override;
	Bool GetAllChildren( const CName& group, TDynArray< CName > & children ) const override;

	// Ambiguity
	Bool CanAttitudeCreateConflict( const CName& srcGroup, const CName& dstGroup, CName& srcConflictGroup, CName& dstConflictGroup ) override;
	Bool CanParenthoodCreateConflict( const CName& child, const CName& parent, CName& childConflictGroup, CName& parentConflictGroup ) override;

	void InitAttitudesTree();
	Uint32 RemoveUnusedGroups( TDynArray< String > & groups );

	// XML load/save
	Bool LoadDataFromXml( const String& filePath );
	Bool LoadDataFromXmls( const TDynArray<String>& filePath );

	Bool SaveDataToXml( CXMLWriter* writer );
	Bool LoadAttitudeGroups();
	Bool AttitudeGroupsLoaded() const;

protected:

	struct AttitudeGroup
	{
		CName						m_name;
		Uint32						m_index;
		AttitudeGroup*				m_parent;
		TDynArray< AttitudeGroup *>	m_children;

		AttitudeGroup() : m_name( CName::NONE ), m_index( 0 ), m_parent( NULL ) { }
		AttitudeGroup( const CName& name, Uint32 index ) : m_name( name ), m_index( index ), m_parent( NULL ) {}
	};

	TIntsBitField<1>				m_attitudes;
	TIntsBitField<0>				m_isCustomAttitude;
	THashMap< CName, AttitudeGroup >	m_attitudeGroups;

	static const Uint32	WRONG_INDEX;

	Bool LoadDataFromXml( CXMLReader* reader );

	Bool IsParentForGroup( const AttitudeGroup* child, const AttitudeGroup* parent ) const;
	Bool GetAllParents( const AttitudeGroup* group, TDynArray< AttitudeGroup* > & parents ) const;
	Bool GetAllChildren( const AttitudeGroup* group, TDynArray< const AttitudeGroup* > & children ) const;
	Bool GetInheritanceRoot( const CName& srcGroup, const CName& dstGroup,AttitudeGroup*& srcGroupParent, AttitudeGroup*& dstGroupParent ) const;
	void UpdateChildrenAttitude( AttitudeGroup* baseGroup, AttitudeGroup* otherGroup, EAIAttitude attitude );
	void UpdateChildrenAttitudes(AttitudeGroup* baseGroup );
	void UpdateAttitudesTree( AttitudeGroup* baseGroup );
	void CopyInheritedAttitudes( AttitudeGroup* group, AttitudeGroup* parent );
	void ResetChildrenAttitude( AttitudeGroup* baseGroup, AttitudeGroup* otherGroup );
	void ResetChildrenAttitudes( AttitudeGroup* baseGroup );
	void ResetInheritedAttributes( AttitudeGroup* group );
	Uint32 GetCacheIndex( const CName& srcGroup, const CName& dstGroup ) const;
	Uint32 GetCacheIndex( Uint32 srcGroupIndex, Uint32 dstGroupIndex ) const;
	Uint32 GetCacheSize() const;
};

class CAIProfile;

struct SAttitudesAIProfilePred
{
	Bool operator()( CAIProfile *aiProfile ) const;
};
