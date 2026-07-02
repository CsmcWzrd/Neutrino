AC_INIT([Neutrino], [0.1.0], [support@example.invalid])
AC_CONFIG_SRCDIR([src/Neu_Application.cpp])
AC_CONFIG_HEADERS([config.h])
AC_PROG_CXX
AC_CHECK_TOOL([AR], [ar])
AC_PROG_RANLIB
AC_LANG([C++])
AC_MSG_CHECKING([whether the C++ compiler supports C++17])
save_CXXFLAGS="$CXXFLAGS"
CXXFLAGS="$CXXFLAGS -std=c++17"
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <optional>]], [[std::optional<int> v = 1; return *v;]])],
  [AC_MSG_RESULT([yes])],
  [AC_MSG_FAILURE([a C++17 capable compiler is required])])
CXXFLAGS="$save_CXXFLAGS"
AC_CHECK_HEADERS([X11/Xlib.h dlfcn.h], [], [AC_MSG_FAILURE([X11/Xlib.h and dlfcn.h are required])])
AC_CHECK_LIB([X11], [XOpenDisplay], [X11_LIBS="-lX11"], [AC_MSG_FAILURE([libX11 is required])])
AC_CHECK_LIB([dl], [dlopen], [DL_LIBS="-ldl"], [DL_LIBS=""])
AC_MSG_NOTICE([Xft anti-aliased font support is enabled automatically by Makefile.autoconf when pkg-config xft is available.])
AC_SUBST([X11_LIBS])
AC_SUBST([DL_LIBS])
AC_CONFIG_FILES([Makefile.autoconf])
AC_OUTPUT
