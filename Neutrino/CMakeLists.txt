cmake_minimum_required(VERSION 3.12)
project(Neutrino VERSION 0.2.1 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(WIN32)
    add_library(Neutrino STATIC
        src/win32/Neu_Win32.cpp
    )
    target_include_directories(Neutrino PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
    target_compile_definitions(Neutrino PUBLIC WIN32_LEAN_AND_MEAN NOMINMAX UNICODE _UNICODE)
    target_link_libraries(Neutrino PUBLIC user32 gdi32 comctl32)

    add_executable(neutrino_win32_demo WIN32 examples/windows/neutrino_win32_demo.cpp)
    target_link_libraries(neutrino_win32_demo PRIVATE Neutrino)

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
    if(XFT_FOUND)
        target_compile_definitions(Neutrino PUBLIC NEUTRINO_USE_XFT)
        target_include_directories(Neutrino PUBLIC ${XFT_INCLUDE_DIRS})
        target_link_libraries(Neutrino PUBLIC ${XFT_LIBRARIES})
    endif()
    target_link_libraries(Neutrino PUBLIC ${X11_LIBRARIES} ${CMAKE_DL_LIBS})

    set(Neutrino_EXAMPLES
        neutrino_demo
        neutrino_test_all_controls
        neutrino_test_windows
        neutrino_test_typed_views
        neutrino_test_smooth_graphics
        neutrino_test_treeview_collapse
        neutrino_test_heavy_data
    )

    add_executable(neutrino_demo examples/demo.cpp)
    add_executable(neutrino_test_all_controls examples/test_all_controls.cpp)
    add_executable(neutrino_test_windows examples/test_windows.cpp)
    add_executable(neutrino_test_typed_views examples/test_typed_views.cpp)
    add_executable(neutrino_test_smooth_graphics examples/test_smooth_graphics.cpp)
    add_executable(neutrino_test_treeview_collapse examples/test_treeview_collapse.cpp)
    add_executable(neutrino_test_heavy_data examples/test_heavy_data.cpp)

    foreach(example ${Neutrino_EXAMPLES})
        target_link_libraries(${example} PRIVATE Neutrino)
    endforeach()

    install(TARGETS Neutrino ${Neutrino_EXAMPLES} ARCHIVE DESTINATION lib RUNTIME DESTINATION bin)
endif()

install(DIRECTORY include/Neutrino DESTINATION include)
install(DIRECTORY assets DESTINATION share/Neutrino)
