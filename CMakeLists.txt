cmake_minimum_required(VERSION 3.21)

# Automatically use the vcpkg toolchain on Windows for Protobuf
if (WIN32 AND EXISTS "C:/Users/Stephen/Downloads/vcpkg/scripts/buildsystems/vcpkg.cmake")
    set(CMAKE_TOOLCHAIN_FILE "C:/Users/Stephen/Downloads/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file" FORCE)
endif()

# Set Qt prefix path for Windows
if (WIN32 AND EXISTS "C:/Qt/6.10.3/msvc2022_64")
    list(APPEND CMAKE_PREFIX_PATH "C:/Qt/6.10.3/msvc2022_64")
endif()

project(FOHMediaShow)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable Qt MOC/UIC/RCC automatically
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# SDL2 removed. Qt QML Windows are used for live display output.

# Find Qt6 (fall back to Qt5 if needed)
find_package(Qt6 REQUIRED COMPONENTS Widgets Test Qml Quick Multimedia QuickControls2)
enable_testing()

# Find Protobuf
find_package(Protobuf REQUIRED)

# Generate C++ sources from .proto files
file(GLOB PROTO_FILES "third_party/ProPresenter7-Proto/proto/*.proto")
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTO_FILES})

# --- Core Library ---
add_library(fohmedia_core STATIC
    inc/DisplayEngine.h
    src/DisplayEngine.cpp
    src/ResizableRectItem.cpp
    inc/ResizableRectItem.h
    src/SlideLayout.cpp
    src/Slide.cpp
    inc/Slide.h
    src/SlideDeck.cpp
    inc/SlideDeck.h
    src/SlideDeckViewModel.cpp
    inc/SlideDeckViewModel.h
    src/ShowViewModel.cpp
    inc/ShowViewModel.h
    src/ArrangementViewModel.cpp
    inc/ArrangementViewModel.h
    src/Show.cpp
    inc/Show.h
    inc/SettingsManager.h
    src/SettingsManager.cpp
    inc/SlideLayout.h
    src/SlideLayout.cpp
    inc/FlowLayout.h
    src/FlowLayout.cpp
    inc/Arrangement.h
    src/Arrangement.cpp
    inc/DraggableComponentBtn.h
    inc/ArrangementDropZone.h
    inc/Library.h
    src/Library.cpp
    inc/ShowSerializer.h
    src/ShowSerializer.cpp
    inc/LyricsEditorViewModel.h
    src/LyricsEditorViewModel.cpp
    inc/ScreenModel.h
    src/ScreenModel.cpp
    inc/ShortcutManager.h
    src/ShortcutManager.cpp
    inc/LayoutEditorViewModel.h
    src/LayoutEditorViewModel.cpp
    inc/ThemeManager.h
    src/ThemeManager.cpp
    inc/ProPresenterImporter.h
    src/ProPresenterImporter.cpp
    inc/MediaViewModel.h
    src/MediaViewModel.cpp
    inc/AppContext.h
    src/AppContext.cpp
    inc/TimerManager.h
    src/TimerManager.cpp
    ${PROTO_SRCS}
    ${PROTO_HDRS}
)

target_include_directories(fohmedia_core PUBLIC 
    inc
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(fohmedia_core PUBLIC
    Qt6::Widgets
    Qt6::Qml
    Qt6::Quick
    Qt6::Multimedia
    Qt6::QuickControls2
    protobuf::libprotobuf
)

if(MSVC)
    target_compile_options(fohmedia_core PRIVATE /W4)
else()
    target_compile_options(fohmedia_core PRIVATE -Wall -Wextra -Wconversion)
endif()

# --- Executable ---
add_executable(FOHMediaShow WIN32 MACOSX_BUNDLE src/main.cpp resources.qrc)
if(WIN32)
    target_sources(FOHMediaShow PRIVATE app_icon.rc)
endif()
target_link_libraries(FOHMediaShow PRIVATE fohmedia_core)
set_target_properties(FOHMediaShow PROPERTIES OUTPUT_NAME "FOHMedia")

# Add QML module
qt_add_qml_module(FOHMediaShow
    URI fohmedia
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/ServicesView.qml
        qml/MediaView.qml
        qml/MediaPickerDialog.qml
        qml/LyricsEditorView.qml
        qml/SettingsView.qml
        qml/LayoutEditorView.qml
        qml/ResizableCanvasItem.qml
        qml/SlidePreview.qml
        qml/TimerControlCard.qml
        qml/LiveOutputWindow.qml
        qml/DisplaysView.qml
    RESOURCES
        qml/qtquickcontrols2.conf
)

# Copy the resource files to the build directory after compilation
add_custom_command(TARGET FOHMediaShow POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/Media/logo0.png"
        "${CMAKE_CURRENT_SOURCE_DIR}/Media/BebasNeue-Regular.ttf"
        "${CMAKE_CURRENT_SOURCE_DIR}/program_files/ShowExample.yaml"
        "${CMAKE_CURRENT_SOURCE_DIR}/program_files/HoneyInTheRock.json"
        $<TARGET_FILE_DIR:FOHMediaShow>
)

if (WIN32)
    # Find and run windeployqt to automatically copy all required Qt DLLs next to the executable
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "C:/Qt/6.10.3/msvc2022_64/bin")
    if (WINDEPLOYQT_EXECUTABLE)
        add_custom_command(TARGET FOHMediaShow POST_BUILD
            COMMAND "${WINDEPLOYQT_EXECUTABLE}" --qmldir "${CMAKE_CURRENT_SOURCE_DIR}/qml" --no-translations "$<TARGET_FILE:FOHMediaShow>"
            COMMENT "Running windeployqt to copy Qt DLLs..."
        )
    endif()
endif()

if(UNIX AND NOT APPLE)
    configure_file(FOHMediaShow.desktop.in FOHMediaShow.desktop @ONLY)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/FOHMediaShow.desktop DESTINATION share/applications)
endif()

# --- Testing ---
add_executable(FOHMediaShow_GUITest tests/GUITests.cpp)
target_link_libraries(FOHMediaShow_GUITest PRIVATE fohmedia_core Qt6::Test)
add_test(NAME GUITests COMMAND FOHMediaShow_GUITest)

# add_executable(test_engine sandbox/test_engine.cpp)
# target_link_libraries(test_engine PRIVATE fohmedia_core Qt6::Test)

# --- Installation & Packaging ---
install(TARGETS FOHMediaShow DESTINATION .)
install(FILES 
    "$<TARGET_RUNTIME_DLLS:FOHMediaShow>"
    "${CMAKE_CURRENT_SOURCE_DIR}/Media/logo0.png"
    "${CMAKE_CURRENT_SOURCE_DIR}/Media/BebasNeue-Regular.ttf"
    "${CMAKE_CURRENT_SOURCE_DIR}/program_files/ShowExample.yaml"
    "${CMAKE_CURRENT_SOURCE_DIR}/program_files/HoneyInTheRock.json"
    DESTINATION .
)

if (WIN32)
    if (WINDEPLOYQT_EXECUTABLE)
        install(CODE "execute_process(COMMAND \"${WINDEPLOYQT_EXECUTABLE}\" --qmldir \"${CMAKE_CURRENT_SOURCE_DIR}/qml\" --no-translations \"\${CMAKE_INSTALL_PREFIX}/FOHMedia.exe\")")
    endif()
    
    set(CPACK_GENERATOR "NSIS")
    set(CPACK_PACKAGE_NAME "FOHMediaShow")
    set(CPACK_PACKAGE_VENDOR "FOHMedia")
    set(CPACK_PACKAGE_VERSION "1.0.0")
    set(CPACK_PACKAGE_FILE_NAME "FOHMediaShow-1.0.0-win64")
    set(CPACK_PACKAGE_INSTALL_DIRECTORY "FOHMediaShow")
    
    # NSIS specific configurations for Start Menu shortcuts
    set(CPACK_NSIS_DISPLAY_NAME "FOHMediaShow")
    set(CPACK_NSIS_EXECUTABLES_DIRECTORY ".")
    set(CPACK_NSIS_MUI_ICON "${CMAKE_CURRENT_SOURCE_DIR}/Media/logo0.ico")
    set(CPACK_NSIS_MUI_UNIICON "${CMAKE_CURRENT_SOURCE_DIR}/Media/logo0.ico")
    set(CPACK_PACKAGE_EXECUTABLES "FOHMedia;FOHMedia")
    set(CPACK_NSIS_MUI_FINISHPAGE_RUN "FOHMedia.exe")
    
    include(CPack)
    
    # Explicit custom target for CPack so it appears in IDE target lists
    add_custom_target(create_installer
        DEPENDS FOHMediaShow
        COMMAND ${CMAKE_CPACK_COMMAND} -G NSIS
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Generating Windows Installer via CPack..."
    )
endif()

# --- Custom CI Triggers ---
# This target allows you to trigger the Mac build directly from CLion's run configurations
add_custom_target(PushToMacCI
    COMMAND sh -c "git push -f github `git commit-tree HEAD^{tree} -m 'Mac CI/CD Build'`:refs/heads/main"
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    VERBATIM
    COMMENT "Pushing flattened commit to GitHub for macOS CI build..."
)
