
#include "Build.h"
#include "updateThumbnailDialog.h"

#include "../../common/core/thumbnail.h"

CEdUpdateThumbnailDialog::CEdUpdateThumbnailDialog( wxWindow* parent, Bool enableEditorCamera, Bool enableLastUsedCamera )
{
	wxXmlResource::Get()->LoadDialog( this, parent, wxT("UpdateThumbnail") );

	m_camera = XRCCTRL( *this, "Camera", wxChoice );
	m_ground = XRCCTRL( *this, "GroundPlane", wxCheckBox );
	m_removeBG = XRCCTRL( *this, "RemoveBG", wxCheckBox );
	m_copyEnv = XRCCTRL( *this, "CopyEnv", wxCheckBox );
	m_outputIcon = XRCCTRL( *this, "OutputIcon", wxCheckBox );
	m_iconSize = XRCCTRL( *this, "IconSize", wxChoice );

	if ( enableLastUsedCamera )
	{
		m_camera->Append( TXT("last used"), reinterpret_cast< void * >( CT_LastUsed ) );
	}

	if ( enableEditorCamera )
	{
		m_camera->Append( TXT("from editor preview"), reinterpret_cast< void * >( CT_FromEditor ) );
	}
	else
	{
		m_copyEnv->Disable();
	}

	m_camera->Append( TXT("auto adjust"), reinterpret_cast< void * >( CT_Auto ) );

	m_camera->Select( 0 );

	if ( m_camera->GetCount() <= 1 )
	{
		m_camera->Disable();
	}

	m_iconSize->Append( TXT("128") );
	m_iconSize->Append( TXT("256") );
	m_iconSize->Append( TXT("512") );
	m_iconSize->Append( TXT("1024") );

	m_iconSize->SetSelection( 0 );
}

CEdUpdateThumbnailDialog::~CEdUpdateThumbnailDialog()
{
}

Bool CEdUpdateThumbnailDialog::Execute( CameraType& camType, Int32& flags, Uint32& iconSize )
{
	m_ground->SetValue( ( flags & TF_UseGround ) != 0 );

	if ( ShowModal() == wxID_OK )
	{
		if ( m_ground->GetValue() ) flags |= TF_UseGround; else flags &= ~TF_UseGround;
		if ( m_removeBG->GetValue() ) flags |= TF_SetBackgroundColor; else flags &= ~TF_SetBackgroundColor;
		if ( m_copyEnv->IsEnabled() && m_copyEnv->GetValue() ) flags |= TF_CopyEnvironment; else flags &= ~TF_CopyEnvironment;
		if ( m_outputIcon->GetValue() )
		{
			flags |= TF_OutputIcon;
			FromString( String( m_iconSize->GetString( m_iconSize->GetSelection() ) ), iconSize );
		}
		else
		{
			iconSize = 0;
			flags &= ~TF_OutputIcon;
		}

		int camIdx = m_camera->GetSelection();
		ASSERT ( camIdx >= 0 );
		camType = static_cast< CameraType >( reinterpret_cast< Int32 >( m_camera->GetClientData( camIdx ) ) );

		return true;
	}
	else
	{
		return false;
	}
}
