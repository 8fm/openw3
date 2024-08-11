#pragma once

//////////////////////////////////////////////////////////////////////////
// For more info see graphs: http://msdn.microsoft.com/en-us/library/ee308751.aspx
enum EEasingType
{
    EET_In = 0, // Interpolation follows the mathematical formula associated with the easing function.
    EET_Out,    // Interpolation follows 100% interpolation minus the output of the formula associated with the easing function.
    EET_InOut   // Interpolation uses EaseIn for the first half of the animation and EaseOut for the second half.
};

//////////////////////////////////////////////////////////////////////////
// Interpolation function types...
enum ETransitionType
{
    EET_Linear,
    EET_Sine,
    EET_Cubic,
    EET_Quad,
    EET_Quart,
    EET_Quint,
    EET_Expo,
    EET_Circ,
    EET_Back,
    EET_Bounce,
    EET_Elastic,
    EET_Count
};

//////////////////////////////////////////////////////////////////////////

const static Float DAMPER_EPSILON = 0.0001f;
typedef Float (*EasingFunctionPtr)( Float, Float, Float, Float );

//////////////////////////////////////////////////////////////////////////

struct EasingFunctions
{
    static EasingFunctionPtr GetFunctionPtr( ETransitionType transition, EEasingType type );

    static Float BackEaseIn( Float currTime, Float startVal, Float destStartValueDiff, Float totalTime );
    static Float BackEaseOut( Float currTime, Float startVal, Float destStartValueDiff, Float totalTime );
    static Float BackEaseInOut( Float currTime, Float startVal, Float destStartValueDiff, Float totalTime );

    static Float BounceEaseIn( Float currTime, Float startVal, Float destStartValueDiff, Float totalTime );
    static Float BounceEaseOut( Float currTime, Float startVal, Float destStartValueDiff, Float totalTime );
    static Float BounceEaseInOut( Float currTime, Float startVal, Float destStartValueDiff, Float totalTime );

    static Float CircEaseIn( Float currTime, Float startVal, Float destStartValueDiff, Float totalTime );
    static Float CircEaseOut( Float currTime, Float startVal, Float destStartValueDiff, Float totalTime );
    static Float CircEaseInOut( Float currTime, Float startVal, Float destStartValueDiff, Float totalTime );

    static Float CubicEaseIn( Float currTime, Float startVal, Float destStartValueDiff, Float totalTime );
    static Float CubicEaseOut( Float currTime, Float startVal, Float destStartValueDiff, Float totalTime );
    static Float CubicEaseInOut( Float currTime, Float startVal, Float destStartValueDiff, Float totalTime );

    static Float ElasticEaseIn( Float currTime, Float startVal, Float destStartValueDiff, Float totalTime );
    static Float ElasticEaseOut( Float currTime, Float startVal, Float destStartValueDiff, Float totalTime );
    static Float ElasticEaseInOut( Float currTime, Float startVal, Float destStartValueDiff, Float totalTime );

    static Float ExpoEaseIn( Float currTime, Float startVal, Float destStartValueDiff, Float totalTime );
    static Float ExpoEaseOut( Float currTime, Float startVal, Float destStartValueDiff, Float totalTime );
    static Float ExpoEaseInOut( Float currTime, Float startVal, Float destStartValueDiff, Float totalTime );

    static Float QuadEaseIn( Float currTime, Float startVal, Float destStartValueDiff, Float totalTime );
    static Float QuadEaseOut( Float currTime, Float startVal, Float destStartValueDiff, Float totalTime );
    static Float QuadEaseInOut( Float currTime, Float startVal, Float destStartValueDiff, Float totalTime );

    static Float QuartEaseIn( Float currTime, Float startVal, Float destStartValueDiff, Float totalTime );
    static Float QuartEaseOut( Float currTime, Float startVal, Float destStartValueDiff, Float totalTime );
    static Float QuartEaseInOut( Float currTime, Float startVal, Float destStartValueDiff, Float totalTime );

    static Float QuintEaseIn( Float currTime, Float startVal, Float destStartValueDiff, Float totalTime );
    static Float QuintEaseOut( Float currTime, Float startVal, Float destStartValueDiff, Float totalTime );
    static Float QuintEaseInOut( Float currTime, Float startVal, Float destStartValueDiff, Float totalTime );

    static Float SineEaseIn( Float currTime, Float startVal, Float destStartValueDiff, Float totalTime );
    static Float SineEaseOut( Float currTime, Float startVal, Float destStartValueDiff, Float totalTime );
    static Float SineEaseInOut( Float currTime, Float startVal, Float destStartValueDiff, Float totalTime );

    static Float Linear( Float currTime, Float startVal, Float destStartValueDiff, Float totalTime );
};

//////////////////////////////////////////////////////////////////////////
