/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#ifndef NO_DEBUG_PAGES

#include "../core/debugPageHandler.h"
#include "../core/debugPageHTMLDoc.h"

#include "triggerManager.h"
#include "triggerManagerImpl.h"
#include "areaConvex.h"

/// trigger debug page
class CDebugPageTriggers : public IDebugPageHandlerHTML
{
public:
	CDebugPageTriggers()
		: IDebugPageHandlerHTML( "/triggers/" )
	{}

private:
	// IDebugPageHandler interface
	virtual StringAnsi GetTitle() const override { return "Triggers"; }
	virtual StringAnsi GetCategory() const override { return "Game"; }
	virtual Bool HasIndexPage() const override { return GGame && GGame->GetActiveWorld(); }

	//! Get trigger mask string
	static const StringAnsi GetTriggerMask( const Uint32 mask )
	{
		StringAnsi ret;

		static CBitField* triggerChannels = static_cast< CBitField* >( GetTypeObject< ETriggerChannel >() );
		for ( Uint32 i=0; i<32; ++i )
		{
			if ( mask & (1<<i) )
			{
				if ( !ret.Empty() )
					ret += "; ";

				CName bitName = triggerChannels ? triggerChannels->GetBitName(i) : CName::NONE;
				if ( bitName )
				{
					ret += bitName.AsAnsiChar();
				}
				else
				{
					ret += StringAnsi::Printf( "Bit%d", i );
				}
			}
		}

		return ret;
	}

	//! Get interactions in given trigger
	static void GetActivatorsInTrigger( const CTriggerManager* manager, const CTriggerObject* trigger, TDynArray< const CTriggerActivator* >& outActivators )
	{
		const Uint32 numActivators = manager->GetNumActivators();
		for ( Uint32 i=0; i<numActivators; ++i )
		{
			const CTriggerActivator* activator = manager->GetActivator(i);

			const Uint32 numInteractions = activator->GetNumInteractions();
			for ( Uint32 j=0; j<numInteractions; ++j )
			{
				if ( activator->GetInteraction(j) == trigger->GetID() )
				{
					outActivators.PushBack( activator );
					break;
				}
			}
		}
	}

	//! Activator information
	class ActivatorInfo : public CDebugPageHTMLTable::IRow
	{
	public:
		const Uint32		m_id;
		StringAnsi			m_debugName;
		const CComponent*	m_comp;
		Int32				m_ccd;
		StringAnsi			m_mask;
		Int32				m_numTriggers;

	public:
		ActivatorInfo( const CTriggerActivator* act )
#ifndef RED_TRIGGER_SYSTEM_NO_DEBUG_NAMES
			: m_debugName( UNICODE_TO_ANSI( act->GetDebugName().AsChar() ) )
#else
			: m_debugName( "debug name optimized out" )
#endif
			, m_comp( act->GetComponent() )
			, m_numTriggers( act->GetNumOccupiedTrigger() )
			, m_mask( GetTriggerMask( act->GetMask() ) )
			, m_ccd( act->IsCCDEnabled() ? 1 : 0)
			, m_id( act->GetUniqueID() )
		{
		}

		virtual Bool OnCompare( const IRow* other, const Int32 columnID ) override
		{
			const auto* b = ((const ActivatorInfo*)other);

			switch ( columnID )
			{
				case 2: return m_mask < b->m_mask;
				case 3: return m_numTriggers < b->m_numTriggers;
				case 4: return m_ccd < b->m_ccd;
				case 5: return (Uint64)m_comp < (Uint64)b->m_comp;
				case 6: return m_debugName < b->m_debugName ;
			}

			return m_id < b->m_id; // 1
		}

		virtual void OnGenerateHTML( CDebugPageHTMLDocument& doc, const Int32 columnID ) override
		{
			switch ( columnID )
			{
				case 1: doc.Link( "/triggers/?activator=%d", m_id).Writef( "%d", m_id ); break;
				case 2: doc.Write( m_mask.AsChar() ); break;
				case 3: if ( m_numTriggers > 0 ) {doc.Writef("%d", m_numTriggers); } break;
				case 4: if ( m_ccd ) doc.Write( "CCD" ); break;
				case 5: if ( m_comp ) { doc.LinkObject( m_comp ); } break;
				case 6: doc.Write( m_debugName.AsChar() ); break;
			}
		}
	};

	//! Trigger information
	class TriggerInfo : public CDebugPageHTMLTable::IRow
	{
	public:
		const Uint32		m_id;
		StringAnsi			m_debugName;
		const CComponent*	m_comp;
		StringAnsi			m_flags;
		StringAnsi			m_exclude;
		Int32				m_ccd;
		Int32				m_memory;
		Int32				m_numActivators;
		Int32				m_numConvex;

	public:
		TriggerInfo( const CTriggerManager* manager, const CTriggerObject* obj )
#ifndef RED_TRIGGER_SYSTEM_NO_DEBUG_NAMES
			: m_debugName( UNICODE_TO_ANSI( obj->GetDebugName().AsChar() ) )
#else
			: m_debugName( "debug name optimized out" )
#endif
			, m_comp( obj->GetComponent() )
			, m_ccd( obj->IsCCDEnabled() ? 1 : 0 )
			, m_numConvex( obj->GetConvexNum() )
			, m_memory( obj->CalcMemoryUsage() )
			, m_id( obj->GetID() )
		{
			TDynArray< const CTriggerActivator* > activators;
			GetActivatorsInTrigger( manager, obj, activators );
			m_numActivators = activators.Size();

			m_flags = GetTriggerMask( obj->GetIncludedChannels() );
			if ( obj->GetExcludedChannels() )
			{
				m_flags += "~(";
				m_flags += GetTriggerMask( obj->GetExcludedChannels() );
				m_flags += ")";
			}
		}

		virtual Bool OnCompare( const IRow* other, const Int32 columnID ) override
		{
			const auto* b = ((const TriggerInfo*)other);

			switch ( columnID )
			{
				case 2: return m_flags < b->m_flags; 
				case 3: return m_numConvex < b->m_numConvex;
				case 4: return m_numActivators < b->m_numActivators;
				case 5: return m_memory < b->m_memory;
				case 6: return m_ccd < b->m_ccd;
				case 7: return (Uint64)m_comp < (Uint64)b->m_comp;
				case 8: return m_debugName < m_debugName;
			}

			return m_id < b->m_id; // 1
		}

		virtual void OnGenerateHTML( CDebugPageHTMLDocument& doc, const Int32 columnID ) override
		{
			switch ( columnID )
			{
				case 1: doc.Link( "/triggers/?trigger=%d", m_id).Writef( "%d", m_id ); break;
				case 2: doc.Write( m_flags.AsChar() ); break;
				case 3: doc.Writef( "%d", m_numConvex ); break;
				case 4: doc.Writef( "%d", m_numActivators ); break;
				case 5: doc.Writef( "%1.2f KB", m_memory / 1024.0f ); break;
				case 6: if ( m_comp ) doc.Write( "CCD" ); break;
				case 7: if ( m_comp ) { doc.LinkObject( m_comp ); } break;
				case 8: doc.Write( m_debugName.AsChar() ); break;
			}
		}
	};

	//! Convex shape information
	class ShapeInfo : public CDebugPageHTMLTable::IRow
	{
	public:
		const Uint32		m_triggerID;
		const Uint32		m_convexID;
		Int32				m_runtimeMemory;
		Int32				m_staticMemory;
		Int32				m_numPlanes;
		Int32				m_numFaces;
		Int32				m_numEdges;
		Int32				m_numVertices;

	public:
		ShapeInfo( const CTriggerConvex* shape )
			: m_triggerID( shape->m_object->GetID() )
			, m_convexID( shape->m_convexIndex )
			, m_runtimeMemory( shape->CalcMemoryUsage() )
			, m_staticMemory( 0 )
			, m_numPlanes( shape->m_numReferencePlanes )
			, m_numFaces( 0 )
			, m_numVertices( 0 )
			, m_numEdges( 0 )
		{
			if ( shape->m_shape )
			{
				auto convex = shape->m_shape->GetConvex( shape->m_convexIndex );
				if ( convex != nullptr )
				{
					m_staticMemory = convex->CalcMemorySize();
					m_numFaces = convex->GetNumFaces();
					m_numVertices = convex->GetNumVertices();
					m_numEdges = convex->GetNumEdges();
				}
			}
		}

		virtual Bool OnCompare( const IRow* other, const Int32 columnID ) override
		{
			const auto* b = ((const ShapeInfo*)other);

			switch ( columnID )
			{
				case 2: return m_runtimeMemory < b->m_runtimeMemory;
				case 3: return m_staticMemory < b->m_staticMemory;
				case 4: return m_numPlanes < b->m_numPlanes;
				case 5: return m_numVertices < b->m_numVertices;
				case 6: return m_numEdges < b->m_numEdges;
				case 7: return m_numFaces < b->m_numFaces;
			}

			return m_convexID < b->m_convexID; // 1
		}

		virtual void OnGenerateHTML( CDebugPageHTMLDocument& doc, const Int32 columnID ) override
		{
			switch ( columnID )
			{
				case 1:	doc.Link( "/triggers/?trigger=%d&shape=%d", m_triggerID, m_convexID ).Writef( "%d", m_convexID ); break;
				case 2: doc.Writef( "%d", m_runtimeMemory ); break;
				case 3: doc.Writef( "%d", m_staticMemory ); break;
				case 4: doc.Writef( "%d", m_numPlanes ); break;
				case 5: doc.Writef( "%d", m_numVertices ); break;
				case 6: doc.Writef( "%d", m_numEdges ); break;
				case 7: doc.Writef( "%d", m_numFaces ); break;
			}
		}
	};

	class CDebugPageHTMLTableActivator : public CDebugPageHTMLTable
	{
	public:
		CDebugPageHTMLTableActivator( class CDebugPageHTMLDocument& doc )
			: CDebugPageHTMLTable( doc, "activators" )
		{
			AddColumn( "ID", 50, true );
			AddColumn( "Channels", 100, true );
			AddColumn( "NumTriggers", 50, true );
			AddColumn( "CCD", 40, true );
			AddColumn( "Component", 80, true );
			AddColumn( "DebugName", 200, true );
		}
	};

	class CDebugPageHTMLTableTriggers : public CDebugPageHTMLTable
	{
	public:
		CDebugPageHTMLTableTriggers( class CDebugPageHTMLDocument& doc )
			: CDebugPageHTMLTable( doc, "triggers" )
		{
			AddColumn( "ID", 50, true );
			AddColumn( "Flags", 100, true );
			AddColumn( "NumConvex", 50, true );
			AddColumn( "NumActivators", 50, true );
			AddColumn( "Memory", 100, true );
			AddColumn( "CCD", 40, true );
			AddColumn( "Component", 80, true );
			AddColumn( "DebugName", 200, true );
		}
	};

	class CDebugPageHTMLTableShapes : public CDebugPageHTMLTable
	{
	public:
		CDebugPageHTMLTableShapes( class CDebugPageHTMLDocument& doc )
			: CDebugPageHTMLTable( doc, "shapes" )
		{
			AddColumn( "ConvexIndex", 70, true );
			AddColumn( "RuntimeMemory", 90, true );
			AddColumn( "SourceMemory", 90, true );
			AddColumn( "NumPlanes", 70, true );
			AddColumn( "NumVertices", 70, true );
			AddColumn( "NumEdges", 70, true );
			AddColumn( "NumFaces", 70, true );
		}
	};

	//! Process command, commands are always passed via URLs, return NULL if not handled
	virtual bool OnFillPage( const class CBasicURL& fullURL, const class CBasicURL& relativeURL, class CDebugPageHTMLDocument& doc ) override
	{
		// no layers
		if ( !GGame || !GGame->GetActiveWorld() )
		{
			doc << "<span class=\"error\">No active world</span>";
			return true;
		}

		// get trigger manager
		const CTriggerManager* manager = static_cast<CTriggerManager*>( GGame->GetActiveWorld()->GetTriggerManager() );
		if ( !manager )
		{
			doc << "<span class=\"error\">No trigger manager</span>";
			return true;
		}

		// Trigger mode
		Int32 customID = -1;
		if ( fullURL.GetKey( "trigger", customID ) )
		{
			// find trigger with that object ID
			const CTriggerObject* object = nullptr;
			{
				const Uint32 count = manager->GetNumObjects();
				for ( Uint32 i=0; i<count; ++i )
				{
					auto temp = manager->GetObject(i);
					if ( temp->GetID() == customID )
					{
						object = temp;
						break;
					}
				}
			}

			if ( !object )
			{
				// no trigger
				doc << "<span class=\"error\">Invalid trigger object ID</span>";
			}
			else
			{
				TDynArray< const CTriggerActivator* > activators;
				GetActivatorsInTrigger( manager, object, activators );

				// general information
				{
					CDebugPageHTMLInfoBlock info( doc, "Trigger information" );
					info.Info( "ID: " ).Writef( "%d", object->GetID() );
					if ( object->GetIncludedChannels() ) info.Info( "Included channels: " ).Write( GetTriggerMask( object->GetIncludedChannels() ).AsChar() );
					if ( object->GetExcludedChannels() ) info.Info( "Excluded channels: " ).Write( GetTriggerMask( object->GetExcludedChannels() ).AsChar() );
					info.Info( "Number of shapes: " ).Writef( "%d", object->GetConvexNum() );
					if ( object->GetBevelRadius() > 0.0f ) info.Info( "Bevel radius: " ).Writef( "%1.2f m", object->GetBevelRadius() );
					if ( object->GetBevelRadiusVertical() > 0.0f ) info.Info( "Bevel radius (vertical): " ).Writef( "%1.2f m", object->GetBevelRadiusVertical() );
					if ( object->GetComponent() ) info.Info( "Component: " ).LinkObject( object->GetComponent() );
					info.Info( "Position: " ).Writef( "%1.2f, %1.2f, %1.2f", object->GetPosition().ToVector().X, object->GetPosition().ToVector().Y, object->GetPosition().ToVector().Z );
					info.Info( "Runtime memory: " ).Writef( "%1.3f KB", object->CalcMemoryUsage() / 1024.0f );
					if ( object->GetSourceShape() ) info.Info( "Source memory: " ).Writef( "%1.3f KB", object->GetSourceShape()->CalcMemorySize() / 1024.0f );
#ifndef RED_TRIGGER_SYSTEM_NO_DEBUG_NAMES
					info.Info( "Debug name: " ).Write( UNICODE_TO_ANSI( object->GetDebugName().AsChar() ) );
#endif
					if ( object->IsCCDEnabled() ) info.Info( "CCD: " ).Write( "Yes" );
				}

				// display activators in the triggers
				if ( !activators.Empty() )
				{
					CDebugPageHTMLInfoBlock info( doc, "Activators in trigger" );
					CDebugPageHTMLTableActivator table( doc );

					for ( Uint32 i=0; i<activators.Size(); ++i )
					{
						table.AddRow( new ActivatorInfo(activators[i]) );
					}
					table.Render( 800, "generic", fullURL );
				}

				// convex shapes
				if ( object->GetConvexNum() > 0 )
				{
					CDebugPageHTMLInfoBlock info( doc, "Convex shapes" );
					CDebugPageHTMLTableShapes table( doc );

					for ( Uint32 i=0; i<object->GetConvexNum(); ++i )
					{
						table.AddRow( new ShapeInfo( object->GetConvex(i) ) );
					}

					table.Render( 450, "generic", fullURL );
				}
			}
		}

		// Activator mode
		else if ( fullURL.GetKey( "activator", customID ) )
		{
			// find the activator with that activator ID
			const CTriggerActivator* act = nullptr;
			{
				const Uint32 count = manager->GetNumActivators();
				for ( Uint32 i=0; i<count; ++i )
				{
					auto temp = manager->GetActivator(i);
					if ( temp->GetUniqueID() == customID )
					{
						act = temp;
						break;
					}
				}
			}

			if ( !act )
			{
				// no activator
				doc << "<span class=\"error\">Invalid activator object ID</span>";
			}
			else
			{
				// general information
				{
					CDebugPageHTMLInfoBlock info( doc, "Activator information" );
					info.Info( "ID: " ).Writef( "%d", act->GetUniqueID() );
					info.Info( "Channels: " ).Write( GetTriggerMask( act->GetChannels() ).AsChar() );
					info.Info( "Shape radius: " ).Writef( "%f", act->GetExtents().ToVector().X );
					info.Info( "Shape height: " ).Writef( "%f", act->GetExtents().ToVector().Z );
					info.Info( "Position: " ).Writef( "%1.2f, %1.2f, %1.2f", act->GetPosition().ToVector().X, act->GetPosition().ToVector().Y, act->GetPosition().ToVector().Z );
					if ( act->GetComponent() ) info.Info( "Component: " ).LinkObject( act->GetComponent() );
#ifndef RED_TRIGGER_SYSTEM_NO_DEBUG_NAMES
					info.Info( "Debug name: " ).Write( UNICODE_TO_ANSI( act->GetDebugName().AsChar() ) );
#endif
					if ( act->IsCCDEnabled() ) info.Info( "CCD: " ).Write( "Yes" );
				}

				// triggers in activator
				if ( act->GetNumInteractions() > 0 )
				{
					CDebugPageHTMLInfoBlock info( doc, "Touched triggers " );
					CDebugPageHTMLTableTriggers table( doc );

					for ( Uint32 i=0; i<act->GetNumInteractions(); ++i )
					{
						const auto triggerID = act->GetInteraction(i);

						// find trigger
						const Uint32 numTriggers = manager->GetNumObjects();
						for ( Uint32 j=0; j<numTriggers; ++j )
						{
							const CTriggerObject* trigger = manager->GetObject(j);
							if ( trigger && trigger->GetID() == triggerID )
							{
								table.AddRow( new TriggerInfo( manager, trigger ) );
								break;
							}
						}
					}

					table.Render( 800, "generic", fullURL );
				}
			}
		}

		// Generic case
		else
		{
			// General stats
			{
				CDebugPageHTMLInfoBlock info( doc, "General stats" );
				info.Info( "Number of triggers: " ).Writef( "%d", manager->GetNumObjects() );
				info.Info( "Number of activators: " ).Writef( "%d", manager->GetNumActivators() );

				// compute memory used by trigger shapes
				Uint32 totalRuntimeMemory = 0;
				Uint32 totalStaticMemory = 0;
				const Uint32 numTriggers = manager->GetNumObjects();
				for ( Uint32 i=0; i<numTriggers; ++i )
				{
					auto trigger = manager->GetObject(i);

					totalRuntimeMemory += trigger->CalcMemoryUsage();

					if ( trigger->GetSourceShape() )
						totalStaticMemory += trigger->GetSourceShape()->CalcMemorySize();
				}
				info.Info( "Runtime memory usage: " ).Writef( "%1.2f KB", totalRuntimeMemory / 1024.0f );
				info.Info( "Source memory usage: " ).Writef( "%1.2f KB", totalStaticMemory / 1024.0f );
			}

			// Triggers
			{
				CDebugPageHTMLInfoBlock info( doc, "Triggers" );

				CDebugPageHTMLTableTriggers table( doc );	
				Uint32 count = manager->GetNumObjects();
				for ( Uint32 i=0; i<count; ++i )
				{
					const CTriggerObject* object = manager->GetObject(i);
					if ( object )
					{
						table.AddRow( new TriggerInfo(manager, object) );
					}
				}

				table.Render( 800, "generic", fullURL );
			}

			// Activators
			{
				CDebugPageHTMLInfoBlock info( doc, "Activators" );

				CDebugPageHTMLTableActivator table( doc );
				Uint32 count = manager->GetNumActivators();
				for ( Uint32 i=0; i<count; ++i )
				{
					const CTriggerActivator* activator = manager->GetActivator(i);
					if ( activator )
					{
						table.AddRow( new ActivatorInfo(activator) );
					}
				}

				table.Render( 800, "generic", fullURL );
			}
		}


		return true;
	}
};

//-----

void InitTriggeraDebugPages()
{
	new CDebugPageTriggers();
}

//-----

#endif
