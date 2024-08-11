#include "build.h"
#include "easingFunctions.h"
#include "..\redMath\mathfunctions_fpu.h"

//////////////////////////////////////////////////////////////////////////

EasingFunctionPtr EasingFunctions::GetFunctionPtr( ETransitionType transition, EEasingType type )
{
    EasingFunctionPtr m_funPtr = nullptr;

    switch(transition)
    {
    case EET_Linear:
        {
            m_funPtr = &EasingFunctions::Linear;
        }
        break;
    case EET_Sine:
        {
            if( type == EET_In )
                m_funPtr = &EasingFunctions::SineEaseIn;
            else if( type == EET_Out )
                m_funPtr = &EasingFunctions::SineEaseOut;
            else
                m_funPtr = &EasingFunctions::SineEaseInOut;
        }
        break;
    case EET_Cubic:
        {
            if( type == EET_In )
                m_funPtr = &EasingFunctions::CubicEaseIn;
            else if( type == EET_Out )
                m_funPtr = &EasingFunctions::CubicEaseOut;
            else
                m_funPtr = &EasingFunctions::CubicEaseInOut;
        }
        break;
    case EET_Quad:
        {
            if( type == EET_In )
                m_funPtr = &EasingFunctions::QuadEaseIn;
            else if( type == EET_Out )
                m_funPtr = &EasingFunctions::QuadEaseOut;
            else
                m_funPtr = &EasingFunctions::QuadEaseInOut;
        }
        break;
    case EET_Quart:
        {
            if( type == EET_In )
                m_funPtr = &EasingFunctions::QuartEaseIn;
            else if( type == EET_Out )
                m_funPtr = &EasingFunctions::QuartEaseOut;
            else
                m_funPtr = &EasingFunctions::QuartEaseInOut;
        }
        break;
    case EET_Quint:
        {
            if( type == EET_In )
                m_funPtr = &EasingFunctions::QuintEaseIn;
            else if( type == EET_Out )
                m_funPtr = &EasingFunctions::QuintEaseOut;
            else
                m_funPtr = &EasingFunctions::QuintEaseInOut;
        }
        break;
    case EET_Expo:
        {
            if( type == EET_In )
                m_funPtr = &EasingFunctions::ExpoEaseIn;
            else if( type == EET_Out )
                m_funPtr = &EasingFunctions::ExpoEaseOut;
            else
                m_funPtr = &EasingFunctions::ExpoEaseInOut;
        }
        break;
    case EET_Circ:
        {
            if( type == EET_In )
                m_funPtr = &EasingFunctions::CircEaseIn;
            else if( type == EET_Out )
                m_funPtr = &EasingFunctions::CircEaseOut;
            else
                m_funPtr = &EasingFunctions::CircEaseInOut;
        }
        break;
    case EET_Back:
        {
            if( type == EET_In )
                m_funPtr = &EasingFunctions::BackEaseIn;
            else if( type == EET_Out )
                m_funPtr = &EasingFunctions::BackEaseOut;
            else
                m_funPtr = &EasingFunctions::BackEaseInOut;
        }
        break;
    case EET_Bounce:
        {
            if( type == EET_In )
                m_funPtr = &EasingFunctions::BounceEaseIn;
            else if( type == EET_Out )
                m_funPtr = &EasingFunctions::BounceEaseOut;
            else
                m_funPtr = &EasingFunctions::BounceEaseInOut;
        }
        break;
    case EET_Elastic:
        {
            if( type == EET_In )
                m_funPtr = &EasingFunctions::ElasticEaseIn;
            else if( type == EET_Out )
                m_funPtr = &EasingFunctions::ElasticEaseOut;
            else
                m_funPtr = &EasingFunctions::ElasticEaseInOut;
        }
        break;
    default:
        {
            m_funPtr = &EasingFunctions::Linear;
        }
        break;
    }

    return m_funPtr;
}

//////////////////////////////////////////////////////////////////////////

/***** LINEAR ****/
Float EasingFunctions::Linear( Float currTime, Float startVal, Float destStartValueDiff, Float totalTime )
{
    return destStartValueDiff * currTime / totalTime + startVal;
}

//////////////////////////////////////////////////////////////////////////
/***** SINE ****/

Float EasingFunctions::SineEaseIn( Float currTime, Float startVal, Float destStartValueDiff, Float totalTime )
{
    return -destStartValueDiff * MCos(currTime / totalTime * (M_PI_HALF)) + destStartValueDiff + startVal;
}

//////////////////////////////////////////////////////////////////////////

Float EasingFunctions::SineEaseOut( Float currTime, Float startVal, Float destStartValueDiff, Float totalTime )
{
    return destStartValueDiff * MSin(currTime / totalTime * (M_PI_HALF)) + startVal;
}

//////////////////////////////////////////////////////////////////////////

Float EasingFunctions::SineEaseInOut( Float currTime, Float startVal, Float destStartValueDiff, Float totalTime )
{
    return -destStartValueDiff / 2 * (MCos(M_PI * currTime / totalTime) - 1) + startVal;
}

//////////////////////////////////////////////////////////////////////////
/**** Quint ****/

Float EasingFunctions::QuintEaseIn( Float currTime, Float startVal, Float destStartValueDiff, Float totalTime )
{
    currTime /= totalTime;
    return destStartValueDiff * (currTime) * currTime * currTime * currTime * currTime + startVal;
}

//////////////////////////////////////////////////////////////////////////

Float EasingFunctions::QuintEaseOut( Float currTime, Float startVal, Float destStartValueDiff, Float totalTime )
{
    currTime = currTime / totalTime - 1;
    return destStartValueDiff * (currTime * currTime * currTime * currTime * currTime + 1) + startVal;
}

//////////////////////////////////////////////////////////////////////////

Float EasingFunctions::QuintEaseInOut( Float currTime, Float startVal, Float destStartValueDiff, Float totalTime )
{
    if ((currTime /= totalTime / 2) < 1)
    {
        return destStartValueDiff / 2 * currTime * currTime * currTime * currTime * currTime + startVal;
    }

    currTime -= 2;

    return destStartValueDiff / 2 * (currTime * currTime * currTime * currTime * currTime + 2) + startVal;
}

//////////////////////////////////////////////////////////////////////////
/**** Quart ****/

Float EasingFunctions::QuartEaseIn( Float currTime, Float startVal, Float destStartValueDiff, Float totalTime )
{
    currTime /= totalTime;
    return destStartValueDiff * (currTime) * currTime * currTime * currTime + startVal;
}

//////////////////////////////////////////////////////////////////////////

Float EasingFunctions::QuartEaseOut( Float currTime, Float startVal, Float destStartValueDiff, Float totalTime )
{
    currTime = currTime / totalTime - 1;
    return -destStartValueDiff * ((currTime) * currTime * currTime * currTime - 1) + startVal;
}

//////////////////////////////////////////////////////////////////////////

Float EasingFunctions::QuartEaseInOut( Float currTime, Float startVal, Float destStartValueDiff, Float totalTime )
{
    if ((currTime /= totalTime / 2) < 1)
    {
        return destStartValueDiff / 2 * currTime * currTime * currTime * currTime + startVal;
    }

    currTime -= 2;

    return -destStartValueDiff / 2 * ((currTime) * currTime * currTime * currTime - 2) + startVal;
}

//////////////////////////////////////////////////////////////////////////
/**** Quad ****/

Float EasingFunctions::QuadEaseIn( Float currTime, Float startVal, Float destStartValueDiff, Float totalTime )
{
    currTime /= totalTime;
    return destStartValueDiff * (currTime) * currTime + startVal;
}

//////////////////////////////////////////////////////////////////////////

Float EasingFunctions::QuadEaseOut( Float currTime, Float startVal, Float destStartValueDiff, Float totalTime )
{
    currTime /= totalTime;
    return -destStartValueDiff * (currTime) * (currTime - 2) + startVal;
}

//////////////////////////////////////////////////////////////////////////

Float EasingFunctions::QuadEaseInOut( Float currTime, Float startVal, Float destStartValueDiff, Float totalTime )
{
    if ((currTime /= totalTime / 2) < 1)
    {
        return ((destStartValueDiff / 2) * (currTime * currTime)) + startVal;
    }
    // TODO hmm.
    return -destStartValueDiff / 2 * (((currTime - 1) * (currTime - 3)) - 1) + startVal;
    /*
    originally return -c/2 * (((t-2)*(--t)) - 1) + b;
    */
}

//////////////////////////////////////////////////////////////////////////
/**** Expo ****/

Float EasingFunctions::ExpoEaseIn( Float currTime, Float startVal, Float destStartValueDiff, Float totalTime )
{
    return (currTime == 0) ? startVal : destStartValueDiff * Red::Math::MPow(2, 10 * (currTime / totalTime - 1)) + startVal;
}

//////////////////////////////////////////////////////////////////////////

Float EasingFunctions::ExpoEaseOut( Float currTime, Float startVal, Float destStartValueDiff, Float totalTime )
{
    return (currTime == totalTime) ? startVal + destStartValueDiff : destStartValueDiff * (-Red::Math::MPow(2, -10 * currTime / totalTime) + 1) + startVal;
}

//////////////////////////////////////////////////////////////////////////

Float EasingFunctions::ExpoEaseInOut( Float currTime, Float startVal, Float destStartValueDiff, Float totalTime )
{
    if (currTime == 0)
    {
        return startVal;
    }

    if (currTime == totalTime)
    {
        return startVal + destStartValueDiff;
    }

    if ((currTime /= totalTime / 2) < 1)
    {
        return destStartValueDiff / 2 * Red::Math::MPow(2, 10 * (currTime - 1)) + startVal;
    }

    return destStartValueDiff / 2 * (-Red::Math::MPow(2, -10 * --currTime) + 2) + startVal;
}

//////////////////////////////////////////////////////////////////////////
/****  Elastic ****/

Float EasingFunctions::ElasticEaseIn( Float currTime, Float startVal, Float destStartValueDiff, Float totalTime )
{
    if (currTime == 0)
    {
        return startVal;
    }

    if ((currTime /= totalTime) == 1)
    {
        return startVal + destStartValueDiff;
    }

    Float p = totalTime * .3f;
    Float a = destStartValueDiff;
    Float s = p / 4;
    Float postFix = a * Red::Math::MPow(2, 10 * (currTime -= 1)); // this is a fix, again, with post-increment operators

    return -(postFix * MSin((currTime * totalTime - s) * (M_PI_TWO) / p)) + startVal;
}

//////////////////////////////////////////////////////////////////////////

Float EasingFunctions::ElasticEaseOut( Float currTime, Float startVal, Float destStartValueDiff, Float totalTime )
{
    if (currTime == 0)
    {
        return startVal;
    }
    if ((currTime /= totalTime) == 1)
    {
        return startVal + destStartValueDiff;
    }

    Float p = totalTime * .3f;
    Float a = destStartValueDiff;
    Float s = p / 4;

    return (a * Red::Math::MPow(2, -10 * currTime) * MSin((currTime * totalTime - s) * (M_PI_TWO) / p) + destStartValueDiff + startVal);
}

//////////////////////////////////////////////////////////////////////////

Float EasingFunctions::ElasticEaseInOut( Float currTime, Float startVal, Float destStartValueDiff, Float totalTime )
{
    if (currTime == 0)
    {
        return startVal;
    }

    if ((currTime /= totalTime / 2) == 2)
    {
        return startVal + destStartValueDiff;
    }

    Float p = totalTime * (.3f * 1.5f);
    Float a = destStartValueDiff;
    Float s = p / 4;

    if (currTime < 1)
    {
        Float postFix = a * Red::Math::MPow(2, 10 * (currTime -= 1)); // postIncrement is evil

        return -.5f * (postFix * MSin((currTime * totalTime - s) * (M_PI_TWO) / p)) + startVal;
    }

    Float postFix = a * Red::Math::MPow(2, -10 * (currTime -= 1)); // postIncrement is evil

    return postFix * MSin((currTime * totalTime - s) * (M_PI_TWO) / p) * .5f + destStartValueDiff + startVal;
}

//////////////////////////////////////////////////////////////////////////
/****  Cubic ****/

Float EasingFunctions::CubicEaseIn( Float currTime, Float startVal, Float destStartValueDiff, Float totalTime )
{
    currTime /= totalTime;
    return destStartValueDiff * (currTime) * currTime * currTime + startVal;
}

//////////////////////////////////////////////////////////////////////////

Float EasingFunctions::CubicEaseOut( Float currTime, Float startVal, Float destStartValueDiff, Float totalTime )
{
    currTime = currTime / totalTime - 1;
    return destStartValueDiff * ((currTime) * currTime * currTime + 1) + startVal;
}

//////////////////////////////////////////////////////////////////////////

Float EasingFunctions::CubicEaseInOut( Float currTime, Float startVal, Float destStartValueDiff, Float totalTime )
{
    if ((currTime /= totalTime / 2) < 1)
    {
        return destStartValueDiff / 2 * currTime * currTime * currTime + startVal;
    }

    currTime -= 2;
    return destStartValueDiff / 2 * ((currTime) * currTime * currTime + 2) + startVal;
}

//////////////////////////////////////////////////////////////////////////
/*** Circ ***/

Float EasingFunctions::CircEaseIn( Float currTime, Float startVal, Float destStartValueDiff, Float totalTime )
{
    currTime /= totalTime;
    return -destStartValueDiff * (MSqrt(1 - (currTime) * currTime) - 1) + startVal;
}

//////////////////////////////////////////////////////////////////////////

Float EasingFunctions::CircEaseOut( Float currTime, Float startVal, Float destStartValueDiff, Float totalTime )
{
    currTime = currTime / totalTime - 1;
    return destStartValueDiff * MSqrt(1 - (currTime) * currTime) + startVal;
}

//////////////////////////////////////////////////////////////////////////

Float EasingFunctions::CircEaseInOut( Float currTime, Float startVal, Float destStartValueDiff, Float totalTime )
{
    if ((currTime /= totalTime / 2) < 1)
    {
        return -destStartValueDiff / 2 * (MSqrt(1 - currTime * currTime) - 1) + startVal;
    }

    (currTime -= 2);

    return destStartValueDiff / 2 * (MSqrt(1 - currTime * currTime) + 1) + startVal;
}

//////////////////////////////////////////////////////////////////////////
/****  Bounce ****/

Float EasingFunctions::BounceEaseIn( Float currTime, Float startVal, Float destStartValueDiff, Float totalTime )
{
    return destStartValueDiff - BounceEaseOut(totalTime - currTime, 0, destStartValueDiff, totalTime) + startVal;
}

//////////////////////////////////////////////////////////////////////////

Float EasingFunctions::BounceEaseOut( Float currTime, Float startVal, Float destStartValueDiff, Float totalTime )
{
    if ((currTime /= totalTime) < (1 / 2.75f))
    {
        return destStartValueDiff * (7.5625f * currTime * currTime) + startVal;
    }
    else if (currTime < (2 / 2.75f))
    {
        Float postFix = currTime -= (1.5f / 2.75f);
        return destStartValueDiff * (7.5625f * (postFix) * currTime + .75f) + startVal;
    }
    else if (currTime < (2.5 / 2.75))
    {
        Float postFix = currTime -= (2.25f / 2.75f);
        return destStartValueDiff * (7.5625f * (postFix) * currTime + .9375f) + startVal;
    }
    else
    {
        Float postFix = currTime -= (2.625f / 2.75f);
        return destStartValueDiff * (7.5625f * (postFix) * currTime + .984375f) + startVal;
    }
}

//////////////////////////////////////////////////////////////////////////

Float EasingFunctions::BounceEaseInOut( Float currTime, Float startVal, Float destStartValueDiff, Float totalTime )
{
    if (currTime < totalTime / 2)
    {
        return BounceEaseIn(currTime * 2, 0, destStartValueDiff, totalTime) * .5f + startVal;
    }
    else
    {
        return BounceEaseOut(currTime * 2 - totalTime, 0, destStartValueDiff, totalTime) * .5f + destStartValueDiff * .5f + startVal;
    }
}

//////////////////////////////////////////////////////////////////////////
/**** Back *****/

Float EasingFunctions::BackEaseIn( Float currTime, Float startVal, Float destStartValueDiff, Float totalTime )
{
    Float s = 1.70158f;
    Float postFix = currTime /= totalTime;
    return destStartValueDiff * (postFix) * currTime * ((s + 1) * currTime - s) + startVal;
}

//////////////////////////////////////////////////////////////////////////

Float EasingFunctions::BackEaseOut( Float currTime, Float startVal, Float destStartValueDiff, Float totalTime )
{
    Float s = 1.70158f;
    currTime = currTime / totalTime - 1;
    return destStartValueDiff * ((currTime) * currTime * ((s + 1) * currTime + s) + 1) + startVal;
}

//////////////////////////////////////////////////////////////////////////

Float EasingFunctions::BackEaseInOut( Float currTime, Float startVal, Float destStartValueDiff, Float totalTime )
{
    Float s = 1.70158f;
    if ((currTime /= totalTime / 2) < 1)
    {
        s *= (1.525f);
        return destStartValueDiff / 2 * (currTime * currTime * (((s) + 1) * currTime - s)) + startVal;
    }
    Float postFix = currTime -= 2;
    s *= (1.525f);
    return destStartValueDiff / 2 * ((postFix) * currTime * (((s) + 1) * currTime + s) + 2) + startVal;
}

//////////////////////////////////////////////////////////////////////////