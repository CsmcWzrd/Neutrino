# Neutrino verification notes

Verified in this Linux container:

```sh
make -j2
cmake -S . -B cmake-build
cmake --build cmake-build -j2
./configure
make -f Makefile.autoconf -j2
```

The Visual Studio 2022 solution cannot be compiled in this Linux container because MSVC and the Windows SDK are unavailable here. The package does include `msvc/Neutrino.sln`, a static library project, the native Win32 demo, and projects for every focused test application. All MSVC projects are configured for standard C++17 and define `_CRT_SECURE_NO_WARNINGS` for Windows compilation.

This package is proprietary and intentionally does not include any open-source license grant.
