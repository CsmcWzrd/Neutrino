cmake_minimum_required(VERSION 3.12)
project(Neutrino VERSION 0.3.1 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(NEUTRINO_EXAMPLE_SOURCES
    demo
    neutrino_test_01_buttons_icons
    neutrino_test_02_text_inputs
    neutrino_test_03_lists_combo_autoscroll
    neutrino_test_04_listview_typed_data
    neutrino_test_05_treeview_collapse
    neutrino_test_06_placement_layout_scaling
    neutrino_test_07_popup_menu_categories
    neutrino_test_08_richtext_code
    neutrino_test_09_readonly_richtext_icons
    neutrino_test_10_images_progress_labels
    neutrino_test_11_rendering_buffering
    neutrino_test_12_scroll_windows_heavy_data
    test_all_controls
    test_heavy_data
    test_progress_square
    test_readonly_richtext_spacing
    test_richtext_formatting
    test_scroll_clip_select
    test_smooth_graphics
    test_treeview_collapse
    test_typed_views
    test_windows
    test_wordwrap_hints
)

if(WIN32)
    add_library(Neutrino STATIC
        src/win32/Neu_Win32.cpp
    )
    target_include_directories(Neutrino PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
    target_compile_definitions(Neutrino PUBLIC WIN32_LEAN_AND_MEAN NOMINMAX UNICODE _UNICODE _CRT_SECURE_NO_WARNINGS)
    target_compile_features(Neutrino PUBLIC cxx_std_17)
    target_link_libraries(Neutrino PUBLIC user32 gdi32 comctl32)

    add_executable(neutrino_win32_demo WIN32 examples/windows/neutrino_win32_demo.cpp)
    target_link_libraries(neutrino_win32_demo PRIVATE Neutrino)
    target_compile_definitions(neutrino_win32_demo PRIVATE _CRT_SECURE_NO_WARNINGS)

    foreach(example ${NEUTRINO_EXAMPLE_SOURCES})
        add_executable(${example} examples/${example}.cpp)
        target_link_libraries(${example} PRIVATE Neutrino)
        target_compile_features(${example} PRIVATE cxx_std_17)
        target_compile_definitions(${example} PRIVATE _CRT_SECURE_NO_WARNINGS)
    endforeach()

    install(TARGETS Neutrino neutrino_win32_demo ARCHIVE DESTINATION lib RUNTIME DESTINATION bin)
else()
    find_package(X11 REQUIRED)
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(XFT QUIET xft)
    endif()
    add_library(Neutrino STATIC
                src/Neu_Application.cpp
        src/Neu_Button.cpp
        src/Neu_ComboBox.cpp
        src/Neu_Control.cpp
        src/Neu_Graphics.cpp
        src/Neu_IconBmp.cpp
        src/Neu_ScrollWindow.cpp
        src/Neu_ScrollBar.cpp
        src/Neu_RichTextCode.cpp
        src/Neu_ReadOnlyRichText.cpp
        src/Neu_ProgressSquare.cpp
        src/Neu_Label.cpp
        src/Neu_ImageView.cpp
        src/Neu_ListView.cpp
        src/Neu_Listbox.cpp
        src/Neu_Multilinetextbox.cpp
        src/Neu_Passwordbox.cpp
        src/Neu_Placement.cpp
        src/Neu_PopWindowMenu.cpp
        src/Neu_Textbox.cpp
        src/Neu_Theme.cpp
        src/Neu_TreeView.cpp
        src/Neu_TypeInterpreter.cpp
        src/Neu_Window.cpp
    )
    target_include_directories(Neutrino PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include ${X11_INCLUDE_DIR})
    target_compile_features(Neutrino PUBLIC cxx_std_17)
    if(XFT_FOUND)
        target_compile_definitions(Neutrino PUBLIC NEUTRINO_USE_XFT)
        target_include_directories(Neutrino PUBLIC ${XFT_INCLUDE_DIRS})
        target_link_libraries(Neutrino PUBLIC ${XFT_LIBRARIES})
    endif()
    target_link_libraries(Neutrino PUBLIC ${X11_LIBRARIES} ${CMAKE_DL_LIBS})

    set(Neutrino_EXAMPLES)
    foreach(example ${NEUTRINO_EXAMPLE_SOURCES})
        add_executable(${example} examples/${example}.cpp)
        target_link_libraries(${example} PRIVATE Neutrino)
        list(APPEND Neutrino_EXAMPLES ${example})
    endforeach()

    install(TARGETS Neutrino ${Neutrino_EXAMPLES} ARCHIVE DESTINATION lib RUNTIME DESTINATION bin)
endif()

install(DIRECTORY include/Neutrino DESTINATION include)
install(DIRECTORY assets DESTINATION share/Neutrino)
