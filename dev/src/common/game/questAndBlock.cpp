#include "build.h"
#include "questGraphSocket.h"
#include "questAndBlock.h"
#include "../core/instanceDataLayoutCompiler.h"
#include "../core/gameSave.h"

IMPLEMENT_ENGINE_CLASS( CQuestAndBlock )

CQuestAndBlock::CQuestAndBlock() 
{ 
	m_name = TXT("AND"); 
}

void CQuestAndBlock::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_activatedInputs;
}

void CQuestAndBlock::OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const
{
	TBaseClass::OnActivate( data, inputName, parentThread );

	data[ i_activatedInputs ].PushBackUnique( inputName );

	if ( data[ i_activatedInputs ].Size() == GetInputsCount() )
	{
		ActivateOutput( data, CNAME( Out ) );
	}
}

void CQuestAndBlock::OnDeactivate( InstanceBuffer& data ) const
{
	TBaseClass::OnDeactivate( data );
	data[ i_activatedInputs ].Clear();
}

void CQuestAndBlock::SaveGame( InstanceBuffer& data, IGameSaver* saver ) const
{
	TBaseClass::SaveGame( data, saver );
	saver->WriteValue( CNAME(ActivatedInputs), data[ i_activatedInputs ] );
}

void CQuestAndBlock::LoadGame( InstanceBuffer& data, IGameLoader* loader ) const
{
	TBaseClass::LoadGame( data, loader );
	loader->ReadValue( CNAME(ActivatedInputs), data[ i_activatedInputs ] );
}

