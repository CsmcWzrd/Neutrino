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
EXAMPLES = build/neutrino_demo build/neutrino_test_all_controls build/neutrino_test_windows build/neutrino_test_typed_views build/neutrino_test_smooth_graphics build/neutrino_test_treeview_collapse build/neutrino_test_heavy_data

all: $(LIB) $(EXAMPLES)

build:
	mkdir -p build

build/%.o: src/%.cpp include/Neutrino/Neutrino.hpp | build
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(XFT_CPPFLAGS) -c $< -o $@

$(LIB): $(OBJ)
	$(AR) rcs $@ $^

build/neutrino_demo: examples/demo.cpp $(LIB)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(XFT_CPPFLAGS) $< $(LIB) $(LDFLAGS) -o $@

build/neutrino_test_all_controls: examples/test_all_controls.cpp $(LIB)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(XFT_CPPFLAGS) $< $(LIB) $(LDFLAGS) -o $@

build/neutrino_test_windows: examples/test_windows.cpp $(LIB)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(XFT_CPPFLAGS) $< $(LIB) $(LDFLAGS) -o $@

build/neutrino_test_typed_views: examples/test_typed_views.cpp $(LIB)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(XFT_CPPFLAGS) $< $(LIB) $(LDFLAGS) -o $@

clean:
	rm -rf build

.PHONY: all clean

build/neutrino_test_smooth_graphics: examples/test_smooth_graphics.cpp $(LIB)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(XFT_CPPFLAGS) $< $(LIB) $(LDFLAGS) -o $@

build/neutrino_test_treeview_collapse: examples/test_treeview_collapse.cpp $(LIB)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(XFT_CPPFLAGS) $< $(LIB) $(LDFLAGS) -o $@


build/neutrino_test_heavy_data: examples/test_heavy_data.cpp $(LIB)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(XFT_CPPFLAGS) $< $(LIB) $(LDFLAGS) -o $@
