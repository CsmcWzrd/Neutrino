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
bin_PROGRAMS = demo neutrino_test_01_buttons_icons neutrino_test_02_text_inputs neutrino_test_03_lists_combo_autoscroll neutrino_test_04_listview_typed_data neutrino_test_05_treeview_collapse neutrino_test_06_placement_layout_scaling neutrino_test_07_popup_menu_categories neutrino_test_08_richtext_code neutrino_test_09_readonly_richtext_icons neutrino_test_10_images_progress_labels neutrino_test_11_rendering_buffering neutrino_test_12_scroll_windows_heavy_data test_all_controls test_heavy_data test_progress_square test_readonly_richtext_spacing test_richtext_formatting test_scroll_clip_select test_smooth_graphics test_treeview_collapse test_typed_views test_windows test_wordwrap_hints
demo_SOURCES = examples/demo.cpp
demo_LDADD = libNeutrino.a -lX11 -ldl
neutrino_test_01_buttons_icons_SOURCES = examples/neutrino_test_01_buttons_icons.cpp
neutrino_test_01_buttons_icons_LDADD = libNeutrino.a -lX11 -ldl
neutrino_test_02_text_inputs_SOURCES = examples/neutrino_test_02_text_inputs.cpp
neutrino_test_02_text_inputs_LDADD = libNeutrino.a -lX11 -ldl
neutrino_test_03_lists_combo_autoscroll_SOURCES = examples/neutrino_test_03_lists_combo_autoscroll.cpp
neutrino_test_03_lists_combo_autoscroll_LDADD = libNeutrino.a -lX11 -ldl
neutrino_test_04_listview_typed_data_SOURCES = examples/neutrino_test_04_listview_typed_data.cpp
neutrino_test_04_listview_typed_data_LDADD = libNeutrino.a -lX11 -ldl
neutrino_test_05_treeview_collapse_SOURCES = examples/neutrino_test_05_treeview_collapse.cpp
neutrino_test_05_treeview_collapse_LDADD = libNeutrino.a -lX11 -ldl
neutrino_test_06_placement_layout_scaling_SOURCES = examples/neutrino_test_06_placement_layout_scaling.cpp
neutrino_test_06_placement_layout_scaling_LDADD = libNeutrino.a -lX11 -ldl
neutrino_test_07_popup_menu_categories_SOURCES = examples/neutrino_test_07_popup_menu_categories.cpp
neutrino_test_07_popup_menu_categories_LDADD = libNeutrino.a -lX11 -ldl
neutrino_test_08_richtext_code_SOURCES = examples/neutrino_test_08_richtext_code.cpp
neutrino_test_08_richtext_code_LDADD = libNeutrino.a -lX11 -ldl
neutrino_test_09_readonly_richtext_icons_SOURCES = examples/neutrino_test_09_readonly_richtext_icons.cpp
neutrino_test_09_readonly_richtext_icons_LDADD = libNeutrino.a -lX11 -ldl
neutrino_test_10_images_progress_labels_SOURCES = examples/neutrino_test_10_images_progress_labels.cpp
neutrino_test_10_images_progress_labels_LDADD = libNeutrino.a -lX11 -ldl
neutrino_test_11_rendering_buffering_SOURCES = examples/neutrino_test_11_rendering_buffering.cpp
neutrino_test_11_rendering_buffering_LDADD = libNeutrino.a -lX11 -ldl
neutrino_test_12_scroll_windows_heavy_data_SOURCES = examples/neutrino_test_12_scroll_windows_heavy_data.cpp
neutrino_test_12_scroll_windows_heavy_data_LDADD = libNeutrino.a -lX11 -ldl
test_all_controls_SOURCES = examples/test_all_controls.cpp
test_all_controls_LDADD = libNeutrino.a -lX11 -ldl
test_heavy_data_SOURCES = examples/test_heavy_data.cpp
test_heavy_data_LDADD = libNeutrino.a -lX11 -ldl
test_progress_square_SOURCES = examples/test_progress_square.cpp
test_progress_square_LDADD = libNeutrino.a -lX11 -ldl
test_readonly_richtext_spacing_SOURCES = examples/test_readonly_richtext_spacing.cpp
test_readonly_richtext_spacing_LDADD = libNeutrino.a -lX11 -ldl
test_richtext_formatting_SOURCES = examples/test_richtext_formatting.cpp
test_richtext_formatting_LDADD = libNeutrino.a -lX11 -ldl
test_scroll_clip_select_SOURCES = examples/test_scroll_clip_select.cpp
test_scroll_clip_select_LDADD = libNeutrino.a -lX11 -ldl
test_smooth_graphics_SOURCES = examples/test_smooth_graphics.cpp
test_smooth_graphics_LDADD = libNeutrino.a -lX11 -ldl
test_treeview_collapse_SOURCES = examples/test_treeview_collapse.cpp
test_treeview_collapse_LDADD = libNeutrino.a -lX11 -ldl
test_typed_views_SOURCES = examples/test_typed_views.cpp
test_typed_views_LDADD = libNeutrino.a -lX11 -ldl
test_windows_SOURCES = examples/test_windows.cpp
test_windows_LDADD = libNeutrino.a -lX11 -ldl
test_wordwrap_hints_SOURCES = examples/test_wordwrap_hints.cpp
test_wordwrap_hints_LDADD = libNeutrino.a -lX11 -ldl
