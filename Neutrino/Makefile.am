AUTOMAKE_OPTIONS = foreign subdir-objects
AM_CPPFLAGS = -I$(top_srcdir)/include
lib_LIBRARIES = libNeutrino.a
libNeutrino_a_SOURCES = src/Neu_Application.cpp \
    src/Neu_Button.cpp \
    src/Neu_ComboBox.cpp \
    src/Neu_Control.cpp \
    src/Neu_Graphics.cpp \
    src/Neu_IconBmp.cpp \
    src/Neu_ScrollWindow.cpp \
    src/Neu_ScrollBar.cpp \
    src/Neu_RichTextCode.cpp \
    src/Neu_ReadOnlyRichText.cpp \
    src/Neu_ProgressSquare.cpp \
    src/Neu_Label.cpp \
    src/Neu_ImageView.cpp \
    src/Neu_ListView.cpp \
    src/Neu_Listbox.cpp \
    src/Neu_Multilinetextbox.cpp \
    src/Neu_Passwordbox.cpp \
    src/Neu_Placement.cpp \
    src/Neu_PopWindowMenu.cpp \
    src/Neu_Textbox.cpp \
    src/Neu_Theme.cpp \
    src/Neu_TreeView.cpp \
    src/Neu_TypeInterpreter.cpp \
    src/Neu_Window.cpp
include_HEADERS = include/Neutrino/Neutrino.hpp
bin_PROGRAMS = neutrino_demo neutrino_test_all_controls neutrino_test_windows neutrino_test_typed_views neutrino_test_smooth_graphics neutrino_test_treeview_collapse neutrino_test_heavy_data
neutrino_demo_SOURCES = examples/demo.cpp
neutrino_demo_LDADD = libNeutrino.a -lX11 -ldl
neutrino_test_all_controls_SOURCES = examples/test_all_controls.cpp
neutrino_test_all_controls_LDADD = libNeutrino.a -lX11 -ldl
neutrino_test_windows_SOURCES = examples/test_windows.cpp
neutrino_test_windows_LDADD = libNeutrino.a -lX11 -ldl
neutrino_test_typed_views_SOURCES = examples/test_typed_views.cpp
neutrino_test_typed_views_LDADD = libNeutrino.a -lX11 -ldl
neutrino_test_smooth_graphics_SOURCES = examples/test_smooth_graphics.cpp
neutrino_test_smooth_graphics_LDADD = libNeutrino.a -lX11 -ldl
neutrino_test_treeview_collapse_SOURCES = examples/test_treeview_collapse.cpp
neutrino_test_treeview_collapse_LDADD = libNeutrino.a -lX11 -ldl

neutrino_test_heavy_data_SOURCES = examples/test_heavy_data.cpp
neutrino_test_heavy_data_LDADD = libNeutrino.a -lX11 -ldl
