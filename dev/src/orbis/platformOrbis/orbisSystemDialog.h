/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#ifndef _ORBIS_SYSTEM_DIALOG_H_
#define _ORBIS_SYSTEM_DIALOG_H_

#include <common_dialog/types.h>

class IOrbisSystemDialog
{
public:
	virtual ~IOrbisSystemDialog() {}

	virtual Bool Shutdown() = 0;

	virtual Bool IsShown() const = 0;
};

template< typename TParams >
class COrbisSystemDialog : public IOrbisSystemDialog
{
public:
	COrbisSystemDialog();
	virtual ~COrbisSystemDialog() {}

	Bool Activate( const TParams& params );
	virtual Bool Shutdown() override final;

private:

	Int32 Load();
	Int32 Initialize();
	Int32 Open( const TParams& params );
	Int32 Close();
	virtual Bool IsShown() const override final;
	Int32 Terminate();
	Int32 Unload();

	enum class EStage
	{
		Start,
		Loaded,
		Initialized,
		Displayed
	};

	EStage m_stage;
};

template< typename TParams >
COrbisSystemDialog<TParams>::COrbisSystemDialog()
:	m_stage( EStage::Start )
{

}

template< typename TParams >
Bool COrbisSystemDialog<TParams>::Activate( const TParams& params )
{
	RED_FATAL_ASSERT( m_stage == EStage::Start, "Cannot display a dialog if another is currently active" );

	ORBIS_SYS_CALL_RET( Load() );
	m_stage = EStage::Loaded;

	ORBIS_SYS_CALL_RET( Initialize() );
	m_stage = EStage::Initialized;

	ORBIS_SYS_CALL_RET( Open( params ) );
	m_stage = EStage::Displayed;

	return true;
}

template< typename TParams >
Bool COrbisSystemDialog<TParams>::Shutdown()
{
	switch( m_stage )
	{
	case EStage::Displayed:
		if( IsShown() )
		{
			ORBIS_SYS_CALL_RET( Close() );
		}
		m_stage = EStage::Initialized;

	case EStage::Initialized:
		ORBIS_SYS_CALL_RET( Terminate() );
		m_stage = EStage::Loaded;

	case EStage::Loaded:
		ORBIS_SYS_CALL_RET( Unload() );
		m_stage = EStage::Start;
	}

	return true;
}

struct SceNpProfileDialogParam;
struct SceWebBrowserDialogParam;
struct SceErrorDialogParam;
struct SceNpCommerceDialogParam;

typedef COrbisSystemDialog< SceNpProfileDialogParam > COrbisUserProfileDialog;
typedef COrbisSystemDialog< SceWebBrowserDialogParam > COrbisWebBrowserDialog;
typedef COrbisSystemDialog< SceErrorDialogParam > COrbisErrorDialog;
typedef COrbisSystemDialog< SceNpCommerceDialogParam > COrbisCommerceDialog;

#endif // _ORBIS_SYSTEM_DIALOG_H_
