cmake_minimum_required(VERSION 3.16)

project(KristenLab1 VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

set(PROJECT_SOURCES
    main.cpp

    mainwindow.cpp
    mainwindow.h
    mainwindow.ui

    leveleditordialog.cpp
    leveleditordialog.h

    gamescene.cpp
    gamescene.h

    audioplayer.cpp
    audioplayer.h

    ball.cpp
    ball.h

    level.cpp
    level.h

    levelmanager.cpp
    levelmanager.h

    progressmanager.cpp
    progressmanager.h

    constants.h
    tiledefs.h

    resources.qrc
)

if(QT_VERSION_MAJOR GREATER_EQUAL 6)
    qt_add_executable(KristenLab1
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
else()
    if(ANDROID)
        add_library(KristenLab1 SHARED
            ${PROJECT_SOURCES}
        )
    else()
        add_executable(KristenLab1
            ${PROJECT_SOURCES}
        )
    endif()
endif()

target_link_libraries(KristenLab1 PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)

if(${QT_VERSION} VERSION_LESS 6.1.0)
    set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.KristenLab1)
endif()

set_target_properties(KristenLab1 PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

# 阶段 17：构建后自动把外部地图文件夹复制到 exe 所在目录
# 这样程序运行时可以从 exe 同级目录读取 levels 和 custom_levels
add_custom_command(TARGET KristenLab1 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/levels"
        "$<TARGET_FILE_DIR:KristenLab1>/levels"

    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/custom_levels"
        "$<TARGET_FILE_DIR:KristenLab1>/custom_levels"
)

include(GNUInstallDirs)

install(TARGETS KristenLab1
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(KristenLab1)
endif()
