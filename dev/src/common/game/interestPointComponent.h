/**
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */
#pragma once


class CInterestPointInstance;
class IPotentialField;


///////////////////////////////////////////////////////////////////////////////

class CInterestPointInstance : public CObject
{
	DECLARE_ENGINE_CLASS( CInterestPointInstance, CObject, 0 );

	friend class CInterestPoint;

private:
	THandle< CInterestPoint >					m_parent;

	THandle< CNode >							m_parentNode;
	Vector										m_position;

	Float										m_timeToLive;

	Float										m_fieldStrengthMultiplier;
	Float										m_parameterFromTest;

public:
	CInterestPointInstance();
	~CInterestPointInstance();

	// Returns the parent point of this instance
	RED_INLINE const CInterestPoint* GetParentPoint() const { return m_parent.Get(); }

	//! Returns the name of the field the point emits
	const CName& GetName() const;

	// Returns the static position of the point.
	RED_INLINE const Vector& GetPosition() const { return m_position; }

	// Returns the node the point is attached to (if there's one)
	RED_INLINE const THandle< CNode >& GetNode() const { return m_parentNode; }

	// Get static or node position;
	Vector GetWorldPosition() const;

	// Get TTL
	Float GetTimeToLive() const { return m_timeToLive; }

	// Initializes the instance
	void Bind( const THandle< CInterestPoint >& parent, const THandle< CNode >& node, Float timeout );

	// Initializes the instance
	void Bind( const THandle< CInterestPoint >& parent, const Vector& position, Float timeout );

	// Reinitializes the instance
	void Rebind( const THandle< CNode >& node, Float timeout );

	// Reinitializes the instance
	void Rebind( const Vector& position, Float timeout );

	// Updates the instance, returning true to indicate that it has finished its work
	Bool Update( Float timeElapsed );

	// Range test
	Bool RangeTest( const Vector& pos ) const;

	// Strength of field in given range
	Float FieldStrength( const Vector& pos ) const;

	// Update instance float parameter
	void SetFieldStrengthMultiplier( Float parameter ){ m_fieldStrengthMultiplier = parameter; }
	// Update instance float parameter
	Float GetFieldStrengthMultiplier() const { return m_fieldStrengthMultiplier; }

	// Update instance float parameter
	void SetTestParameter( Float parameter ){ m_parameterFromTest = parameter; }
	// Update instance float parameter
	Float GetTestParameter() const { return m_parameterFromTest; }

	// -------------------------------------------------------------------------
	// Scripting support
	// -------------------------------------------------------------------------
private:	
	void funcGetParentPoint( CScriptStackFrame& stack, void* result );
	void funcGetWorldPosition( CScriptStackFrame& stack, void* result );
	void funcGetNode( CScriptStackFrame& stack, void* result );
	void funcGetGeneratedFieldName( CScriptStackFrame& stack, void* result );
	void funcGetFieldStrength( CScriptStackFrame& stack, void* result );
	void funcSetFieldStrengthMultiplier( CScriptStackFrame& stack, void* result );
	void funcGetFieldStrengthMultiplier( CScriptStackFrame& stack, void* result );
	void funcSetTestParameter( CScriptStackFrame& stack, void* result );
	void funcGetTestParameter( CScriptStackFrame& stack, void* result );
};
BEGIN_CLASS_RTTI( CInterestPointInstance );
	PARENT_CLASS( CObject );
	PROPERTY_NOSERIALIZE( m_parentNode );
	PROPERTY_NOSERIALIZE( m_position );	
	NATIVE_FUNCTION( "GetParentPoint", funcGetParentPoint );
	NATIVE_FUNCTION( "GetWorldPosition", funcGetWorldPosition );
	NATIVE_FUNCTION( "GetNode", funcGetNode );
	NATIVE_FUNCTION( "GetGeneratedFieldName", funcGetGeneratedFieldName );
	NATIVE_FUNCTION( "GetFieldStrength", funcGetFieldStrength );
	NATIVE_FUNCTION( "SetFieldStrengthMultiplier", funcSetFieldStrengthMultiplier );
	NATIVE_FUNCTION( "GetFieldStrengthMultiplier", funcGetFieldStrengthMultiplier );
	NATIVE_FUNCTION( "SetTestParameter", funcSetTestParameter );
	NATIVE_FUNCTION( "GetTestParameter", funcGetTestParameter );
END_CLASS_RTTI();

///////////////////////////////////////////////////////////////////////////////

class CInterestPoint : public CObject
{
	DECLARE_ENGINE_CLASS( CInterestPoint, CObject, 0 );

	friend class CInterestPointInstance;

private:
	// static data
	CName									m_fieldName;
	IPotentialField* 						m_potentialField;

public:
	CInterestPoint();

	//! Instantiates the interest point
	virtual CInterestPointInstance* CreateInstance( CObject* parent, const THandle< CNode >& node, Float timeout = 0.0f );

	//! Instantiates the interest point at a static position
	virtual CInterestPointInstance* CreateInstance( CObject* parent, const Vector& position, Float timeout = 0.0f );

	// Returns the name of the emitted field
	RED_INLINE const CName& GetFieldName() const { return m_fieldName; }

	// Sets the name of the emitted field
	void SetFieldName( CName fieldName ) { m_fieldName = fieldName; }

	// Returns the potential field
	RED_INLINE const IPotentialField* GetField() const { return m_potentialField; }

	// Sets the potential field
	void SetField( IPotentialField* field ) { m_potentialField = field; }
};
BEGIN_CLASS_RTTI( CInterestPoint );
	PARENT_CLASS( CObject );	
	PROPERTY_CUSTOM_EDIT( m_fieldName, TXT( "Name of the emitted field" ), TXT( "ReactionFieldEditor" ) );
	PROPERTY_INLINED( m_potentialField, TXT( "Potential field this point emits" ) );
END_CLASS_RTTI();

///////////////////////////////////////////////////////////////////////////////

class CScriptedInterestPoint : public CInterestPoint
{
	DECLARE_ENGINE_CLASS( CScriptedInterestPoint, CInterestPoint, 0 );

	friend class CInterestPointInstance;

public:
	CScriptedInterestPoint();

	//! Instantiates the interest point
	virtual CInterestPointInstance* CreateInstance( CObject* parent, const THandle< CNode >& node, Float timeout = 0.0f );

	//! Instantiates the interest point at a static position
	virtual CInterestPointInstance* CreateInstance( CObject* parent, const Vector& position, Float timeout = 0.0f );
};
BEGIN_CLASS_RTTI( CScriptedInterestPoint );
	PARENT_CLASS( CInterestPoint );
END_CLASS_RTTI();

///////////////////////////////////////////////////////////////////////////////

class CInterestPointComponent : public CComponent
{
	DECLARE_ENGINE_CLASS( CInterestPointComponent, CComponent, 0 );

private:
	Bool						m_active;
	CInterestPoint*				m_interestPoint;

public:
	CInterestPointComponent();

	virtual void OnAttached( CWorld* world );
	virtual void OnDetached( CWorld* world );

	//! Activates/deactivates the component
	RED_INLINE void Activate( Bool activationFlag ) { m_active = activationFlag; }

	//! Tells whether the component is active or not
	RED_INLINE Bool IsActive() const { return m_active; }

	//! Returns the interest point this component carries
	RED_INLINE CInterestPoint* GetInterestPoint() const { return m_interestPoint; }

};
BEGIN_CLASS_RTTI( CInterestPointComponent );
	PARENT_CLASS( CComponent );
	PROPERTY_EDIT( m_active, TXT( "Is component active" ) )
	PROPERTY_INLINED( m_interestPoint, TXT( "Interest point" ) )
END_CLASS_RTTI();
