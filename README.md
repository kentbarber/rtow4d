# Ray Tracing In One Weekend for Cinema 4D

This is an integration of Peter Shirleys Ray Tracing In One Weekend code base into Cinema 4D - https://raytracing.github.io/

The original code can be found in the 'original' directory. The very slightly modified verison can be found in the 'rtow' directory. The only changes were to remove compiler warnings and errors due to the integration into C4D.

This has not be optimized in anyway. 

It was intended to just use the source code directly, as-is, in C4D and integrate it so that it can be used to render out images and also have an interactive viewport for realtime editing of objects and material parameters. It is more of an experiment to see how to create a basic renderer with realtime feedback via multi-threaded progressive rendering. 

The codebase can also show how to use basic jobs and multi-threading, VideoPost plugin for adding a renderer to C4D and how to add and use a custom material.

Note that this repository is called "rtow4d" but when compiled it will appear as "FunRay" throughout the C4D interface.

Be warned that right now it is a nasty mix of Cinema 4D memory allocation and also the standard lib (std::make_shared). So you will most likely get a crash warnings if you close it down while it is rendering to the viewport or while the FunRay RenderView dialog is open and rendering. If you really wanted to fix this you would re-write the entire RTOW codebase using C4D datastructures and memory allocat.

You can find more information about this project on the following website: https://plugins4d.com/Product/FunRay

On that website you can also download a compiled version of the plugin for R25 and S26.

If you want to learn how to compile plugins for Cinema 4D you could start by watching the first two tutorials here on youtube: https://www.youtube.com/playlist?list=PLEPTxkpDVvX0r292yn8xL39Cm3Wi3E69i

Maxon Development Support: https://plugincafe.maxon.net/
Maxon Development Website: https://developers.maxon.net/
C4D C++ Development Help: https://developers.maxon.net/docs/Cinema4DCPPSDK/html/index.html
