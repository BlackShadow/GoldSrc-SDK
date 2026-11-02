3ds Max 4.2 SDK dependencies for utils/smdlexp

SDK headers and libraries:
https://archive.org/details/maxsdk-4.2.0.85
https://archive.org/download/maxsdk-4.2.0.85/maxsdk.zip
Archive SHA-1: cbb302c662411a2ef27d0eabbe40fe380eedc748
Only maxsdk/Include and maxsdk/Lib were extracted.

Character Studio 3.2 SDK header:
https://archive.org/details/3ds-max-4.2original
PHYEXP.H was extracted from 3dsmaxSetup9.cab in the installation media,
using the installer's File table to identify the original file.
SHA-256: c65ff50c04fda13120b41f87d71256b8e2b312e8e129d35e81fa4a5d9d67c116

Compatibility edits for the current compiler:
- max.h: omit the obsolete ctl3d.h include, as Valve's build notes require.
- maxtypes.h: name the BMM_Color_fl struct; fields and methods are unchanged.
- polyobj.h: declare the second loop's index in its own scope.
- imtl.h: keep the loop index in scope for the following debug assertion.
- istdplug.h: remove the empty fourth argument from a three-argument macro.

Valve's dependency notes:
https://github.com/ValveSoftware/halflife/blob/master/projects/vs2010/readme.txt

MaxSdkDir and CharacterStudioSdkDir in smdlexp.vcxproj can be overridden
to use another installation of these same SDK versions.
