cmake_minimum_required(VERSION 3.19)
project(Radar_Simulator LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets Network)

qt_standard_project_setup()

qt_add_executable(Radar_Simulator
    WIN32 MACOSX_BUNDLE
    main.cpp
    core/coordconverter.cpp
    core/coordconverter.h
    core/ScenarioIO.cpp
    core/ScenarioIO.h
    core/Target.cpp
    core/Target.h
    core/targetmanager.cpp
    core/targetmanager.h
    core/simengine.cpp
    core/simengine.h
    core/udpprotocol.cpp
    core/udpprotocol.h
    core/udptracksource.cpp
    core/udptracksource.h
    core/udptracksender.cpp
    core/udptracksender.h
    ui/radarwidget.cpp
    ui/radarwidget.h
    ui/TargetTableModel.cpp
    ui/TargetTableModel.h

)

target_include_directories(Radar_Simulator PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(Radar_Simulator
    PRIVATE
        Qt::Core
        Qt::Widgets
        Qt::Network
)

install(TARGETS Radar_Simulator
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET Radar_Simulator
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})

# ===== A3：演示素材生成工具（可选，生成 README 用的 GIF/PNG） =====
qt_add_executable(demo_capture
    tools/demo_capture.cpp
    tools/gifencoder.cpp
    tools/gifencoder.h
    core/coordconverter.cpp
    core/coordconverter.h
    core/Target.cpp
    core/Target.h
    core/targetmanager.cpp
    core/targetmanager.h
    core/simengine.cpp
    core/simengine.h
    ui/radarwidget.cpp
    ui/radarwidget.h
)
target_include_directories(demo_capture PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(demo_capture PRIVATE Qt::Core Qt::Gui Qt::Widgets)

# ===== 单元测试（B1）=====
option(BUILD_TESTING "构建并注册单元测试" ON)

if(BUILD_TESTING)
    enable_testing()
    find_package(Qt6 6.5 REQUIRED COMPONENTS Test)

    set(CORE_TEST_SOURCES
        core/coordconverter.cpp
        core/coordconverter.h
        core/Target.cpp
        core/Target.h
        core/targetmanager.cpp
        core/targetmanager.h
        core/simengine.cpp
        core/simengine.h
        core/udpprotocol.cpp
        core/udpprotocol.h
        core/ScenarioIO.cpp
        core/ScenarioIO.h
    )

    function(add_core_test test_name)
        qt_add_executable(${test_name} tests/${test_name}.cpp ${CORE_TEST_SOURCES})
        target_include_directories(${test_name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
        target_link_libraries(${test_name} PRIVATE Qt::Core Qt::Test)
        add_test(NAME ${test_name} COMMAND ${test_name})
    endfunction()

    add_core_test(test_coordconverter)
    add_core_test(test_target)
    add_core_test(test_targetmanager)
    add_core_test(test_scenarioio)
    add_core_test(test_simengine)
    add_core_test(test_udpprotocol)

    # A3：GIF 编码器单测（需 Qt::Gui 读回验证）
    qt_add_executable(test_gifencoder
        tests/test_gifencoder.cpp
        tools/gifencoder.cpp
        tools/gifencoder.h
    )
    target_include_directories(test_gifencoder PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(test_gifencoder PRIVATE Qt::Core Qt::Gui Qt::Test)
    add_test(NAME test_gifencoder COMMAND test_gifencoder)
endif()
