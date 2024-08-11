
-- Umbra tutorial application

umbraproject("tutorial", nil, "all")
folder "samples"
kind "ConsoleApp"
sourcedirs "samples/tutorial"
includedirs { "utils/main" }
links { "runtime", "common" }

configuration { "x32 or x64" }
links { "optimizer" }

configuration { "xbox360 or ps3 or psvita or durango or cafe or ios_* or orbis" }
defines "RUNTIME_ONLY"
