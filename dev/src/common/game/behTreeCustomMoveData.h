#pragma once

#include "aiStorage.h"
#include "..\core\math.h"

class CBehTreeInstance;

class CBehTreeCustomMoveData
{
	DECLARE_RTTI_SIMPLE_CLASS( CBehTreeCustomMoveData );
protected:
	Vector						m_target;
	Float						m_heading;
public:
	CBehTreeCustomMoveData()					
		: m_target( Vector::ZEROS) 
		, m_heading( 0.0f )													{}

	const Vector&				GetTarget() const							{ return m_target; }
	Float						GetHeading() const							{ return m_heading; }
	void						SetTarget( const Vector& vec )				{ m_target = vec; }
	void						SetTarget( Float x, Float y, Float z)		{ m_target.X = x; m_target.Y = y; m_target.Z = z; }
	void						SetHeading( Float heading )					{ m_heading = heading; }

	static CName				DefaultStorageName()						{ return CNAME( CustomMoveData ); }

	class CInitializer : public CAIStorageItem::CInitializer
	{
	public:
		CInitializer()
			: CAIStorageItem::CInitializer()								{}
		void InitializeItem( CAIStorageItem& item ) const override;
		IRTTIType* GetItemType() const override;
		CName GetItemName() const override;
	};
};

BEGIN_NODEFAULT_CLASS_RTTI( CBehTreeCustomMoveData );
END_CLASS_RTTI();

class CBehTreeCustomMoveDataPtr : public TAIStoragePtr< CBehTreeCustomMoveData >
{
	typedef TAIStoragePtr< CBehTreeCustomMoveData > Super;
public:
	CBehTreeCustomMoveDataPtr( CAIStorage* storage );

	CBehTreeCustomMoveDataPtr()
		: Super()															{}
	CBehTreeCustomMoveDataPtr( const CBehTreeCustomMoveDataPtr& p )
		: Super( p )														{}

	CBehTreeCustomMoveDataPtr( CBehTreeCustomMoveDataPtr&& p )
		: Super( Move( p ) )												{}
};