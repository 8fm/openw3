/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "aiStorageExternalPtr.h"
#include "formationMemberData.h"

//class CAIFormationDataPtr : public TAIStorageExternalPtr< CFormationLeaderData >
//{
//	typedef TAIStorageExternalPtr< CFormationLeaderData > Super;
//protected:
//	CFormationMemberDataPtr						m_memberDataPtr;
// 
//public:
//	CAIFormationDataPtr()
//		: Super()																	{}
//	CAIFormationDataPtr( const CAIFormationDataPtr& p )
//		: Super( p )																{}
//	CAIFormationDataPtr( CAIFormationDataPtr&& p )
//		: Super( Move( p ) )														{}
//
//	RED_INLINE CAIFormationDataPtr& operator=( const CAIFormationDataPtr& ptr )	{ Super::operator=( ptr ); return *this; }
//	RED_INLINE CAIFormationDataPtr& operator=( CAIFormationDataPtr&& ptr )		{ Super::operator=( Move( ptr ) ); return *this; }
//
//	CFormationLeaderData* LeaderData() const										{ return Item(); }
//	CFormationMemberData* MemberData() const										{ return m_memberDataPtr; }
//
//	Bool Setup( IFormationLogic* logic, CActor* leader, CActor* member );
//	void Clear();
//};

class CFormationLeaderDataPtr : public TAIStoragePtr< CFormationLeaderData >
{
	typedef TAIStoragePtr< CFormationLeaderData > Super;
public:
	struct CInitializer : public CAIStorageItem::CNamedInitializer
	{
		CInitializer( IFormationLogic* logic, CActor* leader, CName uniqueName );

		void InitializeItem( CAIStorageItem& item ) const override;
		IRTTIType* GetItemType() const override;

		IFormationLogic*	m_logic;
		CActor*				m_leader;
	};

	CFormationLeaderDataPtr()
		: Super()																	{}

	Bool Setup( CAIStorage* aiStorage, CFormation* formation, CActor* leader );
};

typedef TAIStorageExternalPtr< CFormationLeaderData > CFormationLeaderDataExternalPtr;

class CAIFormationData : public CAIStorageItemVirtualInterface
{
	DECLARE_RTTI_SIMPLE_CLASS( CAIFormationData )
protected:
	CFormationLeaderDataExternalPtr				m_leaderData;
	CFormationMemberDataPtr						m_memberData;
	CFormation*									m_formation;

public:
	CAIFormationData()
		 : m_formation( NULL )														{}

	CFormationLeaderData*	LeaderData() const										{ return m_leaderData; }
	CFormationMemberData*	MemberData() const										{ return m_memberData.Get(); }
	CFormation*				GetFormation() const									{ return m_formation; }
	CBehTreeInstance*		GetExternalOwner() const								{ return m_leaderData.GetExternalOwner(); }

	Bool Test( CFormation* formation, CActor* leader );
	Bool Setup( CFormation* formation, CActor* leader, CActor* member, Bool force = true );
	Bool IsSetup() const															{ return m_leaderData; }
	void Clear();
	void Update( CBehTreeInstance* ai );

	void OnDetached() override;
};

BEGIN_CLASS_RTTI( CAIFormationData )
END_CLASS_RTTI()

class CAIFormationDataPtr : public TAIStoragePtr< CAIFormationData >
{
	typedef TAIStoragePtr< CAIFormationData > Super;
protected:
	struct CInitializer : public CAIStorageItemVirtualInterface::CInitializer
	{
		CName GetItemName() const override;
		IRTTIType* GetItemType() const;
	};
public:
	CAIFormationDataPtr( CAIStorage* storage );							

	CAIFormationDataPtr()
		: Super()															{}
	CAIFormationDataPtr( const CAIFormationDataPtr& p )
		: Super( p )														{}
	CAIFormationDataPtr( CAIFormationDataPtr&& p )
		: Super( Move( p ) )												{}

	RED_INLINE CAIFormationDataPtr& operator=( const CAIFormationDataPtr& ptr )		{ Super::operator=( ptr ); return *this; }
	RED_INLINE CAIFormationDataPtr& operator=( CAIFormationDataPtr&& ptr )			{ Super::operator=( Move( ptr ) ); return *this; }
};

