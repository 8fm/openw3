project(CommunicationCentral)

add_definitions(-DPROXYCENTRAL_CONNECTED)
if (MSVC)
    add_compile_options(/wd4355)
endif()

set(SRC_FILES
    "../CodeBase/CommandChannel.cpp"
    "../CodeBase/Communication.cpp"
    "../CodeBase/CommunicationCentral.cpp"
    "../CodeBase/DiscoveryChannel.cpp"
    "../CodeBase/IncomingChannel.cpp"
    "../CodeBase/NetworkCaptureWriter.cpp"
    "../CodeBase/NotificationChannel.cpp"
    "GameSocket.cpp"
    "GameSocketAddr.cpp"
    "Network.cpp"
    "../../Common/ConsoleDiscoveryMessage.cpp"
    "../../Common/IPConnectorPorts.cpp"
    "../../Common/Serializer.cpp"
)

add_library(${PROJECT_NAME} STATIC ${SRC_FILES})

target_include_directories(${PROJECT_NAME} PRIVATE
    "."
    "../../Common"
    "../CodeBase"
    "../../../../../include"
    "../../../SoundEngineProxy/Remote/CodeBase"
    "../../../SoundEngineProxy/Common"
    "../../../AkAudiolib/PC"
    "../../../AkAudiolib/Common"
)

target_link_libraries(${PROJECT_NAME})

target_precompile_headers(${PROJECT_NAME}
    PRIVATE
    stdafx.cpp
)
