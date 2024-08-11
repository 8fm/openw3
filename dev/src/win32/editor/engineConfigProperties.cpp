#include "build.h"
#include "propertiesPage.h"
#include "engineConfigProperties.h"

#include "../../common/core/configVarSystem.h"

class ConfigPropertyPageSettings : public PropertiesPageSettings
{
public:
	ConfigPropertyPageSettings()
	{
		m_autoExpandGroups = true;
		m_allowGrabbing = false;
	}
};

/// config var item
class CEdConfigPropertyVarItem : public CPropertyItem
{
public:
	CEdConfigPropertyVarItem( CEdPropertiesPage* page, CBasePropItem* parent, Config::IConfigVar* var )
		: CPropertyItem( page, parent )
		, m_configVar( var )
	{}

	virtual String GetName() const override
	{
		return ANSI_TO_UNICODE( m_configVar->GetName() );
	}

	virtual Bool IsReadOnly() const override
	{
		return m_configVar->HasFlag( Config::eConsoleVarFlag_ReadOnly );
	}

	virtual Bool IsInlined() const override
	{
		return false;
	}

	RED_FORCE_INLINE Config::IConfigVar* GetVar() const
	{
		return m_configVar;
	}

private:
	Config::IConfigVar*				m_configVar;
};

/// config group
class CEdConfigPropertyGroup : public CBaseGroupItem
{
public:
	CEdConfigPropertyGroup( CEdPropertiesPage* page, CBasePropItem* parent, const Config::CConfigVarHierarchy::Group* configGroup )
		: CBaseGroupItem( page, parent )
		, m_configGroup( configGroup )
	{
		m_isExpandable = true;
	}

	virtual void Expand() override
	{
		// create sub groups
		for ( auto subGroup : m_configGroup->m_children )
		{
			CEdConfigPropertyGroup* newGroup = new CEdConfigPropertyGroup( m_page, this, subGroup );
			newGroup->Expand();
		}

		// create items
		for ( auto entry : m_configGroup->m_entries )
		{
			Config::IConfigVar* var = entry->m_var;

			IRTTIType* propType = GetTypeForConfigVar( *var );
			if ( propType )
			{
				CPropertyItem* item = new CEdConfigPropertyVarItem( m_page, this, var ); // auto registers in m_children
				item->Init( propType );
				item->GrabPropertyValue();
			}
		}
	}

	virtual String GetCaption() const override
	{
		return ANSI_TO_UNICODE( m_configGroup->m_name.AsChar() );
	}

	virtual Bool ReadImp( CPropertyItem* childItem, void *buffer, Int32 objectIndex ) override
	{
		if ( objectIndex == 0 )
		{
			CEdConfigPropertyVarItem* varItem = static_cast< CEdConfigPropertyVarItem* >( childItem );
			if ( varItem->GetVar() )
			{
				return varItem->GetVar()->GetRaw( buffer, varItem->GetVar()->GetType() );				
			}
		}

		return false;
	}

	virtual Bool WriteImp( CPropertyItem* childItem, void *buffer, Int32 objectIndex ) override
	{
		if ( objectIndex == 0 )
		{
			CEdConfigPropertyVarItem* varItem = static_cast< CEdConfigPropertyVarItem* >( childItem );
			if ( varItem->GetVar() )
			{
				return varItem->GetVar()->SetRaw( buffer, varItem->GetVar()->GetType(), Config::eConfigVarSetMode_User );
			}
		}

		return false;
	}

protected:
	const Config::CConfigVarHierarchy::Group*		m_configGroup;

	static IRTTIType* GetTypeForConfigVar( const Config::IConfigVar& var )
	{
		switch ( var.GetType() )
		{
			case Config::eConsoleVarType_Bool: return GetTypeObject< Bool >();
			case Config::eConsoleVarType_Int: return GetTypeObject< Int32 >();
			case Config::eConsoleVarType_Float: return GetTypeObject< Float >();
			case Config::eConsoleVarType_String: return GetTypeObject< String >();
		}

		return nullptr;
	}
};

/// root group
class CEdConfigPropertyGroupRoot : public CObjectsRootItem
{
public:
	CEdConfigPropertyGroupRoot( CEdPropertiesPage* page, CBasePropItem* parent, const Config::CConfigVarHierarchy::Group* configGroup )
		: CObjectsRootItem( page, parent )
		, m_configGroup( configGroup )
	{
		m_baseClass = ClassID< IScriptable >();
		m_tempObject = new IScriptable();
		m_objects.PushBack( STypedObject( m_tempObject, m_baseClass ) );

		Expand();
	}

	virtual ~CEdConfigPropertyGroupRoot()
	{
		delete m_tempObject;
	}

	virtual void Expand() override	
	{
		for ( auto subGroup : m_configGroup->m_children )
		{			
			CEdConfigPropertyGroup* subGroupItem = new CEdConfigPropertyGroup( m_page, this, subGroup ); // auto registers in m_children
			subGroupItem->Expand();
		}
	}

	virtual Bool ReadProperty( CProperty *property, void* buffer, Int32 objectIndex = 0 ) override { return false;}
	virtual Bool WriteProperty( CProperty *property, void* buffer, Int32 objectIndex = 0 ) override { return false;	}
	virtual Bool ReadImp( CPropertyItem* childItem, void *buffer, Int32 objectIndex = 0 ) override { return false; }
	virtual Bool WriteImp( CPropertyItem* childItem, void *buffer, Int32 objectIndex = 0 ) override { return false;	}

private:
	const Config::CConfigVarHierarchy::Group*		m_configGroup;
	IScriptable*									m_tempObject;
};

//-----

CEdConfigPropertiesPage::CEdConfigPropertiesPage( wxWindow* parent, CEdUndoManager* undoManager )
	: CEdPropertiesPage( parent, ConfigPropertyPageSettings(), undoManager )
{
}

CEdConfigPropertiesPage::~CEdConfigPropertiesPage()
{
}

void CEdConfigPropertiesPage::Fill()
{
	// rebuild the storage
	Config::CConfigVarHierarchyBuilder builder( m_configGroup );
	builder.BuildFromRegistry( SConfig::GetInstance().GetRegistry() ); // TODO: implement filtering if needed

	// create the root group
	CObjectsRootItem* root = new CEdConfigPropertyGroupRoot( this, nullptr, m_configGroup.GetRoot() );
	ForcedSetRoot( root );	
}

