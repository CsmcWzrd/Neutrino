cmake_minimum_required(VERSION 3.12)
project(Neutrino VERSION 0.3.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(Neutrino_EXAMPLE_TARGETS)
function(neutrino_add_example target source_file)
    add_executable(${target} ${source_file})
    target_link_libraries(${target} PRIVATE Neutrino)
    target_include_directories(${target} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
    set(Neutrino_EXAMPLE_TARGETS ${Neutrino_EXAMPLE_TARGETS} ${target} PARENT_SCOPE)
endfunction()

if(WIN32)
    add_library(Neutrino STATIC
        src/win32/Neu_Win32.cpp
    )
    target_include_directories(Neutrino PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
    target_compile_definitions(Neutrino PUBLIC WIN32_LEAN_AND_MEAN NOMINMAX UNICODE _UNICODE _CRT_SECURE_NO_WARNINGS)
    target_link_libraries(Neutrino PUBLIC user32 gdi32 comctl32)

    add_executable(neutrino_win32_demo WIN32 examples/windows/neutrino_win32_demo.cpp)
    target_link_libraries(neutrino_win32_demo PRIVATE Neutrino)
    list(APPEND Neutrino_EXAMPLE_TARGETS neutrino_win32_demo)
else()
    find_package(X11 REQUIRED)
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(XFT QUIET xft)
    endif()
    add_library(Neutrino STATIC
        src/Neu_Application.cpp
        src/Neu_AdditionalControls.cpp
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
    if(XFT_FOUND)
        target_compile_definitions(Neutrino PUBLIC NEUTRINO_USE_XFT)
        target_include_directories(Neutrino PUBLIC ${XFT_INCLUDE_DIRS})
        target_link_libraries(Neutrino PUBLIC ${XFT_LIBRARIES})
    endif()
    target_link_libraries(Neutrino PUBLIC ${X11_LIBRARIES} ${CMAKE_DL_LIBS})
endif()

neutrino_add_example(neutrino_demo examples/demo.cpp)
neutrino_add_example(neutrino_test_all_controls examples/test_all_controls.cpp)
neutrino_add_example(neutrino_test_windows examples/test_windows.cpp)
neutrino_add_example(neutrino_test_typed_views examples/test_typed_views.cpp)
neutrino_add_example(neutrino_test_smooth_graphics examples/test_smooth_graphics.cpp)
neutrino_add_example(neutrino_test_treeview_collapse examples/test_treeview_collapse.cpp)
neutrino_add_example(neutrino_test_heavy_data examples/test_heavy_data.cpp)
neutrino_add_example(neutrino_test_01_buttons_icons examples/neutrino_test_01_buttons_icons.cpp)
neutrino_add_example(neutrino_test_02_text_inputs examples/neutrino_test_02_text_inputs.cpp)
neutrino_add_example(neutrino_test_03_lists_combo_autoscroll examples/neutrino_test_03_lists_combo_autoscroll.cpp)
neutrino_add_example(neutrino_test_04_listview_typed_data examples/neutrino_test_04_listview_typed_data.cpp)
neutrino_add_example(neutrino_test_05_treeview_collapse examples/neutrino_test_05_treeview_collapse.cpp)
neutrino_add_example(neutrino_test_06_placement_layout_scaling examples/neutrino_test_06_placement_layout_scaling.cpp)
neutrino_add_example(neutrino_test_07_popup_menu_categories examples/neutrino_test_07_popup_menu_categories.cpp)
neutrino_add_example(neutrino_test_08_richtext_code examples/neutrino_test_08_richtext_code.cpp)
neutrino_add_example(neutrino_test_09_readonly_richtext_icons examples/neutrino_test_09_readonly_richtext_icons.cpp)
neutrino_add_example(neutrino_test_10_images_progress_labels examples/neutrino_test_10_images_progress_labels.cpp)
neutrino_add_example(neutrino_test_11_rendering_buffering examples/neutrino_test_11_rendering_buffering.cpp)
neutrino_add_example(neutrino_test_12_scroll_windows_heavy_data examples/neutrino_test_12_scroll_windows_heavy_data.cpp)
neutrino_add_example(neutrino_test_13_swing_swt_controls examples/neutrino_test_13_swing_swt_controls.cpp)
neutrino_add_example(neutrino_test_14_label_offsets_themes examples/neutrino_test_14_label_offsets_themes.cpp)
neutrino_add_example(neutrino_test_15_selection_controls examples/neutrino_test_15_selection_controls.cpp)
neutrino_add_example(neutrino_test_16_value_controls examples/neutrino_test_16_value_controls.cpp)
neutrino_add_example(neutrino_test_17_tabs_toolbar_splitter examples/neutrino_test_17_tabs_toolbar_splitter.cpp)
neutrino_add_example(neutrino_test_18_stage2_scroll_controls examples/neutrino_test_18_stage2_scroll_controls.cpp)

install(TARGETS Neutrino ${Neutrino_EXAMPLE_TARGETS} ARCHIVE DESTINATION lib RUNTIME DESTINATION bin)
install(DIRECTORY include/Neutrino DESTINATION include)
install(DIRECTORY assets DESTINATION share/Neutrino)
