/**************************************************************************

Filename    :   GL_HAL.cpp
Content     :   GL Renderer HAL Prototype implementation.
Created     :   
Authors     :   

Copyright   :   Copyright 2011 Autodesk, Inc. All Rights reserved.

Use of this software is subject to the terms of the Autodesk license
agreement provided at the time of installation or download, or which
otherwise accompanies this software in either electronic or hard copy form.

**************************************************************************/

#if !defined(SF_USE_GLES)   // Do not compile under GLES 1.1

#include "Kernel/SF_Debug.h"
#include "Kernel/SF_Random.h"
#include "Kernel/SF_HeapNew.h"  
#include "Kernel/SF_Debug.h"

#include "Render/Render_TextureCacheGeneric.h"
#include "Render/Render_BufferGeneric.h"
#include "Render/GL/GL_HAL.h"
#include "Render/GL/GL_Events.h"

namespace Scaleform { namespace Render { namespace GL {


// ***** RenderHAL_GL

HAL::HAL(ThreadCommandQueue* commandQueue)
:   Render::ShaderHAL<ShaderManager, ShaderInterface>(commandQueue),
    EnabledVertexArrays(-1),
    Cache(Memory::GetGlobalHeap(), MeshCacheParams::PC_Defaults),
    RSync(GetHAL()),
    PrevBatchType(PrimitiveBatch::DP_None),
    Caps(SManager.Caps),
    MajorVersion(0), MinorVersion(0),
    DeterminedDepthStencilFormat(false)
{
}

HAL::~HAL()
{
    ShutdownHAL();
}

Render::RenderSync* HAL::GetRenderSync()
{
    // Only actually return the render sync object, if the MeshCache is using unsynchronized buffer updates. 
    // Fencing is not useful otherwise, but it might have performance implications if used regardless. Returning 
    // NULL from this function will act as though fencing is not supported.
    if (Cache.GetBufferUpdateType() == MeshCache::BufferUpdate_MapBufferUnsynchronized)
        return (Render::RenderSync*)(&RSync);
    else
        return 0;
}

// *** RenderHAL_GL Implementation

bool HAL::InitHAL(const GL::HALInitParams& params)
{
    // Clear the error stack.
    glGetError();

#ifdef SF_GL_RUNTIME_LINK
    Extensions::Init();
#endif

    // Check versions and extensions immediately, to make sure they are initialized.
    CheckExtension(0);
    CheckGLVersion(0,0);

    // Check for GL capabilities.
    Caps = 0;

#if defined(SF_USE_GLES2)
    const char *ren = (const char*) glGetString(GL_RENDERER);
    const char *ven = (const char*) glGetString(GL_VENDOR);
    
    // Check for sync.
    if (CheckExtension("GL_APPLE_sync"))
        Caps |= Cap_Sync;

    // Check for map buffer range (must have sync as well)
    if ((Caps & Cap_Sync) && CheckExtension("GL_EXT_map_buffer_range"))
        Caps |= Cap_MapBufferRange;

    // Check for map buffer
    if (CheckExtension("GL_OES_mapbuffer"))
        Caps |= Cap_MapBuffer;
    
    // GLES2 has glBufferSubData built in.
        Caps |= Cap_BufferUpdate;
    
    // Check for Vivante GPU and if found disable batching.
	// TODO: Find out why batching doesn't work
    if (!strncmp(ven, "Vivante", 7)) 
        Caps |= Cap_NoBatching;

    // Check for binary shaders, but do not use them on PowerVR or Vivante drivers.
    if (CheckExtension("GL_OES_get_program_binary") && strncmp(ren, "PowerVR", 7) && strncmp(ven, "Vivante", 7))
        Caps |= Cap_BinaryShaders;

    // Check if derivatives (dFdx/dFdy) are not supported.
    if (!CheckExtension("GL_OES_standard_derivatives"))
        Caps |= Cap_NoDerivatives;
    
	// Force vertex alignment on GLES2 devices. Vertex alignment is preferred for performance reasons.
	Caps |= MVF_Align;
    
#else
    // OpenGL (desktop)
    
    // Check for binary shaders
    if (CheckExtension("GL_ARB_get_program_binary"))
        Caps |= Cap_BinaryShaders;

    // Check for sync.
    if (CheckExtension("GL_ARB_sync") && CheckGLVersion(3,1))
        Caps |= Cap_Sync;
        
    // Check for map buffer range (must have sync as well)
    // TODO: enable MapBufferRange update method on OpenGL, it currently does function as expected.
    // Check that the things we need for MapBufferRange are available.
    //if ((Caps & Cap_Sync) && CheckExtension("GL_ARB_map_buffer_range"))
    //    Caps |= Cap_MapBufferRange;
    
    Caps |= Cap_BufferUpdate; // Part of GL 2.1 spec.
    Caps |= Cap_MapBuffer;    // Part of GL 2.1 spec.
    
    Caps |= MVF_Align;        // Align for performance

    // Check for instancing (glDrawElementsInstanced is available).
    if ( (CheckGLVersion(3,0) || CheckExtension("GL_ARB_draw_instanced")) && !params.NoInstancing)
        Caps |= Cap_Instancing;
    
#endif
    
    if (params.NoVAO)
    {
        Caps |= Cap_NoVAO;
    }
 
    // Call the base-initialization after the Caps have been initialized.
    if ( !initHAL(params))
        return false;

    // If requested, turn on GL driver debugging messages (all of them).
    if (params.ConfigFlags&HALConfig_DebugMessages)
    {
        if (CheckExtension("GL_ARB_debug_output") || CheckExtension("GL_KHR_debug"))
        {
            glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
            glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, 0, true);
            glDebugMessageCallback((GLDEBUGPROC)DebugMessageCallback, 0);
        }
        else
        {
            SF_DEBUG_WARNING(1, "HALConfig_DebugMessages was specified in InitHALParams, but neither "
                "GL_ARB_debug_output or GL_KHR_debug extensions are not available\n");
        }
    }

    GLint maxUniforms = 128;
#if defined(SF_USE_GLES2)
    #if defined(GL_MAX_VERTEX_UNIFORM_VECTORS)
        glGetIntegerv(GL_MAX_VERTEX_UNIFORM_VECTORS, &maxUniforms);
    #endif
#else
    glGetIntegerv(GL_MAX_VERTEX_UNIFORM_COMPONENTS, &maxUniforms);
#endif

#if defined(SF_OS_ANDROID)
    // Reports 128 uniforms but some drivers crash when more than 64 are used. The 544 experiences
    // extreme corruption when using more than 32. These driver problems only seem to be present on 
    // Android PVR chips, so handicap them for now (Vivante too it seems).
    if (!strncmp(ren, "PowerVR SGX 5", 12))
        maxUniforms = 64;
    if (!strncmp(ren, "PowerVR SGX 544", 14) || !strncmp(ven, "Vivante", 7)) 
        maxUniforms = 32;
#endif

    Caps |= maxUniforms << Cap_MaxUniforms_Shift;

    // Disable the usage of texture density profile mode, if derivatives are not available.
    if (Caps & Cap_NoDerivatives)
        GetProfiler().SetModeAvailability((unsigned)(Profile_All & ~Profile_TextureDensity));

    SManager.SetBinaryShaderPath(params.BinaryShaderPath);

    GLint maxAttributes;
    glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &maxAttributes);

    SF_DEBUG_MESSAGE1(1, "GL_VENDOR                   = %s\n", (const char*)glGetString(GL_VENDOR));
    SF_DEBUG_MESSAGE1(1, "GL_VERSION                  = %s\n", (const char*)glGetString(GL_VERSION));
    SF_DEBUG_MESSAGE1(1, "GL_RENDERER                 = %s\n", (const char*)glGetString(GL_RENDERER));
    SF_DEBUG_MESSAGE1(1, "GL_SHADING_LANGUAGE_VERSION = %s\n", (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION));
    SF_DEBUG_MESSAGE1(1, "GL_MAX_VERTEX_ATTRIBS       = %d\n", maxAttributes);
    SF_DEBUG_MESSAGE1(1, "GL_EXTENSIONS               = %s\n", Extensions.ToCStr());
    SF_DEBUG_MESSAGE1(1, "GL_CAPS                     = 0x%x\n", Caps);

    // In GLES 2.0, print out the plane bit-depths.
#if defined(SF_USE_GLES2)
    GLint rgbaBits[4], stencilBits, depthBits;
    glGetIntegerv(GL_RED_BITS, &rgbaBits[0]);
    glGetIntegerv(GL_GREEN_BITS, &rgbaBits[1]);
    glGetIntegerv(GL_BLUE_BITS, &rgbaBits[2]);
    glGetIntegerv(GL_ALPHA_BITS, &rgbaBits[3]);
    glGetIntegerv(GL_STENCIL_BITS, &stencilBits);
    glGetIntegerv(GL_DEPTH_BITS, &depthBits);
    SF_DEBUG_MESSAGE6(1, "GL_x_BITS                   = R%dG%dB%dA%d, D%dS%d\n", rgbaBits[0], rgbaBits[1], rgbaBits[2], rgbaBits[3], depthBits, stencilBits);
#endif

    pTextureManager = params.GetTextureManager();
    if (!pTextureManager)
    {
        Ptr<TextureCacheGeneric> textureCache = 0;

        // On GLES, create a texture cache, with the default size. Otherwise, do not use texture caching.
#if defined(SF_USE_GLES_ANY)
        textureCache = *SF_NEW TextureCacheGeneric();
#endif
        pTextureManager = 
            *SF_HEAP_AUTO_NEW(this) TextureManager(params.RenderThreadId, pRTCommandQueue, textureCache);
    }
    pTextureManager->Initialize(this);

    // Allocate our matrix state
    Matrices = *SF_HEAP_AUTO_NEW(this) MatrixState(this);


    pRenderBufferManager = params.pRenderBufferManager;
    if (!pRenderBufferManager)
    {
        pRenderBufferManager = *SF_HEAP_AUTO_NEW(this) RenderBufferManagerGeneric();
        if ( !pRenderBufferManager || !createDefaultRenderBuffer())
        {
            ShutdownHAL();
            return false;
        }
    }

    if (!SManager.Initialize(this, VMCFlags) ||
        !Cache.Initialize(this))
        return false;

    HALState|= HS_ModeSet;    
    notifyHandlers(HAL_Initialize);
    return true;
}

// Returns back to original mode (cleanup)
bool HAL::ShutdownHAL()
{
    if (!(HALState & HS_ModeSet))
        return true;

    if (!shutdownHAL())
        return false;

    destroyRenderBuffers();
    pRenderBufferManager.Clear();
    pTextureManager->ProcessQueues();
    pTextureManager.Clear();
    Cache.Reset();
    SManager.Reset();

    // Reset these, incase the HAL is started again with a different GL profile.
    MajorVersion = 0;
    MinorVersion = 0;
    Extensions.Clear();

    return true;
}

void HAL::initMatrices()
{
    // Allocate our matrix state
    Matrices = *SF_HEAP_AUTO_NEW(this) MatrixState(this);
}

bool HAL::ResetContext()
{
    notifyHandlers(HAL_PrepareForReset);
    pTextureManager->NotifyLostContext();
    Cache.Reset(true);
    SManager.Reset();
    ShaderData.ResetContext();

#ifdef SF_GL_RUNTIME_LINK
    Extensions::Init();
#endif

    pTextureManager->Initialize(this);

    if (!SManager.Initialize(this, VMCFlags))
        return false;

    if (!Cache.Initialize(this))
        return false;

    if (pRenderBufferManager)
        pRenderBufferManager->Reset();

    notifyHandlers(HAL_RestoreAfterReset);
    return true;
}


// ***** Rendering

bool HAL::BeginFrame()
{
    // Clear the error state.
    glGetError();
    return Render::HAL::BeginFrame();
}

// Set states not changed in our rendering, or that are reset after changes
bool HAL::BeginScene()
{
    if ( !Render::HAL::BeginScene())
        return false;

    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glStencilMask(0xffffffff);

#if defined(GL_ALPHA_TEST)
    if (!CheckGLVersion(3,0))
        glDisable(GL_ALPHA_TEST);
#endif

    BlendEnable = -1;

    if (!ShouldUseVAOs())
    {
        // Reset vertex array usage (in case it changed between frames).
        EnabledVertexArrays = -1;
        GLint va;
        glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &va);
        for (int i = 0; i < va; i++)
            glDisableVertexAttribArray(i);
    }
    return true;
}

bool HAL::EndScene()
{
    if ( !Render::HAL::EndScene())
        return false;

    // Unbind the current VAO, so it doesn't get modified if this is an index buffer.
    if (ShouldUseVAOs())
    {
        glBindVertexArray(0);
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glUseProgram(0);

    return true;
}


void HAL::beginDisplay(BeginDisplayData* data)
{
    glDisable(GL_STENCIL_TEST);

    Render::HAL::beginDisplay(data);
}

// Updates HW Viewport and ViewportMatrix based on provided viewport
// and view rectangle.
void HAL::updateViewport()
{
    Viewport vp;

    if (HALState & HS_ViewValid)
    {
        int dx = ViewRect.x1 - VP.Left,
            dy = ViewRect.y1 - VP.Top;

        // Modify HW matrix and viewport to clip.
        CalcHWViewMatrix(VP.Flags, &Matrices->View2D, ViewRect, dx, dy);
        Matrices->SetUserMatrix(Matrices->User);
        Matrices->ViewRect    = ViewRect;
        Matrices->UVPOChanged = 1;

        if ( HALState & HS_InRenderTarget )
        {
            glViewport(VP.Left, VP.Top, VP.Width, VP.Height);
            glDisable(GL_SCISSOR_TEST);
        }
        else
        {
            vp = VP;
            vp.Left     = ViewRect.x1;
            vp.Top      = ViewRect.y1;
            vp.Width    = ViewRect.Width();
            vp.Height   = ViewRect.Height();
            vp.SetStereoViewport(Matrices->S3DDisplay);
            glViewport(vp.Left, VP.BufferHeight-vp.Top-vp.Height, vp.Width, vp.Height);
            if (VP.Flags & Viewport::View_UseScissorRect)
            {
                glEnable(GL_SCISSOR_TEST);
                glScissor(VP.ScissorLeft, VP.BufferHeight-VP.ScissorTop-VP.ScissorHeight, VP.ScissorWidth, VP.ScissorHeight);
            }
            else
            {
                glDisable(GL_SCISSOR_TEST);
            }
        }
    }
    else
    {
        glViewport(0,0,0,0);
    }

    // Workaround: it appears that when changing FBOs, the Tegra 3 will lose the current shader program
    // binding, and crash when rendering the next primitive. updateViewport is always called when GFx changes
    // FBOs, so, clear the cached shader program, so that next time it is requested, it will actually be set.
    // This should only result in a minimal amount of redundant state-sets.
    ShaderData.BeginScene();
}

void   HAL::MapVertexFormat(PrimitiveFillType fill, const VertexFormat* sourceFormat,
                            const VertexFormat** single,
                            const VertexFormat** batch, const VertexFormat** instanced, unsigned)
{
    unsigned instancingFlag = (Caps&Cap_Instancing ? MVF_HasInstancing : 0);
    SManager.MapVertexFormat(fill, sourceFormat, single, batch, instanced, (Caps&MVF_Align)| instancingFlag);
    if (Caps & Cap_NoBatching)
        *batch = 0;
}

void HAL::FinishFrame()
{
#if defined(SF_USE_GLES_ANY)
    glFinish();
#endif
}

void HAL::applyDepthStencilMode(DepthStencilMode mode, unsigned stencilRef)
{
    ScopedRenderEvent GPUEvent(GetEvent(Event_ApplyDepthStencil), __FUNCTION__);
    static GLenum DepthStencilCompareFunctions[DepthStencilFunction_Count] =
    {
        GL_NEVER,           // Ignore
        GL_NEVER,           // Never
        GL_LESS,            // Less
        GL_EQUAL,           // Equal
        GL_LEQUAL,          // LessEqual
        GL_GREATER,         // Greater
        GL_NOTEQUAL,        // NotEqual
        GL_GEQUAL,          // GreaterEqual
        GL_ALWAYS,          // Always
    };
    static GLenum StencilOps[StencilOp_Count] =
    {
        GL_KEEP,      // Ignore
        GL_KEEP,      // Keep
        GL_REPLACE,   // Replace
        GL_INCR,      // Increment        
    };

    const HALDepthStencilDescriptor& oldState = DepthStencilModeTable[CurrentDepthStencilState];
    const HALDepthStencilDescriptor& newState = DepthStencilModeTable[mode];

    // Apply the modes now.
    if (oldState.ColorWriteEnable != newState.ColorWriteEnable)
    {
        if (newState.ColorWriteEnable)
            glColorMask(1,1,1,1);
        else
            glColorMask(0,0,0,0);
    }

    if (oldState.StencilEnable != newState.StencilEnable)
    {
        if (newState.StencilEnable)
            glEnable(GL_STENCIL_TEST);
        else
            glDisable(GL_STENCIL_TEST);
    }

    // Only need to set stencil pass/fail ops if stenciling is actually enabled.
    if (newState.StencilEnable)
    {
        // No redundancy checking on stencil ref/write mask.
        glStencilFunc(DepthStencilCompareFunctions[newState.StencilFunction], stencilRef, 0XFF);

        if ((oldState.StencilFailOp != newState.StencilFailOp &&
            newState.StencilFailOp != HAL::StencilOp_Ignore) ||
            (oldState.StencilPassOp != newState.StencilPassOp &&
            newState.StencilPassOp!= HAL::StencilOp_Ignore) ||
            (oldState.StencilZFailOp != newState.StencilZFailOp &&
            newState.StencilZFailOp != HAL::StencilOp_Ignore))
        {
            glStencilOp(StencilOps[newState.StencilFailOp], StencilOps[newState.StencilZFailOp], StencilOps[newState.StencilPassOp]);
        }
    }

    // If the value of depth test/write change, we may have to change the value of ZEnable.
    if ((oldState.DepthTestEnable || oldState.DepthWriteEnable) != 
        (newState.DepthTestEnable || newState.DepthWriteEnable))
    {
        if ((newState.DepthTestEnable || newState.DepthWriteEnable))
            glEnable(GL_DEPTH_TEST);
        else
            glDisable(GL_DEPTH_TEST);

        // Only need to set the function, if depth testing is enabled.
        if (newState.DepthTestEnable)
        {
            if (oldState.DepthFunction != newState.DepthFunction &&
                newState.DepthFunction != HAL::DepthStencilFunction_Ignore)
            {
                glDepthFunc(DepthStencilCompareFunctions[newState.DepthFunction]);
            }
        }
    }

    if (oldState.DepthWriteEnable != newState.DepthWriteEnable)
    {
        glDepthMask(newState.DepthWriteEnable ? GL_TRUE : GL_FALSE);
    }

    CurrentDepthStencilState = mode;
}

bool HAL::checkDepthStencilBufferCaps()
{
    if (!StencilChecked)
    {
        GLint currentFBO;
        glGetIntegerv(GL_FRAMEBUFFER_BINDING, &currentFBO);

        if (currentFBO != 0)
        {
            GLint stencilType, stencilBits;
            glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, &stencilType);
            if (stencilType == GL_NONE)
            {
                stencilBits = 0;
            }
            else
            {
                GLint stencilName;
                glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &stencilName);
                glBindRenderbuffer(GL_RENDERBUFFER, stencilName);
                if (stencilType == GL_RENDERBUFFER)
                    glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_STENCIL_SIZE, &stencilBits);
                else
                    stencilBits = 8;
                glBindRenderbuffer(GL_RENDERBUFFER, 0);
            }

            if (stencilBits > 0)
            {
                StencilAvailable = true;
                MultiBitStencil = (stencilBits > 1);
            }

            // Check for depth buffer.
            GLint depthType, depthBits;
            glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, &depthType);
            if (depthType == GL_NONE)
            {
                depthBits = 0;
            }
            else
            {
                GLint depthName;
                glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &depthName);
                glBindRenderbuffer(GL_RENDERBUFFER, depthName);
                if (depthType == GL_RENDERBUFFER)
                    glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_DEPTH_SIZE, &depthBits);
                else
                    depthBits = 8;
                glBindRenderbuffer(GL_RENDERBUFFER, 0);
            }
            DepthBufferAvailable = (depthBits >= 1);
        }
        else
        {
            // If we are using the default FBO, assume we have everything. TBD: this should be overridable in HALInitParams
            StencilAvailable = true;
            DepthBufferAvailable = true;
            MultiBitStencil = true;
        }
        StencilChecked = 1;
    }   

    SF_DEBUG_WARNONCE(!StencilAvailable && !DepthBufferAvailable, 
        "RendererHAL::PushMask_BeginSubmit used, but neither stencil or depth buffer is available");
    return (StencilAvailable || DepthBufferAvailable);
}

bool HAL::IsRasterModeSupported(RasterModeType mode) const
{
    SF_UNUSED(mode);
#if !defined(SF_USE_GLES_ANY)
    // OpenGL supports all.
    return true;
#else
    // GLES supports none
    return false;
#endif
}

void HAL::applyRasterModeImpl(RasterModeType mode)
{
#if !defined(SF_USE_GLES_ANY)
    GLenum fillMode;
    switch(mode)
    {
        default:
        case RasterMode_Solid:      fillMode = GL_FILL; break;
        case RasterMode_Wireframe:  fillMode = GL_LINE; break;
        case RasterMode_Point:      fillMode = GL_POINT; break;
    }
    glPolygonMode(GL_FRONT_AND_BACK, fillMode);
#else
    SF_UNUSED(mode);
#endif
}



//--------------------------------------------------------------------
// Background clear helper, expects viewport coordinates.
void HAL::clearSolidRectangle(const Rect<int>& r, Color color, bool blend)
{
    if ((!blend || color.GetAlpha() == 0xFF) && !(VP.Flags & Viewport::View_Stereo_AnySplit))
    {
        ScopedRenderEvent GPUEvent(GetEvent(Event_Clear), "HAL::clearSolidRectangle"); // NOTE: inside scope, base impl has its own profile.

        glEnable(GL_SCISSOR_TEST);

        PointF tl((float)(VP.Left + r.x1), (float)(VP.Top + r.y1));
        PointF br((float)(VP.Left + r.x2), (float)(VP.Top + r.y2));
        tl = Matrices->Orient2D * tl;
        br = Matrices->Orient2D * br;
        Rect<int> scissor((int)Alg::Min(tl.x, br.x), (int)Alg::Min(tl.y,br.y), (int)Alg::Max(tl.x,br.x), (int)Alg::Max(tl.y,br.y));
        glScissor(scissor.x1, scissor.y1, scissor.Width(), scissor.Height());
        glClearColor(color.GetRed() * 1.f/255.f, color.GetGreen() * 1.f/255.f, color.GetBlue() * 1.f/255.f, color.GetAlpha() * 1.f/255.f);
        glClear(GL_COLOR_BUFFER_BIT);

        if (VP.Flags & Viewport::View_UseScissorRect)
        {
            glEnable(GL_SCISSOR_TEST);
            glScissor(VP.ScissorLeft, VP.BufferHeight-VP.ScissorTop-VP.ScissorHeight, VP.ScissorWidth, VP.ScissorHeight);
        }
        else
        {
            glDisable(GL_SCISSOR_TEST);
        }
    }
    else
    {
        BaseHAL::clearSolidRectangle(r, color, blend);
    }
}

//--------------------------------------------------------------------
// *** Mask / Stencil support
//--------------------------------------------------------------------


//--------------------------------------------------------------------
// *** BlendMode Stack support
//--------------------------------------------------------------------

void HAL::applyBlendModeImpl(BlendMode mode, bool sourceAc, bool forceAc)
{    
    static const UInt32 BlendOps[BlendOp_Count] = 
    {
        GL_FUNC_ADD,                // BlendOp_ADD
        GL_MAX,                     // BlendOp_MAX
        GL_MIN,                     // BlendOp_MIN
        GL_FUNC_REVERSE_SUBTRACT,   // BlendOp_REVSUBTRACT
    };

    static const UInt32 BlendFactors[BlendFactor_Count] = 
    {
        GL_ZERO,                // BlendFactor_ZERO
        GL_ONE,                 // BlendFactor_ONE
        GL_SRC_ALPHA,           // BlendFactor_SRCALPHA
        GL_ONE_MINUS_SRC_ALPHA, // BlendFactor_INVSRCALPHA
        GL_DST_COLOR,           // BlendFactor_DESTCOLOR
        GL_ONE_MINUS_DST_COLOR, // BlendFactor_INVDESTCOLOR
    };

    GLenum sourceColor = BlendFactors[BlendModeTable[mode].SourceColor];
    if ( sourceAc && sourceColor == GL_SRC_ALPHA )
        sourceColor = GL_ONE;

    if (VP.Flags & Viewport::View_AlphaComposite || forceAc)
    {
        glBlendFuncSeparate(sourceColor, BlendFactors[BlendModeTable[mode].DestColor], 
            BlendFactors[BlendModeTable[mode].SourceAlpha], BlendFactors[BlendModeTable[mode].DestAlpha]);
        glBlendEquationSeparate(BlendOps[BlendModeTable[mode].Operator], BlendOps[BlendModeTable[mode].AlphaOperator]);
    }
    else
    {
        glBlendFunc(sourceColor, BlendFactors[BlendModeTable[mode].DestColor]);
        glBlendEquation(BlendOps[BlendModeTable[mode].Operator]);
    }
}

void HAL::applyBlendModeEnableImpl(bool enabled)
{
    if (enabled)
        glEnable(GL_BLEND);
    else
        glDisable(GL_BLEND);
}

RenderTarget* HAL::CreateRenderTarget(GLuint fbo)
{
    GLint currentFbo;
    RenderTarget* prt = pRenderBufferManager->CreateRenderTarget(getFboInfo(fbo, currentFbo, false), RBuffer_User, Image_R8G8B8A8, 0);
    if ( !prt )
        return 0;
    if ( prt->GetRenderTargetData() != 0 )
        return prt;

    RenderTargetData::UpdateData(prt, this, fbo, 0);
    return prt;
}

RenderTarget* HAL::CreateRenderTarget(Render::Texture* texture, bool needsStencil)
{
    GL::Texture* pt = (GL::Texture*)texture;

    if ( !pt || pt->TextureCount != 1 )
        return 0;

    GLuint fboID = 0;
    RenderTarget* prt = pRenderBufferManager->CreateRenderTarget(
        texture->GetSize(), RBuffer_Texture, texture->GetFormat(), texture);
    if ( !prt )
        return 0;
    Ptr<DepthStencilBuffer> pdsb;

    // Cannot render to textures which have multiple HW representations.
    SF_ASSERT(pt->TextureCount == 1); 
    GLuint colorID = pt->pTextures[0].TexId;

    glGenFramebuffers(1, &fboID);
    glBindFramebuffer(GL_FRAMEBUFFER, fboID);
    ++AccumulatedStats.RTChanges;

#if defined(GL_ES_VERSION_2_0)
    // If on GLES2, and it has NPOT limitations, then we need to ensure that the texture
    // uses clamping mode without mipmapping, otherwise the glCheckFramebufferStatus will 
    // return that the target is unsupported.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
#endif

    // Bind the color buffer.
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorID, 0);

    // Create (and bind) the depth/stencil buffers if required.
    if ( needsStencil )
        pdsb = *createCompatibleDepthStencil(texture->GetSize(), false);

    RenderTargetData::UpdateData(prt, this, fboID, pdsb);
    return prt;
}

RenderTarget* HAL::CreateTempRenderTarget(const ImageSize& size, bool needsStencil)
{
    RenderTarget* prt = pRenderBufferManager->CreateTempRenderTarget(size);
    if ( !prt )
        return 0;
    Texture* pt = (Texture*)prt->GetTexture();
    if ( !pt )
        return 0;

    RenderTargetData* phd = (RenderTargetData*)prt->GetRenderTargetData();
    if ( phd && (!needsStencil || phd->pDepthStencilBuffer != 0 ))
        return prt;

    // If only a new depth stencil is required.
    GLuint fboID;
    GLuint colorID = pt->pTextures[0].TexId;

    if ( phd )
        fboID = phd->FBOID;
    else
        glGenFramebuffers(1, &fboID);

    glBindFramebuffer(GL_FRAMEBUFFER, fboID);
    ++AccumulatedStats.RTChanges;

#if defined(GL_ES_VERSION_2_0)
    // If on GLES2, and it has NPOT limitations, then we need to ensure that the texture
    // uses clamping mode without mipmapping, otherwise the glCheckFramebufferStatus will 
    // return that the target is unsupported.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
#endif

    // Bind the color buffer
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorID, 0);

    // Create (and bind) the depth/stencil buffers if required.
    Ptr<DepthStencilBuffer> pdsb = 0;
    if ( needsStencil )
        pdsb = *createCompatibleDepthStencil(size, true);

    RenderTargetData::UpdateData(prt, this, fboID, pdsb);
    return prt;
}

bool HAL::SetRenderTarget(RenderTarget* ptarget, bool setState)
{
    // When changing the render target while in a scene, we must flush all drawing.
    if ( HALState & HS_InScene)
        Flush();

    // Cannot set the bottom level render target if already in display.
    if ( HALState & HS_InDisplay )
        return false;

    RenderTargetEntry entry;
    if ( setState )
    {
        RenderTargetData* phd = (RenderTargetData*)ptarget->GetRenderTargetData();
        glBindFramebuffer(GL_FRAMEBUFFER, phd->FBOID);
    }

    entry.pRenderTarget = ptarget;

    // Replace the stack entry at the bottom, or if the stack is empty, add one.
    if ( RenderTargetStack.GetSize() > 0 )
        RenderTargetStack[0] = entry;
    else
        RenderTargetStack.PushBack(entry);
    return true;   
}

void HAL::PushRenderTarget(const RectF& frameRect, RenderTarget* prt, unsigned flags, Color clearColor)
{
    // Setup the render target/depth stencil on the device.
    HALState |= HS_InRenderTarget;
    RenderTargetEntry entry;
    entry.pRenderTarget = prt;
    entry.OldViewport = VP;
    entry.OldViewRect = ViewRect;
    entry.OldMatrixState.CopyFrom(Matrices);
    Matrices->Orient2D.SetIdentity();
    Matrices->Orient3D.SetIdentity();
    Matrices->SetUserMatrix(Matrix2F::Identity);
    // VP.Flags &= ~Viewport::View_Orientation_Mask;

    // Setup the render target/depth stencil.
    if ( !prt )
    {
        SF_DEBUG_WARNING(1, "HAL::PushRenderTarget - invalid render target.");
        RenderTargetStack.PushBack(entry);
        return;
    }
    RenderTargetData* phd = (GL::RenderTargetData*)prt->GetRenderTargetData();
    glBindFramebuffer(GL_FRAMEBUFFER, phd->FBOID);
    StencilChecked = false;
    ++AccumulatedStats.RTChanges;

    glDisable(GL_SCISSOR_TEST);

    // Clear, if not specifically excluded
    if ( (flags & PRT_NoClear) == 0 )
    {
        float clear[4];
        clearColor.GetRGBAFloat(clear);
        glClearColor(clear[0], clear[1], clear[2], clear[3]);
        glClear(GL_COLOR_BUFFER_BIT);
    }

    // Setup viewport.
    Rect<int> viewRect = prt->GetRect(); // On the render texture, might not be the entire surface.
    const ImageSize& bs = prt->GetBufferSize();
    VP = Viewport(bs.Width, bs.Height, viewRect.x1, viewRect.y1, viewRect.Width(), viewRect.Height());    
    VP.Flags |= Viewport::View_IsRenderTexture;

    ViewRect.x1 = (int)frameRect.x1;
    ViewRect.y1 = (int)frameRect.y1;
    ViewRect.x2 = (int)frameRect.x2;
    ViewRect.y2 = (int)frameRect.y2;

    // Must offset the 'original' viewrect, otherwise the 3D compensation matrix will be offset.
    Matrices->ViewRectOriginal.Offset(-entry.OldViewport.Left, -entry.OldViewport.Top);
    Matrices->UVPOChanged = true;

    HALState |= HS_ViewValid;
    updateViewport();

    RenderTargetStack.PushBack(entry);
}

void HAL::PopRenderTarget(unsigned flags)
{
    RenderTargetEntry& entry = RenderTargetStack.Back();
    RenderTarget* prt = entry.pRenderTarget;
    if ( prt->GetType() == RBuffer_Temporary )
    {
        // Strip off the depth stencil surface/buffer from temporary targets.
        GL::RenderTargetData* plasthd = (GL::RenderTargetData*)prt->GetRenderTargetData();
        if ( plasthd->pDepthStencilBuffer )
        {
            glBindFramebuffer(GL_FRAMEBUFFER, plasthd->FBOID);
            ++AccumulatedStats.RTChanges;
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, 0);
        }
        plasthd->pDepthStencilBuffer = 0;
    }
    Matrices->CopyFrom(&entry.OldMatrixState);
    ViewRect = entry.OldViewRect;
    VP = entry.OldViewport;

    RenderTargetStack.PopBack();
    RenderTargetData* phd = 0;
    GLuint fboID = 0;
    if ( RenderTargetStack.GetSize() > 0 )
    {
        RenderTargetEntry& back = RenderTargetStack.Back();
        phd = (GL::RenderTargetData*)back.pRenderTarget->GetRenderTargetData();
        fboID = phd->FBOID;
    }

    if ( RenderTargetStack.GetSize() == 1 )
        HALState &= ~HS_InRenderTarget;

    // Restore the old render target.
    if ((flags & PRT_NoSet) == 0)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, fboID);
        ++AccumulatedStats.RTChanges;

        // Reset the viewport to the last render target on the stack.
        HALState |= HS_ViewValid;
        updateViewport();
    }
}

bool HAL::CheckExtension(const char *name)
{
    if (Extensions.IsEmpty())
    {
#if defined(SF_USE_GLES_ANY) || !defined(GL_VERSION_3_0)
        Extensions = (const char *) glGetString(GL_EXTENSIONS);
#else
        if (!CheckGLVersion(3,0))
            Extensions = (const char*)glGetString(GL_EXTENSIONS);
        else
        {
            GLint extCount;
            glGetIntegerv(GL_NUM_EXTENSIONS, &extCount);
            for (int extIndex = 0; extIndex < extCount; ++extIndex)
            {
                const char* ext = (const char*)glGetStringi(GL_EXTENSIONS, extIndex);
                Extensions += " ";
                Extensions += ext;
            }
        }
#endif
        // Add space, just so we know it is initialized.
        Extensions += " "; 
    }

    if (name == 0)
        return false;
    const char *p = strstr(Extensions.ToCStr(), name);
    return (p && (p[strlen(name)] == 0 || p[strlen(name)] == ' '));
}

bool HAL::CheckGLVersion(unsigned reqMajor, unsigned reqMinor)
{
    if (MajorVersion == 0 && MinorVersion == 0)
    {
        const char* version = (const char*)glGetString(GL_VERSION);
        SFsscanf(version, "%d.%d", &MajorVersion, &MinorVersion);
    }
    return (MajorVersion > reqMajor || (MajorVersion == reqMajor && MinorVersion >= reqMinor));
}

ImageSize HAL::getFboInfo(GLint fbo, GLint& currentFBO, bool useCurrent)
{
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &currentFBO);
    if (!useCurrent)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        ++AccumulatedStats.RTChanges;
    }

    bool validFBO = glIsFramebuffer( fbo ) ? true : false;
    GLint width = 0, height = 0;
    GLint type, id;

    if ( validFBO )
    {
        glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, &type );
        glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &id );
        switch(type)
        {
        case GL_TEXTURE:
            {
#ifdef GL_TEXTURE_WIDTH
                glBindTexture(GL_TEXTURE_2D, id );
                glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width );
                glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height );
#endif
                break;
            }
        case GL_RENDERBUFFER:
            if ( !glIsRenderbuffer( id ) )
                break;
            glBindRenderbuffer(GL_RENDERBUFFER, id);
            glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH, &width );
            glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, &height );
            break;
        }
    }

    if (width == 0 || height == 0)
    {
        // Get the dimensions of the framerect from glViewport.
        GLfloat viewport[4];
        glGetFloatv(GL_VIEWPORT, viewport);
        width = (GLint)viewport[2];
        height = (GLint)viewport[3];
    }

    if (!useCurrent)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, currentFBO);
        ++AccumulatedStats.RTChanges;
    }

    return ImageSize(width, height);
}

DepthStencilBuffer* HAL::createCompatibleDepthStencil(const ImageSize& size, bool temporary)
{
    GLuint dsbID = 0;

    // NOTE: until the HAL has successfully created compatible depth stencil buffer, it creates
    // 'user' depth stencil buffers, as these will not be reused. If we happen to create incompatible
    // ones, when trying to locate a compatible format, we don't want them to be destroyed.
    DepthStencilBuffer* pdsb = pRenderBufferManager->CreateDepthStencilBuffer(size, temporary && DeterminedDepthStencilFormat);
    DepthStencilSurface* pdss = (DepthStencilSurface*)pdsb->GetSurface();
    dsbID = pdss->RenderBufferID;

    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, dsbID);

    // Some devices require that the depth buffer be attached, even if we don't use it.
    if (GL::DepthStencilSurface::CurrentFormatHasDepth())
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, dsbID);

    // If this check fails, it means that the stencil format and color format are incompatible.
    // In this case, we will need to try another depth stencil format combination.
    GLenum framebufferStatusError;
    while ((framebufferStatusError = glCheckFramebufferStatus(GL_FRAMEBUFFER)) != GL_FRAMEBUFFER_COMPLETE)
    {
        if (!GL::DepthStencilSurface::SetNextGLFormatIndex())
        {
            SF_DEBUG_WARNING(1, "No compatible depth stencil formats available. Masking in filter will be disabled");
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, 0);
            pdsb = 0;
            break;
        }

        pdsb = pRenderBufferManager->CreateDepthStencilBuffer(size, temporary && DeterminedDepthStencilFormat);
        DepthStencilSurface* pdss = (DepthStencilSurface*)pdsb->GetSurface();
        dsbID = pdss->RenderBufferID;        
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, dsbID);

        // Some devices require that the depth buffer be attached, even if we don't use it. If it was previously attached,
        // and now our format does not have depth, we must remove it.
        if (GL::DepthStencilSurface::CurrentFormatHasDepth())
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, dsbID);
        else
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, 0);
    }

    // If a complete framebuffer was found, then indicate that the depth/stencil format has been determined.
    if (framebufferStatusError == GL_FRAMEBUFFER_COMPLETE)
        DeterminedDepthStencilFormat = true;

    return pdsb;
}

bool HAL::ShouldUseVAOs()
{
    // If we are not using VBOs, then we cannot use VAOs.
    if ((GetMeshCache().GetBufferUpdateType() == MeshCache::BufferUpdate_ClientBuffers) )
        return false;

#if !defined(SF_USE_GLES_ANY) && defined(GL_VERSION_3_0)
    // OpenGL 3.0+ should use it, unless specifically disabled, or using client buffers
    return CheckGLVersion(3,0) && !(GetHAL()->Caps & Cap_NoVAO);
#else
    // All other GL should not.
    return false;
#endif
}

bool HAL::createDefaultRenderBuffer()
{
    ImageSize rtSize;

    if ( GetDefaultRenderTarget() )
    {
        RenderTarget* prt = GetDefaultRenderTarget();
        rtSize = prt->GetSize();
    }
    else
    {
        RenderTargetEntry entry;

        GLint currentFBO;
        rtSize = getFboInfo(0, currentFBO, true);

        Ptr<RenderTarget> ptarget = *SF_HEAP_AUTO_NEW(this) RenderTarget(0, RBuffer_Default, rtSize );
        Ptr<DepthStencilBuffer> pdsb = *SF_HEAP_AUTO_NEW(this) DepthStencilBuffer(0, rtSize);
        RenderTargetData::UpdateData(ptarget, this, currentFBO, pdsb);

        if (!SetRenderTarget(ptarget))
            return false;
    }

    return pRenderBufferManager->Initialize(pTextureManager, Image_R8G8B8A8, rtSize );
}

void HAL::drawUncachedFilter(const FilterStackEntry& e)
{
    FilterVertexBufferSet = 0;
    ShaderHAL<ShaderManager, ShaderInterface>::drawUncachedFilter(e);
}

void HAL::drawCachedFilter(FilterPrimitive* primitive)
{
    FilterVertexBufferSet = 0;
    ShaderHAL<ShaderManager, ShaderInterface>::drawCachedFilter(primitive);
}

//--------------------------------------------------------------------
#if defined(GL_ES_VERSION_2_0)
#include "Render/GL/GLES_ExtensionMacros.h"
#else
#include "Render/GL/GL_ExtensionMacros.h"
#endif

RenderTargetData::~RenderTargetData()
{
    if (pBuffer->GetType() != RBuffer_Default && pBuffer->GetType() != RBuffer_User)
    {
        GL::TextureManager* pmgr = (GL::TextureManager*)pHAL->GetTextureManager();

        // If the texture manager isn't present, just try deleting it immediately.
        if (!pmgr)
            glDeleteFramebuffers(1, &FBOID);
        else
            pmgr->DestroyFBO(FBOID);
    }

}

// Helper function to retrieve the vertex element type (VET) and normalization from a vertex attribute. Returns false if the attribute should be ignored.
bool VertexBuilderVET(unsigned attr, GLenum&vet, bool& norm)
{
    switch (attr & VET_CompType_Mask)
    {
    case VET_U8:  vet = GL_UNSIGNED_BYTE; norm = false; break;
    case VET_U8N: vet = GL_UNSIGNED_BYTE; norm = true; break;
    case VET_U16: vet = GL_UNSIGNED_SHORT; norm = false; break;
    case VET_S16: vet = GL_SHORT; norm = false; break;
    case VET_U32: vet = GL_UNSIGNED_INT; norm = false; break;
    case VET_F32: vet = GL_FLOAT; norm = false;  break;

        // Instance indices are not used in the vertex arrays, so just ignore them.
    case VET_I8:
    case VET_I16:
        return false;

    default: SF_ASSERT(0); vet = GL_FLOAT; norm = false; return false;
    }
    return true;
}

// Uses functions within the GL 2.1- (or GLES 2.0) spec to define vertex attributes. This is not
// compatible with GL 3.0+ (see VertexBuilder_Core30).
class VertexBuilder_Legacy
{
public:
    HAL*            pHal;
    unsigned        Stride;
    UByte*          VertexOffset;

    VertexBuilder_Legacy(HAL* phal, unsigned stride, GLuint vbuffer, GLuint ibuffer, UByte* vertOffset) : 
        pHal(phal), Stride(stride), VertexOffset(vertOffset)
    {
        // Bind the vertex buffer and the index buffer immediately.
        glBindBuffer(GL_ARRAY_BUFFER, vbuffer);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibuffer);
    }

    HAL* GetHAL() { return pHal; }

    void Add(int vi, int attr, int ac, int offset)
    {
        GLenum vet; bool norm;
        if (!VertexBuilderVET(attr, vet, norm))
            return;

        if ( pHal->EnabledVertexArrays < vi )
        {
            glEnableVertexAttribArray(vi);
            pHal->EnabledVertexArrays++;
        }

        // Note: Extend the size of UByte w/1 component to 4 components. On certain drivers, this
        // appears to work incorrectly, but extending it to 4xUByte corrects the issue (even though
        // 3 of the elements are unused).
        if (vet == GL_UNSIGNED_BYTE && ac < 4)
            ac = 4;

        glVertexAttribPointer(vi, ac, vet, norm, Stride, VertexOffset + offset);
    }

    void Finish(int vi)
    {
        int newEnabledCount = vi-1;
        for (int i = vi; i < pHal->EnabledVertexArrays; i++)
        {
            glDisableVertexAttribArray(i);
        }
        pHal->EnabledVertexArrays = newEnabledCount;
    }
};

class VertexBuilder_Core30
{
public:
    HAL*            pHal;
    unsigned        Stride;
    MeshCacheItem*  pMesh;
    bool            NeedsGeneration;    // Set to true if the VAO has not been initialized yet (and should be done by this class).
    UByte*          VertexOffset;

    VertexBuilder_Core30(HAL* phal, const VertexFormat* pformat, MeshCacheItem* pmesh, UPInt vbOffset) : 
        pHal(phal), Stride(pformat->Size), pMesh(pmesh), NeedsGeneration(false), VertexOffset(0)
    {
        // Allocate VAO for this mesh now.
        VertexOffset = pmesh->pVertexBuffer->GetBufferBase() + pmesh->VBAllocOffset + vbOffset;
        if (pMesh->VAOFormat != pformat || pMesh->VAOOffset != VertexOffset || pMesh->VAO == 0)
        {
            if (pMesh->VAO)
                glDeleteVertexArrays(1, &pMesh->VAO);
            glGenVertexArrays(1, &pMesh->VAO);

            // Store the vertex offset, and indicate that we need to generate the contents of the VAO.
            pMesh->VAOOffset = VertexOffset;
            pMesh->VAOFormat = pformat;
            NeedsGeneration = true;
        }

        // Bind the VAO.
        glBindVertexArray(pMesh->VAO);        

        // If need to generate the VAO, bind the VB/IB now
        if (NeedsGeneration)
        {
            GLuint vbuffer = pmesh->pVertexBuffer->GetBuffer();
            GLuint ibuffer = pmesh->pIndexBuffer->GetBuffer();
            glBindBuffer(GL_ARRAY_BUFFER, vbuffer);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibuffer);
        }
    }

    HAL* GetHAL() { return pHal; }

    void Add(int vi, int attr, int ac, int offset)
    {
        // If we have already generated the VAO, just skip everything.
        if (!NeedsGeneration)
            return;

        GLenum vet; bool norm;
        if (!VertexBuilderVET(attr, vet, norm))
            return;

        // Note: Extend the size of UByte w/1 component to 4 components. On certain drivers, this
        // appears to work incorrectly, but extending it to 4xUByte corrects the issue (even though
        // 3 of the elements are unused).
        if (vet == GL_UNSIGNED_BYTE && ac < 4)
            ac = 4;

        glEnableVertexAttribArray(vi);
        glVertexAttribPointer(vi, ac, vet, norm, Stride, VertexOffset + offset);
    }

    void Finish(int)
    {
    }
};

UPInt HAL::setVertexArray(PrimitiveBatch* pbatch, Render::MeshCacheItem* pmesh)
{
    return setVertexArray(pbatch->pFormat, pmesh, 0);
}

UPInt HAL::setVertexArray(const ComplexMesh::FillRecord& fr, unsigned formatIndex, Render::MeshCacheItem* pmesh)
{
    return setVertexArray(fr.pFormats[formatIndex], pmesh, fr.VertexByteOffset);
}

UPInt HAL::setVertexArray(const VertexFormat* pformat, Render::MeshCacheItem* pmeshBase, UPInt vboffset)
{
    GL::MeshCacheItem* pmesh = reinterpret_cast<GL::MeshCacheItem*>(pmeshBase);
    if (ShouldUseVAOs())
    {
        VertexBuilder_Core30 vb (this, pformat, pmesh, vboffset);
        BuildVertexArray(pformat, vb);
    }
    else
    {
        // Legacy and/or GLES path.
        VertexBuilder_Legacy vb (this, pformat->Size, pmesh->pVertexBuffer->GetBuffer(),
            pmesh->pIndexBuffer->GetBuffer(), pmesh->pVertexBuffer->GetBufferBase() + pmesh->VBAllocOffset + vboffset);
        BuildVertexArray(pformat, vb);
    }
    return reinterpret_cast<UPInt>((pmesh->pIndexBuffer->GetBufferBase() + pmesh->IBAllocOffset)) / sizeof(IndexType); 
}

void HAL::setVertexArray(const VertexFormat* pFormat, GLuint buffer, GLuint vao)
{
	SF_UNUSED(pFormat);
    SF_UNUSED(buffer);
    if (ShouldUseVAOs())
    {
        // Immediately bind the VAO, it must be constructed already.
        glBindVertexArray(vao);
        return;
    }

    // Legacy and/or GLES path. Assume no buffer offsets.
    VertexBuilder_Legacy vb (this, pFormat->Size, buffer, 0, 0);
    BuildVertexArray(pFormat, vb);
}

const ShaderObject* HAL::GetStaticShader( ShaderDesc::ShaderType shaderType )
{
    unsigned comboIndex = FragShaderDesc::GetShaderComboIndex(shaderType, SManager.GLSLVersion);
    SF_DEBUG_ASSERT(VertexShaderDesc::GetShaderComboIndex(shaderType, SManager.GLSLVersion) == comboIndex,
        "Expected ComboIndex for both vertex and fragment shaders to be equivalent.");
    if ( comboIndex >= UniqueShaderCombinations )
        return 0;

    ShaderObject* shader = &SManager.StaticShaders[comboIndex];

    // Initialize the shader if it hasn't already been initialized.
    if ( (VMCFlags & HALConfig_DynamicShaderCompile) && !shader->IsInitialized() )
    {
        if ( !shader->Init(this, SManager.GLSLVersion, comboIndex, 
            SManager.UsingSeparateShaderObject(), SManager.CompiledShaderHash ))
        {
            return 0;
        }
        else if (VMCFlags & HALConfig_MultipleShaderCacheFiles)
        {
            // If we have a file-per-shader, save shaders after every dynamic initialization.
            SManager.saveBinaryShaders();
        }
    }
    return shader;
}

bool HAL::shouldRenderFilters(const FilterPrimitive*) const
{
    return true;
}

// Simply sets a quad vertex buffer and draws.
void HAL::drawScreenQuad()
{
    // Set the vertices, and Draw
    setBatchUnitSquareVertexStream();
    drawPrimitive(6,1);
}

void    HAL::drawFilter(const Matrix2F& mvp, const Cxform & cx, const Filter* filter, Ptr<RenderTarget> * targets, 
                        unsigned* shaders, unsigned pass, unsigned passCount, const VertexFormat* pvf, 
                        BlurFilterState& leBlur)
{
    if (leBlur.Passes > 0)
    {
        leBlur.SetPass(pass);

        const BlurFilterShader* pShader = ShaderData.GetBlurShader(leBlur);
        if (!pShader)
            return;

        Rect<int> srcrect = targets[Target_Source]->GetRect();
        Rect<int> destrect = Rect<int>(0,0,1,1);

        glUseProgram(pShader->Shader);

        if (pass != passCount-1)
        {
            BlendEnable = 1;
            glEnable(GL_BLEND);
            glBlendFunc(GL_ONE, GL_ONE);
            glBlendEquation(GL_FUNC_ADD);
        }
        // else do nothing, dest blend mode was set before calling drawFilter

        glUniform4fv(pShader->mvp, 2, &mvp.M[0][0]);

        if (pShader->cxadd >= 0)
        {
            float cxform[2][4];
            cx.GetAsFloat2x4(cxform);
            glUniform4fv(pShader->cxmul, 1, cxform[0]);
            glUniform4fv(pShader->cxadd, 1, cxform[1]);
        }

        for (int i = 0; i < 2; i++)
            if (pShader->scolor[i] >= 0)
            {
                float color[4];
                leBlur.CurPass->Colors[i].GetRGBAFloat(color);
                glUniform4fv(pShader->scolor[i], 1, color);
            }

        if (pShader->samples >= 0)
        {
            glUniform1f(pShader->samples, 1.0f/leBlur.Samples);
        }

        if (pShader->tex[1] >= 0)
        {
            GL::Texture *ptexture = (GL::Texture*) targets[Target_Original]->GetTexture();
            GL::TextureManager* pmanager = (GL::TextureManager*)ptexture->GetTextureManager();
            pmanager->ApplyTexture(1, ptexture->pTextures[0].TexId);
            glUniform1i(pShader->tex[1], 1);
            glUniform2f(pShader->texscale[1], 1.0f/ptexture->GetSize().Width, 1.0f/ptexture->GetSize().Height);
        }

        GL::Texture *ptexture = (GL::Texture*) targets[Target_Source]->GetTexture();
        GL::TextureManager* pmanager = (GL::TextureManager*)ptexture->GetTextureManager();
        pmanager->ApplyTexture(0, ptexture->pTextures[0].TexId);
        glUniform1i(pShader->tex[0], 0);
        glUniform2f(pShader->texscale[0], 1.0f/ptexture->GetSize().Width, 1.0f/ptexture->GetSize().Height);

        float* pvertices = (float*) alloca(sizeof(float) * leBlur.GetVertexBufferSize());
        VertexFunc_Buffer vout (pvertices);
        leBlur.GetVertices(srcrect, destrect, vout);
        int vbstride = leBlur.VertexAttrs * 2 * sizeof(float);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        if (EnabledVertexArrays < 0)
            glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, 0, vbstride, pvertices);
        for (int i = 0; i < leBlur.TotalTCs; i++)
        {
            if (EnabledVertexArrays < i + 1)
                glEnableVertexAttribArray(i + 1);
            glVertexAttribPointer(i + 1, 2, GL_FLOAT, 0, vbstride, pvertices+(2+i*2));
        }
        for (int i = leBlur.TotalTCs+2; i < EnabledVertexArrays; i++)
           glDisableVertexAttribArray(i);
        EnabledVertexArrays = leBlur.TotalTCs;

        drawPrimitive(6 * leBlur.Quads, leBlur.Quads);

        FilterVertexBufferSet = 0;
    }
    else
    {
        if (!FilterVertexBufferSet)
        {
            setBatchUnitSquareVertexStream();
            FilterVertexBufferSet = 1;
        }

        SManager.SetFilterFill(mvp, cx, filter, targets, shaders, pass, passCount, pvf, &ShaderData);
        drawPrimitive(6,1);
    }
}

void HAL::setBatchUnitSquareVertexStream()
{
    setVertexArray(&VertexXY16iInstance::Format, Cache.MaskEraseBatchVertexBuffer, Cache.MaskEraseBatchVAO);
}

void HAL::drawPrimitive(unsigned indexCount, unsigned meshCount)
{
    glDrawArrays(GL_TRIANGLES, 0, indexCount);

    SF_UNUSED(meshCount);
#if !defined(SF_BUILD_SHIPPING)
    AccumulatedStats.Meshes += meshCount;
    AccumulatedStats.Triangles += indexCount / 3;
    AccumulatedStats.Primitives++;
#endif
}

void HAL::drawIndexedPrimitive(unsigned indexCount, unsigned vertexCount, unsigned meshCount, UPInt indexPtr, UPInt vertexOffset )
{
    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_SHORT, reinterpret_cast<const GLvoid*>(indexPtr*sizeof(IndexType)));

    SF_UNUSED3(meshCount, vertexCount, vertexOffset);
#if !defined(SF_BUILD_SHIPPING)
    AccumulatedStats.Meshes += meshCount;
    AccumulatedStats.Triangles += indexCount / 3;
    AccumulatedStats.Primitives++;
#endif
}

void HAL::drawIndexedInstanced(unsigned indexCount, unsigned vertexCount, unsigned meshCount, UPInt indexPtr, UPInt vertexOffset )
{
    SF_UNUSED2(vertexCount, vertexOffset);
#if !defined (SF_USE_GLES_ANY)
    glDrawElementsInstanced(GL_TRIANGLES, indexCount, GL_UNSIGNED_SHORT, reinterpret_cast<const GLvoid*>(indexPtr*sizeof(IndexType)), meshCount);
#else
    SF_DEBUG_ASSERT(0, "Instancing not supported on GLES platforms.");
#endif

#if !defined(SF_BUILD_SHIPPING)
    AccumulatedStats.Meshes += meshCount;
    AccumulatedStats.Triangles += (indexCount / 3) * meshCount;
    AccumulatedStats.Primitives++;
#endif
}

Render::RenderEvent& HAL::GetEvent(EventType eventType)
{
#if !defined(SF_BUILD_SHIPPING) && !defined(SF_USE_GLES_ANY)
    static GL::RenderEvent GLEvents[Event_Count];
    static bool EventsInitialized = false;
    if (!EventsInitialized)
    {
        for ( unsigned event = 0; event < Event_Count; ++event)
            GLEvents[event].pHAL = this;
    }
    return GLEvents[eventType];
#else
    // Shipping builds just return a default event, which does nothing.
    return Render::HAL::GetEvent(eventType);
#endif
}

void MatrixState::recalculateUVPOC() const
{
    if (UVPOChanged)
    {
        // Recalculated the view compensation matrix.
        if ( ViewRect != ViewRectOriginal && !ViewRectOriginal.IsNull())
        {
            Point<int> dc = ViewRect.Center() - ViewRectOriginal.Center();
            float      dx = ((float)ViewRectOriginal.Width()) / ViewRect.Width();
            float      dy = ((float)ViewRectOriginal.Height()) / ViewRect.Height();
            float      ox = 2.0f * dc.x / ViewRect.Width();
            float      oy = 2.0f * dc.y / ViewRect.Height();
            ViewRectCompensated3D.MultiplyMatrix(Matrix4F::Translation(-ox, oy, 0), Matrix4F::Scaling(dx, dy, 1));
        }
        else
        {
            ViewRectCompensated3D = Matrix4F::Identity;
        }

        const Matrix4F& Projection = updateStereoProjection();

        Matrix4F flipmat;
        flipmat.SetIdentity();
        if(pHAL && (pHAL->VP.Flags & Viewport::View_IsRenderTexture))
        {
            flipmat.Append(Matrix4F::Scaling(1.0f, -1.0f, 1.0f));
        }
        
        Matrix4F FV(flipmat, ViewRectCompensated3D);
        Matrix4F UO(User3D, FV);
        Matrix4F VRP(Orient3D, Projection);
        UVPO = Matrix4F(Matrix4F(UO, VRP), View3D);
        UVPOChanged = 0;
    }
}

// Simple linear searching. This is only for debugging purposes, so performance is not an issue, and creating a hash seems like overkill.
struct GLEnumString
{
    GLenum      key;
    const char* str;
};

// Returns the string associated with the key, in the given list (list must terminate with key == 0).
const char* findEntry(GLenum key, GLEnumString* list)
{
    while (list && list->key)
    {
        if (list->key == key)
            return list->str;
        list++;
    }
    return "Unknown GLenum";
}

#if defined(SF_CC_GNU)
void GL_APIENTRY DebugMessageCallback(  unsigned source,
                                        unsigned type,
                                        unsigned id,
                                        unsigned severity,
                                        int,
                                        const char* message,
                                        const void*)
#else
void GL_APIENTRY DebugMessageCallback(  GLenum source,
                                      GLenum type,
                                      GLuint id,
                                      GLenum severity,
                                      GLsizei,
                                      const char* message,
                                      void*)
#endif
{
    static GLEnumString sourceList[] = 
    {
        {GL_DEBUG_SOURCE_API,               "GL_DEBUG_SOURCE_API"},            
        {GL_DEBUG_SOURCE_WINDOW_SYSTEM,     "GL_DEBUG_SOURCE_WINDOW_SYSTEM"},  
        {GL_DEBUG_SOURCE_SHADER_COMPILER,   "GL_DEBUG_SOURCE_SHADER_COMPILER"},
        {GL_DEBUG_SOURCE_THIRD_PARTY,       "GL_DEBUG_SOURCE_THIRD_PARTY"},    
        {GL_DEBUG_SOURCE_APPLICATION,       "GL_DEBUG_SOURCE_APPLICATION"},    
        {GL_DEBUG_SOURCE_OTHER,             "GL_DEBUG_SOURCE_OTHER"},          
        {0,                                 ""}
    };

    static GLEnumString typeList[] =
    {
        {GL_DEBUG_TYPE_ERROR,               "GL_DEBUG_TYPE_ERROR"},               
        {GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR, "GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR"}, 
        {GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR,  "GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR"},  
        {GL_DEBUG_TYPE_PORTABILITY,         "GL_DEBUG_TYPE_PORTABILITY"},         
        {GL_DEBUG_TYPE_PERFORMANCE,         "GL_DEBUG_TYPE_PERFORMANCE"},         
        {GL_DEBUG_TYPE_OTHER,               "GL_DEBUG_TYPE_OTHER"},               
        {0,                                 ""}
    };

    static GLEnumString severityList[] =
    {
        {GL_DEBUG_SEVERITY_HIGH,    "GL_DEBUG_SEVERITY_HIGH"},  
        {GL_DEBUG_SEVERITY_MEDIUM,  "GL_DEBUG_SEVERITY_MEDIUM"},
        {GL_DEBUG_SEVERITY_LOW,     "GL_DEBUG_SEVERITY_LOW"},   
        {0,                         ""}
    };

    const char* sourceText   = findEntry(source, sourceList);
    const char* typeText     = findEntry(type, typeList);
    const char* severityText = findEntry(severity, severityList);

    LogDebugMessage(Log_Warning,
                    "GL Debug Message: %s\n"
                    "Source          : %s\n"
                    "Type            : %s\n"
                    "Severity        : %s\n"
                    "Id              : %d\n", 
                    message, sourceText, typeText, severityText, id);
}

}}} // Scaleform::Render::GL

#endif // !defined(SF_USE_GLES)
