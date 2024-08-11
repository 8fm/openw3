/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#pragma once

///////////////////////////////////////////////////////////////////////////////

template< class T >
void CEntityTemplate::GetAllParameters( TDynArray< T* >& params,  Bool exploreTemplateList /*= true*/ ) const
{
	Uint32 count			= m_templateParams.Size();

#ifndef RED_FINAL_BUILD
	Bool isCooked			= IsCooked();
	Bool hasTemplateList	= false;
#endif
	
	for ( Uint32 i = 0; i < count; ++i )
	{
		CEntityTemplateParam* param = m_templateParams[i];
		if ( param )
		{
			if ( param->IsA< T >() )
			{
				params.PushBackUnique( Cast< T >( param ) );
			}
#ifndef RED_FINAL_BUILD
			if ( !isCooked && exploreTemplateList )
			{
				if ( param->IsA< CTemplateListParam >() )
				{
					CTemplateListParam*const templateListParam = static_cast< CTemplateListParam *>( param );
					for ( auto templateIt = templateListParam->m_templateList.Begin(), templateEnd = templateListParam->m_templateList.End(); templateIt != templateEnd; ++templateIt )
					{
						// Do not explore template lists recusively ( includes ) if this template has a template list 
						hasTemplateList = true;
						CEntityTemplate* entityTemplate = (*templateIt).Get();
						if ( entityTemplate )
						{
							// need exploreTemplateList = true so that we may have aipresets sat-up with AIWizard
							entityTemplate->GetAllParameters( params, true );
						}
					}
				}
			}
#endif			// #ifndef RED_FINAL_BUILD

		}
	}

#ifndef RED_FINAL_BUILD
	if ( !isCooked )
	{
		if ( hasTemplateList )
		{
			exploreTemplateList = false;
		}

		count = m_includes.Size();
		for ( Uint32 i = 0; i < count; ++i )
		{
			CEntityTemplate *templ = m_includes[i].Get();
			if ( templ )
			{
				templ->GetAllParameters( params, exploreTemplateList );
			}
		}
	}
#endif			// #ifndef RED_FINAL_BUILD
}

///////////////////////////////////////////////////////////////////////////////

template< class T >
T* CEntityTemplate::FindParameter( Bool recursive, Bool exploreTemplateList /*= true*/ ) const
{
	// Search by type
	Uint32 count			= m_templateParams.Size();
#ifndef RED_FINAL_BUILD
	Bool isCooked			= IsCooked();
	Bool hasTemplateList	= false;
#endif

	for ( Uint32 i = 0; i < count; ++i )
	{
		CEntityTemplateParam* param = m_templateParams[i];
		
		if( param )
		{
			const Bool canTest = !IsCooked( ) || ( recursive || param->WasIncluded()  == false ) ;
			if ( canTest && param->IsA< T >() )
			{
				return static_cast< T* >( param );
			}
#ifndef RED_FINAL_BUILD
			if ( !isCooked && exploreTemplateList )
			{
				if ( param->IsA< CTemplateListParam >() )
				{
					CTemplateListParam*const templateListParam = static_cast< CTemplateListParam *>( param );
					for ( auto templateIt = templateListParam->m_templateList.Begin(), templateEnd = templateListParam->m_templateList.End(); templateIt != templateEnd; ++templateIt )
					{
						// Do not explore template lists recusively ( icludes ) if this template has a template list 
						hasTemplateList = true;
						CEntityTemplate* entityTemplate = (*templateIt).Get();
						if ( entityTemplate )
						{
							// need exploreTemplateList = true so that we may have aipresets sat-up with AIWizard
							T * param = entityTemplate->FindParameter< T >( recursive, true );
							if ( param )
							{
								return param;
							}
						}
					}
				}
			}
#endif			// #ifndef RED_FINAL_BUILD
		}		
	}
	
#ifndef RED_FINAL_BUILD
	// Try in base templates
	if ( !isCooked && recursive )
	{
		if ( hasTemplateList )
		{
			exploreTemplateList = false;
		}

		count = m_includes.Size();
		for ( Uint32 i = 0; i < count; ++i )
		{
			CEntityTemplate* baseInclude = m_includes[i].Get();
			if ( baseInclude )
			{
				// Search in included templates
				T* param = baseInclude->FindParameter< T >( recursive, exploreTemplateList );
				if ( param )
				{
					return param;
				}
			}
		}
	}
#endif			// #ifndef RED_FINAL_BUILD

	// Not found
	return NULL;
}

///////////////////////////////////////////////////////////////////////////////

template< class T, class Pred >
T* CEntityTemplate::FindParameter( Bool recursive, const Pred &pred, Bool exploreTemplateList /*= true*/ ) const
{
	// Search by type
	Uint32 count			= m_templateParams.Size();

#ifndef RED_FINAL_BUILD
	Bool hasTemplateList	= false;
	Bool isCooked			= IsCooked();
#endif

	for ( Uint32 i = 0; i < count; ++i )
	{
		CEntityTemplateParam* param = m_templateParams[i];
		
		if( param )
		{
			const Bool canTest = !IsCooked( ) || ( recursive || param->WasIncluded() == false );
			if ( canTest && param->IsA< T >() )
			{
				T *destParam = Cast< T >( param );
				if ( pred( destParam ) )
				{
					return destParam;
				}
			}

#ifndef RED_FINAL_BUILD
			if ( !isCooked && exploreTemplateList )
			{
				if ( param->IsA< CTemplateListParam >() )
				{
					CTemplateListParam*const templateListParam = static_cast< CTemplateListParam *>( param );
					for ( auto templateIt = templateListParam->m_templateList.Begin(), templateEnd = templateListParam->m_templateList.End(); templateIt != templateEnd; ++templateIt )
					{
						// Do not explore template lists recusively ( includes ) if this template has a template list 
						hasTemplateList = true;
						CEntityTemplate* entityTemplate = (*templateIt).Get();
						if ( entityTemplate )
						{
							// Need exploreTemplateList = true so that we may have aipresets sat-up with AIWizard
							T * param = entityTemplate->FindParameter< T >( recursive, pred, true );

							if ( param )
							{
								return param;
							}
						}
					}
				}
			}
#endif			// #ifndef RED_FINAL_BUILD
		}
	}


#ifndef RED_FINAL_BUILD
	// Try in base templates
	if ( !isCooked && recursive )
	{
		if ( hasTemplateList )
		{
			exploreTemplateList = false;
		}

		count = m_includes.Size();
		for ( Uint32 i = 0; i < count; ++i )
		{
			CEntityTemplate* baseInclude = m_includes[i].Get();
			if ( baseInclude )
			{
				// Search in included templates
				T* param = baseInclude->FindParameter< T >( recursive, pred, exploreTemplateList );
				if ( param )
				{
					return param;
				}
			}
		}
	}
#endif		// #ifndef RED_FINAL_BUILD


	// Not found
	return NULL;
}
