/*
	GadgetTachyonDisruptor.cpp
	(c)1999 Palestar Development, Richard Lyle
*/


#include "Debug/Assert.h"
#include "Display/PrimitiveSetTransform.h"
#include "Display/PrimitiveTriangleFan.h"
#include "NounShip.h"
#include "NounCargo.h"
#include "NounUnit.h"
#include "NounAsteroid.h"
#include "ShipPlatform.h"
#include "NounJumpGate.h"
#include "NounNebula.h"
#include "Constants.h"
#include "VerbBreakOrbit.h"
#include "GadgetTachyonDisruptor.h"
#include "GameContext.h"
#include "resource.h"
#include "VerbGadgetOff.h"

//---------------------------------------------------------------------------------------------------

static Constant TACHYON_DISRUPTOR_ENERGY_COST( "TACHYON_DISRUPTOR_ENERGY_COST", 0.1f );

//----------------------------------------------------------------------------

IMPLEMENT_ABSTRACT_FACTORY( GadgetTachyonDisruptor, NounGadget ); 

BEGIN_ABSTRACT_PROPERTY_LIST( GadgetTachyonDisruptor, NounGadget );
	ADD_REPLICATED_PROPERTY( m_Target, TICKS_PER_SECOND );
END_PROPERTY_LIST();

//----------------------------------------------------------------------------

void GadgetTachyonDisruptor::render( RenderContext &context, 
					const Matrix33 & frame, 
					const Vector3 & position )
{
	// render the beam
	if ( m_Target.valid() )
	{
		// render the use effect
		Scene * pUseEffect = useEffect();
		if ( pUseEffect != NULL )
			pUseEffect->render( context, frame, position );
		
		Vector3 positionVS( context.worldToView( position ) );
		if (! context.sphereVisible( positionVS, length() ) )
			return;

		Vector3 direction( m_Target->worldPosition() - position );
		Vector3 head( position );
		Vector3 tail( head + direction );

		// calculate the material wrap
		float h = 0.05f;
		float w = 10.0f;

		Material * pTracerMaterial = tracerMaterial();
		if ( pTracerMaterial != NULL )
		{
			h = pTracerMaterial->height();
			w = pTracerMaterial->width();
			Material::push( context, pTracerMaterial );
		}
		else
			Material::push( context, Color(255,0,0), true, PrimitiveMaterial::ADDITIVE );

		float u = (head - tail).magnitude() / w;

		const Vector3	N( 0,0, 0);
		const Vector3	Y( 0, h, 0 );
		const Vector3	X( h, 0, 0 );
		const Color		HC( 255,255,255,255 );
		const Color		TC( 255,255,255,32 );

		VertexL beamY[4] = 
		{
			VertexL( head + Y, N, HC, u, 0.0f ),
			VertexL( tail + Y, N, TC, 0.0f, 0.0f ),
			VertexL( tail - Y, N, TC, 0.0f, 1.0f ),
			VertexL( head - Y, N, HC, u, 1.0f ),
		};
		VertexL beamX[4] = 
		{
			VertexL( head + X, N, HC, u, 0.0f ),
			VertexL( tail + X, N, TC, 0.0f, 0.0f ),
			VertexL( tail - X, N, TC, 0.0f, 1.0f ),
			VertexL( head - X, N, HC, u, 1.0f ),
		};
		
		DisplayDevice * pDisplay = context.device();
		ASSERT( pDisplay );

		context.pushIdentity();
		PrimitiveTriangleFanDL::push( pDisplay, 4, beamY );
		PrimitiveTriangleFanDL::push( pDisplay, 4, beamX );
	}
}

//----------------------------------------------------------------------------

void GadgetTachyonDisruptor::release()
{
	NounGadget::release();
	m_Target = NULL;
}

//----------------------------------------------------------------------------

NounGadget::Type GadgetTachyonDisruptor::type() const
{
	return TACHYON_DISRUPTOR;
}

dword GadgetTachyonDisruptor::hotkey() const
{
	return 'T';
}

bool GadgetTachyonDisruptor::usable( Noun * pTarget, bool shift ) const
{
	if ( m_Target.valid() )
		return true;	// we can always turn the beam off
	if (! NounGadget::usable( pTarget, shift ) )
		return false;
	if ( destroyed() )
		return false;
	if ( isCloaked() )
		return false;

	return validateTarget( pTarget );
}

bool GadgetTachyonDisruptor::useActive() const
{
	return m_Target.valid();
}

//----------------------------------------------------------------------------

void GadgetTachyonDisruptor::use( dword when, Noun * pTarget, bool shift )
{
	if (! m_Target.valid() && pTarget )
	{
		NounGadget::use( when, pTarget, shift );

		m_Target = (NounBody *)pTarget;
		m_Target->setFlags(NounShip::FLAG_JUMP_DISABLED);
	}
	else
		m_Target = NULL;

	message( CharString().format( "<color;ffffff>Tactical: %s %s.", name(), useActive() ? "Active" : "Inactive" ) ); 
}

int	GadgetTachyonDisruptor::useEnergy( dword nTick, int energy )
{
	return energy;
}

//----------------------------------------------------------------------------

bool GadgetTachyonDisruptor::validateTarget( Noun * pTarget ) const
{
	if (! NounGame::validateTarget( pTarget ) )
		return false;
	
	if ( WidgetCast<NounShip>( pTarget ) == NULL )
		return false;	// only ships can be targeted

	NounShip * pTargetShip = (NounShip *)pTarget;
	if ( pTargetShip->jumpDrive() == NULL)
		return false;	// we can only target objects with jumpdrives

	// finally check the facing of the weapon
	return( checkFacing( pTarget->worldPosition() ) );
}

//----------------------------------------------------------------------------
// EOF
