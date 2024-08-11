
#pragma once

enum EActionMoveAnimationSyncType
{
	AMAST_None,
	AMAST_CrossBlendIn,
	AMAST_CrossBlendOut,
};

BEGIN_ENUM_RTTI( EActionMoveAnimationSyncType );
	ENUM_OPTION( AMAST_None );
	ENUM_OPTION( AMAST_CrossBlendIn );
	ENUM_OPTION( AMAST_CrossBlendOut );
END_ENUM_RTTI();

class CActionMoveAnimationProxy : public CObject
{
	DECLARE_ENGINE_CLASS( CActionMoveAnimationProxy, CObject, 0 );

public:
	Bool	m_isInitialized;
	Bool	m_isValid;

public: // Move action time situation
	Float	m_duration;
	Float	m_prevTime;
	Float	m_currTime;
	//Float	m_blendIn;
	//Float	m_blendOut;

public: // Flags
	Bool	m_finished;

public:
	CActionMoveAnimationProxy();

protected:
	Bool IsInitialized() const;
	Bool IsValid() const;
	Bool IsFinished() const;
	Bool WillBeFinished( Float time ) const;

protected:
	void funcIsInitialized( CScriptStackFrame& stack, void* result );
	void funcIsValid( CScriptStackFrame& stack, void* result );
	void funcIsFinished( CScriptStackFrame& stack, void* result );
	void funcWillBeFinished( CScriptStackFrame& stack, void* result );
};

BEGIN_CLASS_RTTI( CActionMoveAnimationProxy );
	PARENT_CLASS( CObject );
	PROPERTY( m_isInitialized );
	PROPERTY( m_isValid );
	PROPERTY( m_duration );
	PROPERTY( m_prevTime );
	PROPERTY( m_currTime );
	PROPERTY( m_finished );

	NATIVE_FUNCTION( "IsInitialized", funcIsInitialized );
	NATIVE_FUNCTION( "IsValid", funcIsValid );
	NATIVE_FUNCTION( "IsFinished", funcIsFinished );
	NATIVE_FUNCTION( "WillBeFinished", funcWillBeFinished );
END_CLASS_RTTI();
