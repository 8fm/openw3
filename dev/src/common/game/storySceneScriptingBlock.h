/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "storySceneGraphBlock.h"
#include "storySceneControlPart.h"

struct StorySceneScriptParam
{
	DECLARE_RTTI_STRUCT( StorySceneScriptParam );

	CName		m_name;
	CVariant	m_value;

	RED_INLINE StorySceneScriptParam() {};

	RED_INLINE StorySceneScriptParam( CName propName, CName valueTypeName )
		: m_name( propName )
		, m_value( valueTypeName, NULL )
	{};

	RED_INLINE StorySceneScriptParam( CName propName, const CVariant& propValue )
		: m_name( propName )
		, m_value( propValue )
	{};

};

BEGIN_CLASS_RTTI( StorySceneScriptParam );
	PROPERTY_RO( m_name, TXT("Property name") );
	PROPERTY_EDIT( m_value, TXT("Value") );
END_CLASS_RTTI();


class CStorySceneScript : public CStorySceneControlPart, public IDynamicPropertiesSupplier
{
	DECLARE_ENGINE_CLASS( CStorySceneScript, CStorySceneControlPart, 0 )

public:
	static const Uint32 TRUE_LINK_INDEX;
	static const Uint32 FALSE_LINK_INDEX;

protected:
	CName									m_functionName;
	mutable CFunction*						m_function;

	TDynArray<CStorySceneLinkElement*>		m_links;
	TDynArray< StorySceneScriptParam >		m_parameters;

public:
	RED_INLINE const TDynArray< StorySceneScriptParam >& GetParameters() const { return m_parameters; }
	const TDynArray<CStorySceneLinkElement*>& GetOutLinks() const { return m_links; }
	RED_INLINE const CName& GetFunctionName() const { return m_functionName; }

public: 
	CStorySceneScript();

	//! Get function that is bound to this block
	CFunction* GetFunction() const;

	//! Reset internal layout of function parameters
	Bool ResetFunctionLayout();

	//! Execute script function, returns NULL if no latent thread was created
	CScriptThread* Execute( CStoryScenePlayer* player, void* threadReturnValue, Int32& result ) const;

public:
#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnPropertyPostChange( IProperty* property );		
#endif

	//to be removed afer resave
	virtual Bool OnPropertyMissing( CName propertyName, const CVariant& readValue );

	virtual void OnScriptReloaded();

protected:
	virtual IDynamicPropertiesSupplier* QueryDynamicPropertiesSupplier();
	virtual const IDynamicPropertiesSupplier* QueryDynamicPropertiesSupplier() const;
	virtual void GetDynamicProperties( TDynArray< CName >& properties ) const;
	virtual Bool ReadDynamicProperty( const CName& propName, CVariant& propValue ) const;
	virtual Bool WriteDynamicProperty( const CName& propName, const CVariant& propValue );
	virtual void SerializeDynamicPropertiesForGC( IFile& file );

private:
	virtual void CollectControlPartsImpl( TDynArray< CStorySceneControlPart* >& controlParts, TDynArray< const CStorySceneControlPart* >& visitedControlParts ) override;
};

BEGIN_CLASS_RTTI( CStorySceneScript );
	PARENT_CLASS( CStorySceneControlPart );
	PROPERTY_CUSTOM_EDIT( m_functionName, TXT("Name of the scene function"), TXT("SceneFunctionList") );
	PROPERTY( m_links );
END_CLASS_RTTI();

class CStorySceneScriptingBlock : public CStorySceneGraphBlock
{
	DECLARE_ENGINE_CLASS( CStorySceneScriptingBlock, CStorySceneGraphBlock, 0 )

private:
	CStorySceneScript*	m_sceneScript;
		
public:
	CStorySceneScriptingBlock()  {}

	void SetStorySceneScript( CStorySceneScript* sceneScript );
	RED_INLINE CStorySceneScript* GetStorySceneScript() const { return m_sceneScript; }

#ifndef NO_EDITOR_GRAPH_SUPPORT

	void UpdateScriptInfo();
	//! Get the name of the block
	virtual String GetBlockName() const;

	//! Rebuild block layout
	virtual void OnRebuildSockets();

	//! Get title bar color
	virtual Color GetTitleColor() const;

	virtual void OnDestroyed();

#endif

public:
	virtual CStorySceneControlPart* GetControlPart() const { return m_sceneScript;  }
	virtual void SetControlPart( CStorySceneControlPart* part ) { m_sceneScript = Cast< CStorySceneScript >( part ); }
};

BEGIN_CLASS_RTTI( CStorySceneScriptingBlock );
	PARENT_CLASS( CStorySceneGraphBlock );
	PROPERTY( m_sceneScript );
END_CLASS_RTTI();
