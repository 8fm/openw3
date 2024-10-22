project(libgfx)

set(SRC_FILES
    "../../../../Src/Kernel/HeapPT/HeapPT_SysAllocMalloc.cpp"
    "../../../../Src/Kernel/HeapPT/HeapPT_AllocBitSet1.cpp"
    "../../../../Src/Kernel/HeapPT/HeapPT_AllocBitSet2.cpp"
    "../../../../Src/Kernel/HeapPT/HeapPT_AllocEngine.cpp"
    "../../../../Src/Kernel/HeapPT/HeapPT_AllocLite.cpp"
    "../../../../Src/Kernel/HeapPT/HeapPT_BitSet1.cpp"
    "../../../../Src/Kernel/HeapPT/HeapPT_Bookkeeper.cpp"
    "../../../../Src/Kernel/HeapPT/HeapPT_DebugInfo.cpp"
    "../../../../Src/Kernel/HeapPT/HeapPT_FreeBin.cpp"
    "../../../../Src/Kernel/HeapPT/HeapPT_Granulator.cpp"
    "../../../../Src/Kernel/HeapPT/HeapPT_MemoryHeap.cpp"
    "../../../../Src/Kernel/HeapPT/HeapPT_PageTable.cpp"
    "../../../../Src/Kernel/HeapPT/HeapPT_Root.cpp"
    "../../../../Src/Kernel/HeapPT/HeapPT_Starter.cpp"
    "../../../../Src/Kernel/HeapPT/HeapPT_SysAllocMapper.cpp"
    "../../../../Src/Kernel/HeapPT/HeapPT_SysAllocStatic.cpp"
    "../../../../Src/Kernel/HeapMH/HeapMH_AllocBitSet2.cpp"
    "../../../../Src/Kernel/HeapMH/HeapMH_AllocEngine.cpp"
    "../../../../Src/Kernel/HeapMH/HeapMH_DebugInfo.cpp"
    "../../../../Src/Kernel/HeapMH/HeapMH_FreeBin.cpp"
    "../../../../Src/Kernel/HeapMH/HeapMH_MemoryHeap.cpp"
    "../../../../Src/Kernel/HeapMH/HeapMH_Root.cpp"
    "../../../../Src/Kernel/SF_FileFILE.cpp"
    "../../../../Src/Kernel/SF_Alg.cpp"
    "../../../../Src/Kernel/SF_AllocAddr.cpp"
    "../../../../Src/Kernel/SF_AmpInterface.cpp"
    "../../../../Src/Kernel/SF_Atomic.cpp"
    "../../../../Src/Kernel/SF_Debug.cpp"
    "../../../../Src/Kernel/SF_File.cpp"
    "../../../../Src/Kernel/SF_Locale.cpp"
    "../../../../Src/Kernel/SF_Log.cpp"
    "../../../../Src/Kernel/SF_MemItem.cpp"
    "../../../../Src/Kernel/SF_MemoryHeap.cpp"
    "../../../../Src/Kernel/SF_MemReport.cpp"
    "../../../../Src/Kernel/SF_MsgFormat.cpp"
    "../../../../Src/Kernel/SF_Random.cpp"
    "../../../../Src/Kernel/SF_RefCount.cpp"
    "../../../../Src/Kernel/SF_Stats.cpp"
    "../../../../Src/Kernel/SF_Std.cpp"
    "../../../../Src/Kernel/SF_String.cpp"
    "../../../../Src/Kernel/SF_String_PathUtil.cpp"
    "../../../../Src/Kernel/SF_SysFile.cpp"
    "../../../../Src/Kernel/SF_System.cpp"
    "../../../../Src/Kernel/SF_Threads.cpp"
    "../../../../Src/Kernel/SF_Timer.cpp"
    "../../../../Src/Kernel/SF_UTF8Util.cpp"
    "../../../../Src/Kernel/SF_WString.cpp"
    "../../../../Src/GFx/AMP/Amp_Message.cpp"
    "../../../../Src/GFx/AMP/Amp_MessageRegistry.cpp"
    "../../../../Src/GFx/AMP/Amp_ObjectsLog.cpp"
    "../../../../Src/GFx/AMP/Amp_ProfileFrame.cpp"
    "../../../../Src/GFx/AMP/Amp_Server.cpp"
    "../../../../Src/GFx/AMP/Amp_ServerCallbacks.cpp"
    "../../../../Src/GFx/AMP/Amp_Stream.cpp"
    "../../../../Src/GFx/AMP/Amp_ThreadMgr.cpp"
    "../../../../Src/GFx/AMP/Amp_ViewStats.cpp"
    "../../../../Src/GFx/Net/GFx_Socket.cpp"
    "../../../../Src/GFx/Audio/GFx_Sound.cpp"
    "../../../../Src/GFx/Audio/GFx_SoundResource.cpp"
    "../../../../Src/GFx/Audio/GFx_SoundTagsReader.cpp"
    "../../../../Src/GFx/Audio/GFx_Audio.cpp"
    "../../../../Src/GFx/Text/Text_EditorKit.cpp"
    "../../../../Src/GFx/Text/Text_StyleSheet.cpp"
    "../../../../Src/GFx/XML/XML_Document.cpp"
    "../../../../Src/GFx/XML/XML_DOM.cpp"
    "../../../../Src/GFx/XML/XML_Object.cpp"
    "../../../../Src/GFx/XML/XML_Support.cpp"
    "../../../../Src/GFx/IME/GFx_IMEManager.cpp"
    "../../../../Src/GFx/IME/GFxKoreanIME.cpp"
    "../../../../Src/GFx/GFx_ASString.cpp"
    "../../../../Src/GFx/GFx_ASUtils.cpp"
    "../../../../Src/GFx/GFx_Button.cpp"
    "../../../../Src/GFx/GFx_ButtonDef.cpp"
    "../../../../Src/GFx/GFx_CharacterDef.cpp"
    "../../../../Src/GFx/GFx_DisplayList.cpp"
    "../../../../Src/GFx/GFx_DisplayObjContainer.cpp"
    "../../../../Src/GFx/GFx_DisplayObject.cpp"
    "../../../../Src/GFx/GFx_DrawText.cpp"
    "../../../../Src/GFx/GFx_DrawingContext.cpp"
    "../../../../Src/GFx/GFx_Event.cpp"
    "../../../../Src/GFx/GFx_FileOpener.cpp"
    "../../../../Src/GFx/GFx_FilterDesc.cpp"
    "../../../../Src/GFx/GFx_FontCompactor.cpp"
    "../../../../Src/GFx/GFx_FontGlyphPacker.cpp"
    "../../../../Src/GFx/GFx_FontLib.cpp"
    "../../../../Src/GFx/GFx_FontProviderHUD.cpp"
    "../../../../Src/GFx/GFx_FontManager.cpp"
    "../../../../Src/GFx/GFx_FontMap.cpp"
    "../../../../Src/GFx/GFx_FontResource.cpp"
    "../../../../Src/GFx/GFx_Gesture.cpp"
    "../../../../Src/GFx/GFx_ImageCreator.cpp"
    "../../../../Src/GFx/GFx_ImagePacker.cpp"
    "../../../../Src/GFx/GFx_ImageResource.cpp"
    "../../../../Src/GFx/GFx_ImageSupport.cpp"
    "../../../../Src/GFx/GFx_Input.cpp"
    "../../../../Src/GFx/GFx_InteractiveObject.cpp"
    "../../../../Src/GFx/GFx_LoadProcess.cpp"
    "../../../../Src/GFx/GFx_Loader.cpp"
    "../../../../Src/GFx/GFx_LoaderUtil.cpp"
    "../../../../Src/GFx/GFx_LoaderImpl.cpp"
    "../../../../Src/GFx/GFx_Log.cpp"
    "../../../../Src/GFx/GFx_MorphCharacter.cpp"
    "../../../../Src/GFx/GFx_MovieDef.cpp"
    "../../../../Src/GFx/GFx_PlayerImpl.cpp"
    "../../../../Src/GFx/GFx_PlayerTasks.cpp"
    "../../../../Src/GFx/GFx_Resource.cpp"
    "../../../../Src/GFx/GFx_ResourceHandle.cpp"
    "../../../../Src/GFx/GFx_Shape.cpp"
    "../../../../Src/GFx/GFx_ShapeSwf.cpp"
    "../../../../Src/GFx/GFx_Sprite.cpp"
    "../../../../Src/GFx/GFx_SpriteDef.cpp"
    "../../../../Src/GFx/GFx_StaticText.cpp"
    "../../../../Src/GFx/GFx_Stream.cpp"
    "../../../../Src/GFx/GFx_String.cpp"
    "../../../../Src/GFx/GFx_TagLoaders.cpp"
    "../../../../Src/GFx/GFx_TaskManager.cpp"
    "../../../../Src/GFx/GFx_TextField.cpp"
    "../../../../Src/GFx/GFx_TextFieldDef.cpp"
    "../../../../Src/GFx/GFx_TextureFont.cpp"
    "../../../../Src/GFx/GFx_ZLibFile.cpp"
    "../../../../Src/GFx/GFx_ZlibSupport.cpp"
    "../../../../Src/Render/ImageFiles/DDS_ImageReader.cpp"
    "../../../../Src/Render/ImageFiles/JPEG_ImageCommon.cpp"
    "../../../../Src/Render/ImageFiles/JPEG_ImageReader.cpp"
    "../../../../Src/Render/ImageFiles/JPEG_ImageWriter.cpp"
    "../../../../Src/Render/ImageFiles/PNG_ImageReader.cpp"
    "../../../../Src/Render/ImageFiles/PVR_ImageReader.cpp"
    "../../../../Src/Render/ImageFiles/Render_ImageFile.cpp"
    "../../../../Src/Render/ImageFiles/Render_ImageFileUtil.cpp"
    "../../../../Src/Render/ImageFiles/TGA_ImageReader.cpp"
    "../../../../Src/Render/ImageFiles/TGA_ImageWriter.cpp"
    "../../../../Src/Render/ImageFiles/SIF_ImageReader.cpp"
    "../../../../Src/Render/ImageFiles/SIF_ImageWriter.cpp"
    "../../../../Src/Render/Text/Text_Core.cpp"
    "../../../../Src/Render/Text/Text_DocView.cpp"
    "../../../../Src/Render/Text/Text_FilterDesc.cpp"
    "../../../../Src/Render/Text/Text_Highlight.cpp"
    "../../../../Src/Render/Text/Text_HTMLParser.cpp"
    "../../../../Src/Render/Text/Text_LineBuffer.cpp"
    "../../../../Src/Render/Text/Text_StyledText.cpp"
    "../../../../Src/Render/Text/Text_WWHelper.cpp"
    "../../../../Src/Render/FontProvider/Render_FontProviderHUD.cpp"
    "../../../../Src/Render/Render_BufferGeneric.cpp"
    "../../../../Src/Render/Render_Bundle.cpp"
    "../../../../Src/Render/Render_CacheEffect.cpp"
    "../../../../Src/Render/Render_Color.cpp"
    "../../../../Src/Render/Render_Context.cpp"
    "../../../../Src/Render/Render_CxForm.cpp"
    "../../../../Src/Render/Render_DrawableImage.cpp"
    "../../../../Src/Render/Render_DrawableImage_HW.cpp"
    "../../../../Src/Render/Render_DrawableImage_Queue.cpp"
    "../../../../Src/Render/Render_DrawableImage_SW.cpp"
    "../../../../Src/Render/Render_Filters.cpp"
    "../../../../Src/Render/Render_Font.cpp"
    "../../../../Src/Render/Render_FontCacheHandle.cpp"
    "../../../../Src/Render/Render_GlyphCache.cpp"
    "../../../../Src/Render/Render_GlyphFitter.cpp"
    "../../../../Src/Render/Render_GlyphQueue.cpp"
    "../../../../Src/Render/Render_Gradients.cpp"
    "../../../../Src/Render/Render_HAL.cpp"
    "../../../../Src/Render/Render_Hairliner.cpp"
    "../../../../Src/Render/Render_HitTest.cpp"
    "../../../../Src/Render/Render_Image.cpp"
    "../../../../Src/Render/Render_Matrix2x4.cpp"
    "../../../../Src/Render/Render_Matrix3x4.cpp"
    "../../../../Src/Render/Render_Matrix4x4.cpp"
    "../../../../Src/Render/Render_MatrixPool.cpp"
    "../../../../Src/Render/Render_MeshCache.cpp"
    "../../../../Src/Render/Render_MeshKey.cpp"
    "../../../../Src/Render/Render_PathDataPacker.cpp"
    "../../../../Src/Render/Render_Primitive.cpp"
    "../../../../Src/Render/Render_PrimitiveBundle.cpp"
    "../../../../Src/Render/Render_PrimitiveProcessor.cpp"
    "../../../../Src/Render/Render_Profiler.cpp"
    "../../../../Src/Render/Render_Queue.cpp"
    "../../../../Src/Render/Render_Rasterizer.cpp"
    "../../../../Src/Render/Render_RectPacker.cpp"
    "../../../../Src/Render/Render_ResizeImage.cpp"
    "../../../../Src/Render/Render_Scale9Grid.cpp"
    "../../../../Src/Render/Render_ScreenToWorld.cpp"
    "../../../../Src/Render/Render_ShapeDataFloat.cpp"
    "../../../../Src/Render/Render_ShapeDataFloatMP.cpp"
    "../../../../Src/Render/Render_ShapeMeshProvider.cpp"
    "../../../../Src/Render/Render_SimpleMeshCache.cpp"
    "../../../../Src/Render/Render_StateBag.cpp"
    "../../../../Src/Render/Render_States.cpp"
    "../../../../Src/Render/Render_Stats.cpp"
    "../../../../Src/Render/Render_Stroker.cpp"
    "../../../../Src/Render/Render_StrokerAA.cpp"
    "../../../../Src/Render/Render_Sync.cpp"
    "../../../../Src/Render/Render_TessCurves.cpp"
    "../../../../Src/Render/Render_Tessellator.cpp"
    "../../../../Src/Render/Render_TextLayout.cpp"
    "../../../../Src/Render/Render_TextMeshProvider.cpp"
    "../../../../Src/Render/Render_TextPrimitiveBundle.cpp"
    "../../../../Src/Render/Render_TextureCacheGeneric.cpp"
    "../../../../Src/Render/Render_TextureUtil.cpp"
    "../../../../Src/Render/Render_TreeCacheNode.cpp"
    "../../../../Src/Render/Render_TreeCacheText.cpp"
    "../../../../Src/Render/Render_TreeLog.cpp"
    "../../../../Src/Render/Render_TreeMesh.cpp"
    "../../../../Src/Render/Render_TreeNode.cpp"
    "../../../../Src/Render/Render_TreeShape.cpp"
    "../../../../Src/Render/Render_TreeText.cpp"
    "../../../../Src/Render/Render_Vertex.cpp"
    "../../../../Src/Render/Render_VertexPath.cpp"
    "../../../../Src/Render/Renderer2D.cpp"
    "../../../../Src/Render/Renderer2DImpl.cpp"
)

if (WIN32)
list(APPEND SRC_FILES
    "../../../../Src/Kernel/HeapPT/HeapPT_SysAllocWinAPI.cpp"
    "../../../../Src/Kernel/SF_ThreadsWinAPI.cpp"
    "../../../../Src/GFx/Net/GFx_Socket_Win32.cpp"
    "../../../../Src/GFx/GFx_FontProviderWin32.cpp"
    "../../../../Src/Render/FontProvider/Render_FontProviderWinAPI.cpp"
)
else()
list(APPEND SRC_FILES
    "../../../../Src/Kernel/SF_ThreadsPthread.cpp"
    "../../../../Src/GFx/Net/GFx_Socket_Std.cpp"
)
endif()

include(../BuildOptions.cmake)

target_link_libraries(${PROJECT_NAME} PRIVATE
    libjpeg
    libpng
)
