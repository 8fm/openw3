
-- GLUT
-- Only needed for host platforms that don't have it

if os.is("windows") then

defaultproject("glut", nil, "source")
platforms { "x32", "x64" }
kind "SharedLib"
folder "external"
includedirs { "external/glut/include" }
defines { "_CRT_SECURE_NO_WARNINGS" }
links { "winmm", "opengl32", "glu32" }
glutfiles = { "glut_8x13.c", "glut_9x15.c", "glut_bitmap.c", "glut_bwidth.c", "glut_cindex.c", "glut_cmap.c", "glut_cursor.c", "glut_dials.c", "glut_dstr.c", "glut_event.c", "glut_ext.c", "glut_fullscrn.c", "glut_gamemode.c", "glut_get.c", "glut_hel10.c", "glut_hel12.c", "glut_hel18.c", "glut_init.c", "glut_input.c", "glut_joy.c", "glut_key.c", "glut_keyctrl.c", "glut_keyup.c", "glut_mesa.c", "glut_modifier.c", "glut_mroman.c", "glut_overlay.c", "glut_roman.c", "glut_shapes.c", "glut_space.c", "glut_stroke.c", "glut_swap.c", "glut_swidth.c", "glut_tablet.c", "glut_teapot.c", "glut_tr10.c", "glut_tr24.c", "glut_util.c", "glut_vidresize.c", "glut_warp.c", "glut_win.c", "glut_winmisc.c", "win32_glx.c", "win32_menu.c", "win32_util.c", "win32_winproc.c", "win32_x11.c" }
for _,i in pairs(glutfiles) do
	files("external/glut/src/" .. i)
end
-- todo: need a mechanism for specifying extra rel package files
files("external/glut/src/*.h")
files("external/glut/include/GL/*.h")
files("external/opengl/gl/*.h")

end
