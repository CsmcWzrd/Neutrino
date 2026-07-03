CXX ?= g++
AR ?= ar
CXXFLAGS ?= -std=c++17 -Wall -Wextra -O2
XFT_CFLAGS := $(shell pkg-config --cflags xft 2>/dev/null)
XFT_LIBS := $(shell pkg-config --libs xft 2>/dev/null)
XFT_CPPFLAGS := $(if $(XFT_LIBS),-DNEUTRINO_USE_XFT $(XFT_CFLAGS),)
CPPFLAGS ?= -Iinclude
LDFLAGS ?= -lX11 -ldl $(XFT_LIBS)
SRC = src/Neu_Application.cpp \
    src/Neu_AdditionalControls.cpp \
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
OBJ = $(SRC:src/%.cpp=build/%.o)
LIB = build/libNeutrino.a
EXAMPLES = build/neutrino_demo \
    build/neutrino_test_all_controls \
    build/neutrino_test_windows \
    build/neutrino_test_typed_views \
    build/neutrino_test_smooth_graphics \
    build/neutrino_test_treeview_collapse \
    build/neutrino_test_heavy_data \
    build/neutrino_test_01_buttons_icons \
    build/neutrino_test_02_text_inputs \
    build/neutrino_test_03_lists_combo_autoscroll \
    build/neutrino_test_04_listview_typed_data \
    build/neutrino_test_05_treeview_collapse \
    build/neutrino_test_06_placement_layout_scaling \
    build/neutrino_test_07_popup_menu_categories \
    build/neutrino_test_08_richtext_code \
    build/neutrino_test_09_readonly_richtext_icons \
    build/neutrino_test_10_images_progress_labels \
    build/neutrino_test_11_rendering_buffering \
    build/neutrino_test_12_scroll_windows_heavy_data \
    build/neutrino_test_13_swing_swt_controls \
    build/neutrino_test_14_label_offsets_themes \
    build/neutrino_test_15_selection_controls \
    build/neutrino_test_16_value_controls \
    build/neutrino_test_17_tabs_toolbar_splitter \
    build/neutrino_test_18_stage2_scroll_controls \
    build/neutrino_test_19_material_beauty

all: $(LIB) $(EXAMPLES)

build:
	mkdir -p build

build/%.o: src/%.cpp include/Neutrino/Neutrino.hpp | build
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(XFT_CPPFLAGS) -c $< -o $@

$(LIB): $(OBJ)
	$(AR) rcs $@ $^

build/%: examples/%.cpp $(LIB)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(XFT_CPPFLAGS) $< $(LIB) $(LDFLAGS) -o $@

build/neutrino_demo: examples/demo.cpp $(LIB)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(XFT_CPPFLAGS) $< $(LIB) $(LDFLAGS) -o $@

build/neutrino_test_all_controls: examples/test_all_controls.cpp $(LIB)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(XFT_CPPFLAGS) $< $(LIB) $(LDFLAGS) -o $@

build/neutrino_test_windows: examples/test_windows.cpp $(LIB)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(XFT_CPPFLAGS) $< $(LIB) $(LDFLAGS) -o $@

build/neutrino_test_typed_views: examples/test_typed_views.cpp $(LIB)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(XFT_CPPFLAGS) $< $(LIB) $(LDFLAGS) -o $@

build/neutrino_test_smooth_graphics: examples/test_smooth_graphics.cpp $(LIB)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(XFT_CPPFLAGS) $< $(LIB) $(LDFLAGS) -o $@

build/neutrino_test_treeview_collapse: examples/test_treeview_collapse.cpp $(LIB)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(XFT_CPPFLAGS) $< $(LIB) $(LDFLAGS) -o $@

build/neutrino_test_heavy_data: examples/test_heavy_data.cpp $(LIB)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(XFT_CPPFLAGS) $< $(LIB) $(LDFLAGS) -o $@

clean:
	rm -rf build

.PHONY: all clean
