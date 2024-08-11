#include "build.h"
#include "aiProfile.h"
#include "factsDB.h"
#include "aiPresetParam.h"
#include "aiParameters.h"
#include "aiRedefinitionParameters.h"

IMPLEMENT_RTTI_ENUM( EVisibilityTest );
IMPLEMENT_ENGINE_CLASS( CAIProfile );
IMPLEMENT_ENGINE_CLASS( CAIReaction );
IMPLEMENT_ENGINE_CLASS( CAISenseParams );
IMPLEMENT_ENGINE_CLASS( SAIReactionRange );
IMPLEMENT_ENGINE_CLASS( SAIReactionFactTest );
IMPLEMENT_ENGINE_CLASS( SAIMinigameParams );
IMPLEMENT_ENGINE_CLASS( CAIMinigameParamsWristWrestling );
IMPLEMENT_RTTI_ENUM( EAIMinigameDifficulty );

///////////////////////////////////////////////////////////////////////
Bool SAIReactionRange::PerformTest( CNode* reactingNPC, const Vector& interestPointPosition ) const
{
	if( !m_enabled )
		return true;

	Vector vec = interestPointPosition - reactingNPC->GetWorldPosition();
	Float dist = vec.Mag2();
	if( dist < m_rangeMax )
	{
		if( vec.Z < m_rangeTop && vec.Z > m_rangeBottom )
		{
			Vector dir = reactingNPC->GetLocalToWorld().V[1];
			if( m_yaw > 0.0f )
			{
				dir = EulerAngles( 0.0f, 0.0f, m_yaw ).TransformVector( dir );
			}
			dir.Z = 0.0f;
			dir.Normalize2();
			vec.Z = 0.0f;
			vec.Normalize2();
			Float dot = Vector::Dot2( dir, vec );
			Float cosHalfAngle = MCos( DEG2RAD( m_rangeAngle * 0.5f ) );
			if( dot >= cosHalfAngle )
			{
				return true;
			}
		}
	}

	return false;
};

///////////////////////////////////////////////////////////////////////
Bool SAIReactionFactTest::PerformTest() const
{
	if( !m_enabled )
		return true;
	
	CFactsDB *factsDB = GCommonGame ? GCommonGame->GetSystem< CFactsDB >() : NULL;
	if( factsDB )
		return CFactsDBEditorQuery::Evaluate( *factsDB, m_queryFact, m_factId, m_value, m_compareFunc );
	else
		return false;
};

///////////////////////////////////////////////////////////////////////
CAIProfile::CAIProfile()
	: m_attitudeGroup( CNAME( default ) )
#ifdef AI_WIZARD_BACKWARD_COMPAT
	, m_aiResource( NULL )
#endif
	, m_senseVisionParams( NULL )
	, m_senseAbsoluteParams( NULL )
#ifndef NO_EDITOR
	, m_aiWizardRes( NULL )
#endif
{
}

void CAIProfile::AddReaction( CAIReaction* reaction )
{
	if ( !reaction || m_reactions.Exist( reaction ) )
	{
		return;
	}
	reaction->SetParent( this );
	m_reactions.PushBack( reaction );
}

void CAIProfile::RemoveReaction( CAIReaction* reaction )
{
	if ( !reaction )
	{
		return;
	}
	m_reactions.Remove( reaction );
}

void CAIProfile::MoveReaction( CAIReaction* reaction, Bool up )
{
	if( m_reactions.Size() == 0 )
	{
		return;
	}

	Uint32 idx = 0xFFFFFFFF;
	for( Uint32 i=0; i<m_reactions.Size(); i++ )
	{
		if( m_reactions[i] == reaction )
		{
			idx = i;
			break;
		}
	}

	ASSERT( idx < m_reactions.Size() );

	if( up )
	{
		if( idx >= 1 && idx<m_reactions.Size() )
		{
			CAIReaction* tmp = m_reactions[idx-1].Get();
			m_reactions[idx-1] = m_reactions[idx];
			m_reactions[idx] = tmp;
		}
	}
	else
	{
		if( idx<m_reactions.Size()-1 )
		{
			CAIReaction* tmp = m_reactions[idx].Get();
			m_reactions[idx] = m_reactions[idx+1];
			m_reactions[idx+1] = tmp;
		}
	}
}

CAIReaction* CAIProfile::FindReaction( const CName& fieldName ) const
{
	for ( TDynArray< THandle< CAIReaction > >::const_iterator it = m_reactions.Begin(); 
		it != m_reactions.End(); ++it )
	{
		if ( !*it )
		{
			continue;
		}

		if ( (*it)->m_fieldName == fieldName )
		{
			return (*it).Get();
		}
	}

	return NULL;
}

CAISenseParams* CAIProfile::GetSenseParams( EAISenseType senseType ) const
{
	switch( senseType )
	{
	case AIST_Vision:
		return m_senseVisionParams;

	case AIST_Absolute:
		return m_senseAbsoluteParams;

	default:
		ASSERT( 0 && "Unknown sense type" );
	}

	return NULL;
}

void CAIProfile::SetSenseParams( EAISenseType senseType, CAISenseParams* senseParams )
{
	switch( senseType )
	{
	case AIST_Vision:
		m_senseVisionParams = senseParams;
		break;

	case AIST_Absolute:
		m_senseAbsoluteParams = senseParams;
		break;

	default:
		ASSERT( 0 && "Unknown sense type" );
	}
}

#ifdef AI_WIZARD_BACKWARD_COMPAT
void CAIProfile::ClearOldDataFromIncludes()
{
	for( Uint32 i = m_aiPresets.Size(); i > 0 ; --i )
	{
		CEntityTemplate *const preset = m_aiPresets[ i - 1 ].Get();
		CObject* et = this->GetParent();
		CEntityTemplate* templ	= et ? Cast< CEntityTemplate > ( et ) : NULL;
		if( templ && templ->MarkModified() )
		{		
			if( m_aiPresets.Remove( preset ) )
			{
				TDynArray< THandle< CEntityTemplate > >& includes = templ->GetIncludes();
				for ( Uint32 i = includes.Size(); i > 0; --i )
				{
					if ( includes[i-1].Get() == preset )
					{
						includes.RemoveAt(i-1);
						break;
					}
				}
			}
		}		
	}

	m_aiPresets.Clear();
	m_aiResource = nullptr;
}
#endif
/////////////////////////////////////////////////////////////////////////
// CAITemplateParam
IMPLEMENT_ENGINE_CLASS( CAITemplateParam )

/////////////////////////////////////////////////////////////////////////
// CAIBaseTreeTemplateParam
IMPLEMENT_ENGINE_CLASS( CAIBaseTreeTemplateParam )
CAIBaseTreeTemplateParam::CAIBaseTreeTemplateParam()
	: CAITemplateParam( TXT("aiBaseTree") )
{
}
/////////////////////////////////////////////////////////////////////////
// CAIPresetsTemplateParam
IMPLEMENT_ENGINE_CLASS( CAIPresetsTemplateParam )
IMPLEMENT_ENGINE_CLASS( CEDSavedAnswer )
IMPLEMENT_ENGINE_CLASS( CEdWizardSavedAnswers )
CAIPresetsTemplateParam::CAIPresetsTemplateParam()
{
}

#ifndef NO_RESOURCE_COOKING
Bool CAIPresetsTemplateParam::IsCooked()
{
	return !m_customValParameters.Empty();
}
#endif


void CAIPresetsTemplateParam::AddCustomValParameters( ICustomValAIParameters *param )
{ 
	// Clearing out all other custom val params that are the same ( we check all because old files might have more than one )
	for ( Int32 i = m_customValParameters.Size() - 1; i >= 0; --i )
	{
		ICustomValAIParameters *const customValAIParameters = m_customValParameters[ i ];
		if ( customValAIParameters->GetClass()->GetType() == param->GetClass()->GetType() )
		{
			m_customValParameters.RemoveAt( i );
		}
	}
	m_customValParameters.PushBack( param ); 
}

#ifndef NO_EDITOR
/////////////////////////////////////////////////////////////////////////
// CAIWizardTemplateParam
IMPLEMENT_ENGINE_CLASS( CAIWizardTemplateParam )
CAIWizardTemplateParam::CAIWizardTemplateParam()
	: m_aiWizardResource( nullptr )
{
}
#ifndef NO_RESOURCE_COOKING
Bool CAIWizardTemplateParam::IsCooked()
{
	return false;
}
#endif
#endif
