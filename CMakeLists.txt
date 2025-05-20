cmake_minimum_required(VERSION 3.16)

project(GUI_power_supply VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

set(PROJECT_SOURCES
        main.cpp
        GUI_MAIN_POWER_SUPPLY.cpp
        GUI_MAIN_POWER_SUPPLY.h
        UI_POWER_SUPPLY.ui
        ${CMAKE_CURRENT_SOURCE_DIR}/drivers/drv_power_supply.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/drivers/drv_power_supply.h
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/visa
)

# VISA library paths
set(VISA_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lib/visa")
set(VISA_LIB "${CMAKE_CURRENT_SOURCE_DIR}/lib/visa/visa64.lib")

# VISA library paths
# set(VISA_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lib/visa")
# set(VISA_LIB "${CMAKE_CURRENT_SOURCE_DIR}/lib/visa/visa64.lib")

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(GUI_power_supply
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        resources.qrc
    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET GUI_power_supply APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(GUI_power_supply SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(GUI_power_supply
            ${PROJECT_SOURCES}
        )
    endif()
endif()

target_link_libraries(GUI_power_supply PRIVATE Qt${QT_VERSION_MAJOR}::Widgets ${VISA_LIB})

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.GUI_power_supply)
endif()
set_target_properties(GUI_power_supply PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

# VISA include directory
include_directories(${VISA_INCLUDE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/drivers)

include(GNUInstallDirs)
install(TARGETS GUI_power_supply
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(GUI_power_supply)
endif()
