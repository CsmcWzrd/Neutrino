CXX ?= g++
AR ?= ar
CXXFLAGS ?= -std=c++17 -Wall -Wextra -O2
XFT_CFLAGS := $(shell pkg-config --cflags xft 2>/dev/null)
XFT_LIBS := $(shell pkg-config --libs xft 2>/dev/null)
XFT_CPPFLAGS := $(if $(XFT_LIBS),-DNEUTRINO_USE_XFT $(XFT_CFLAGS),)
CPPFLAGS ?= -Iinclude
LDFLAGS ?= -lX11 -ldl $(XFT_LIBS)
SRC = src/Neu_Application.cpp \
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
EXAMPLE_SOURCES = examples/demo.cpp examples/neutrino_test_01_buttons_icons.cpp examples/neutrino_test_02_text_inputs.cpp examples/neutrino_test_03_lists_combo_autoscroll.cpp examples/neutrino_test_04_listview_typed_data.cpp examples/neutrino_test_05_treeview_collapse.cpp examples/neutrino_test_06_placement_layout_scaling.cpp examples/neutrino_test_07_popup_menu_categories.cpp examples/neutrino_test_08_richtext_code.cpp examples/neutrino_test_09_readonly_richtext_icons.cpp examples/neutrino_test_10_images_progress_labels.cpp examples/neutrino_test_11_rendering_buffering.cpp examples/neutrino_test_12_scroll_windows_heavy_data.cpp examples/test_all_controls.cpp examples/test_heavy_data.cpp examples/test_progress_square.cpp examples/test_readonly_richtext_spacing.cpp examples/test_richtext_formatting.cpp examples/test_scroll_clip_select.cpp examples/test_smooth_graphics.cpp examples/test_treeview_collapse.cpp examples/test_typed_views.cpp examples/test_windows.cpp examples/test_wordwrap_hints.cpp
EXAMPLES = $(patsubst examples/%.cpp,build/%,$(EXAMPLE_SOURCES))

all: $(LIB) $(EXAMPLES)

build:
	mkdir -p build

build/%.o: src/%.cpp include/Neutrino/Neutrino.hpp | build
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(XFT_CPPFLAGS) -c $< -o $@

$(LIB): $(OBJ)
	$(AR) rcs $@ $^

build/%: examples/%.cpp $(LIB)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(XFT_CPPFLAGS) $< $(LIB) $(LDFLAGS) -o $@

clean:
	rm -rf build

.PHONY: all clean
