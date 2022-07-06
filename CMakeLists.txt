cmake_minimum_required(VERSION 3.20)
project(BankReconcTool VERSION 0.1)

# for Vim + YouCompleteMe; later run ln -s build/compile_commands.json ./compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# tell CMake where Qt6 is installed if it complains about not finding Qt6Config.cmake
# set(CMAKE_PREFIX_PATH "/path/to/Qt/6.2.2/gcc_64")

message("CMAKE_SYSTEM_NAME ${CMAKE_SYSTEM_NAME}")
message("CMAKE_SYSTEM_VERSION ${CMAKE_SYSTEM_VERSION}")


set(CMAKE_CXX_STANDARD 20)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOUIC_SEARCH_PATHS ui)

if (WIN32)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)
endif ()

find_package(Qt6 COMPONENTS
        Core
        Widgets
        REQUIRED)


add_subdirectory(bank-reconc-lib)

set(INCLUDE_DIRECTORIES ${Qt6Widgets_INCLUDE_DIRS} bank-reconc-lib include)

include_directories(${INCLUDE_DIRECTORIES})


add_executable(BankReconcTool
        ui/mainwindow.ui
        ui/filesettings.ui
        ui/aboutdialog.ui

        br-res.qrc

        src/BR_MainWindow.cpp include/BR_MainWindow.h
        main.cpp
        src/MissingEntryModel.cpp include/MissingEntryModel.h
        src/EntryMatchModel.cpp include/EntryMatchModel.h
        include/helpers.h src/helpers.cpp
        src/FileSettingsDialog.cpp include/FileSettingsDialog.h
        src/mainwindow-tbl-selection.cpp
        src/EntryDataModel.cpp include/EntryDataModel.h
        include/AboutDialog.h)

message("project version: ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}")

target_compile_definitions(BankReconcTool PUBLIC PROJECT_VERSION_MAJOR=${PROJECT_VERSION_MAJOR})
target_compile_definitions(BankReconcTool PUBLIC PROJECT_VERSION_MINOR=${PROJECT_VERSION_MINOR})

target_include_directories(BankReconcTool PUBLIC Qt6Widgets_INCLUDE_DIRS bank-reconc-lib)


target_link_libraries(BankReconcTool
        PUBLIC
        Qt6::Core
        Qt6::Widgets
        bank-reconc-lib
        )

if (WIN32)
    add_custom_command(
            TARGET BankReconcTool POST_BUILD
            COMMAND ${CMAKE_PREFIX_PATH}/bin/windeployqt.exe
            --no-translations --no-system-d3d-compiler --no-virtualkeyboard --no-compiler-runtime --no-opengl-sw
            "$<TARGET_FILE:BankReconcTool>" VERBATIM)
endif ()
