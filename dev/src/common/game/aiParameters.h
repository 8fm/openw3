#pragma once

RED_WARNING_PUSH()
RED_DISABLE_WARNING_MSC( 4800 )			// forcing value to bool 'true' or 'false' (performance warning)

//#define DEBUG_AI_INITIALIZATION

///////////////////////////// INTERFACE /////////////////////////////
class IAIParameters : public IScriptable
{
	typedef IScriptable TBaseClass;
	DECLARE_RTTI_SIMPLE_CLASS( IAIParameters );
protected:
	// Mutable parameters due to lazy initialization
	TArrayMap< CName, CProperty* >					m_paramsMap;
	TDynArray< IAIParameters* >						m_subParameters;
#ifdef DEBUG_AI_INITIALIZATION
	Bool											m_initialized;
#endif
	virtual void OnManualRuntimeCreation();
public:
	IAIParameters();

	void MergeParams( IAIParameters* params );
	void InitializeAIParameters();

	const TArrayMap< CName, CProperty* > GetParams() const			{ return m_paramsMap; }
	const TDynArray< IAIParameters* > GetSubParams() const			{ return m_subParameters; }
	IAIParameters *const  GetAiParametersByClassName( CName contextName ) const;

	template < class T >
	Bool GetParameter( CName name, T& outVal ) const;
	Bool HasParameter( CName name ) const;

	void OnScriptReloaded() override;
	Bool OnPropertyTypeMismatch( CName propertyName, IProperty* existingProperty, const CVariant& readValue ) override;

	void OnPostLoad() override;
#ifndef NO_EDITOR
	void OnCreatedInEditor() override;
	void OnPropertyPostChange( IProperty* property ) override;
#endif
private:
	void funcLoadSteeringGraph( CScriptStackFrame& stack, void* result );
	void funcOnManualRuntimeCreation( CScriptStackFrame& stack, void* result );
};

BEGIN_CLASS_RTTI( IAIParameters )
	PARENT_CLASS( IScriptable )
	NATIVE_FUNCTION( "LoadSteeringGraph", funcLoadSteeringGraph )
	NATIVE_FUNCTION( "OnManualRuntimeCreation", funcOnManualRuntimeCreation )
END_CLASS_RTTI()

///////////////////////////// BASE /////////////////////////////
class CAIParameters : public IAIParameters
{
	typedef IAIParameters TBaseClass;
	DECLARE_RTTI_SIMPLE_CLASS( CAIParameters );
};

BEGIN_CLASS_RTTI( CAIParameters )
	PARENT_CLASS( IAIParameters )
END_CLASS_RTTI()

///////////////////////////// DEFAULTS /////////////////////////////
class CAIDefaults : public IAIParameters
{
	typedef IAIParameters TBaseClass;
	DECLARE_RTTI_SIMPLE_CLASS( CAIDefaults );
};

BEGIN_CLASS_RTTI( CAIDefaults )
	PARENT_CLASS( IAIParameters )
END_CLASS_RTTI()

///////////////////////////// AI TREE OBJECTS //////////////////////
class IAITree : public IAIParameters
{
	typedef IAIParameters TBaseClass;
	DECLARE_RTTI_SIMPLE_CLASS( IAITree );
private:
	THandle< IAIParameters >		m_internalParameters;
	String							m_aiTreeName;

protected:
	THandle< CBehTree >				m_tree;

	virtual void OnManualRuntimeCreation() override;
public:
	IAITree();
	CBehTree* GetTree() const										{ return m_tree.Get(); }
	const String& GetAITreeName() const								{ return m_aiTreeName; }
	IAIParameters* GetParams();
	virtual Bool InitializeData();
	void InitializeTree();

	virtual Bool OnPropertyTypeMismatch( CName propertyName, IProperty* existingProperty, const CVariant& readValue ) override;
	virtual Bool OnPropertyMissing( CName propertyName, const CVariant& readValue ) override;

	virtual void OnPostLoad() override;
#ifndef NO_EDITOR
	void OnCreatedInEditor() override;

	static Bool RefactorAll();
#endif

	void OnCreated();
protected:
	void funcOnCreated( CScriptStackFrame& stack, void* result );
};

BEGIN_CLASS_RTTI( IAITree )
	PARENT_CLASS( IAIParameters )
	PROPERTY_NOSERIALIZE( m_aiTreeName )
	PROPERTY( m_tree )
	NATIVE_FUNCTION( "OnCreated", funcOnCreated )
END_CLASS_RTTI()

class CAIBaseTree : public IAITree
{
	typedef IAITree TBaseClass;
	DECLARE_RTTI_SIMPLE_CLASS( CAIBaseTree );
public:
	CAIBaseTree() 
		: IAITree() {}
};

BEGIN_CLASS_RTTI( CAIBaseTree )
	PARENT_CLASS( IAITree )
END_CLASS_RTTI()

class CAITree : public IAITree
{
	typedef IAITree TBaseClass;
	DECLARE_RTTI_SIMPLE_CLASS( CAITree );

public:
	CAITree()
		: IAITree() {}
};

BEGIN_CLASS_RTTI( CAITree )
	PARENT_CLASS( IAITree )
END_CLASS_RTTI()

class CAIPerformCustomWorkTree : public IAITree
{
	typedef IAITree TBaseClass;
	DECLARE_RTTI_SIMPLE_CLASS( CAIPerformCustomWorkTree );

public:
	CAIPerformCustomWorkTree()
		: IAITree() {}
};

BEGIN_CLASS_RTTI( CAIPerformCustomWorkTree )
	PARENT_CLASS( IAITree )
END_CLASS_RTTI()

#include "aiParameters.inl"

RED_WARNING_POP()

