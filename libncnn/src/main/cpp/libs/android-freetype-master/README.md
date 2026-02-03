# FreeType library for Android for Castle Game Engine

This is a fork of https://github.com/cdave1/freetype2-android with small modifications to:

- Build for all platforms relevant for _Castle Game Engine_:
  - 32-bit ARM (android-16 platform).
  - 64-bit ARM (aka Aarch64) (android-21, minimal platform possible).
  - 32-bit X86
  - 64-bit X86 (x86_64) (both x86 and x86_64 are mostly useful in emulators/virtual machines).

- Easily copy the output to _Castle Game Engine_ tree.

Just use `make build`. This calls `ndk-build` to compile the libraries and copies them to _Castle Game Engine_ tree (assumed in `$CASTLE_ENGINE_PATH`).

# Credits

See README-original.md for license and original (from https://github.com/cdave1/freetype2-android ) comments. Most of them still apply -- the changes here are minimal.
