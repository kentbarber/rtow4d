# Ray Tracing In One Weekend for Cinema 4D

This is an integration of Peter Shirleys Ray Tracing In One Weekend code base into Cinema 4D - https://raytracing.github.io/

Download a version for R25 and S26 as well as read more information on what this plugin does here: https://plugins4d.com/Product/FunRay

Note that this repository is called "rtow4d" but when compiled it will appear as "FunRay" throughout the C4D interface.

The original code can be found in the 'original' directory. The very slightly modified verison can be found in the 'rtow' directory. The only changes were to remove compiler warnings and errors due to the integration into C4D.

This has not been optimized at all.

It was intended to just use the source code directly, as-is, in C4D and integrate it so that it can be used to render out images and also have an interactive viewport for realtime editing of objects and material parameters. It is more of an experiment to see how to create a basic renderer with realtime feedback via multi-threaded progressive rendering. 

The codebase can also show how to use basic jobs and multi-threading, VideoPost plugin for adding a renderer to C4D and how to add and use a custom material.

Be warned that right now it is a nasty mix of Cinema 4D memory allocation and also the standard lib (std::make_shared). So you will most likely get a crash warnings if you shut down C4D while it is rendering. If you really wanted to fix this you would re-write the entire RTOW codebase using C4D datastructures and memory allocations.

## Currently supports the following
- Supports Spheres, Cubes and Cylinders that are individual objects in the scene.
- Custom Materials (Material Manager->Create->Extensions->FunRay Material)
  - Metal
  - Glass
  - Lambert
- Supported Camera Parameters
  - Object Tab
    - Focal Length
    - Sensor Size
    - Field of View (Horizontal)
    - Field of View (Vertical)
    - Focus Distance
    - Use Target Object
    - Focus Object
  - Physical Tab
    - F-Stop
- Interactive Renderer View (Extensions->FunRay->FunRay RenderView)
  - Interactive Preview
  - Supports moving and adjusting camera parameters
  - Supports editor camera and selected camera
  - Supports moving objects
  - Different Render modes
    - Multi-Threaded: When enabled uses all your cores. When disabled uses a single core.
    - Progressive: When enabled will continuously update. When disabled it will use the number of Samples as defined in the "RenderSettings->FunRay Renderer->Samples Per Pixel"
- Render Settings dropdown to select the FunRay Renderer
  - Samples Per Pixel: Number of samples per pixel when rendering to Picture Viewer or to File
  - Max Depth
  - Viewport Render Mode: Set what kind of rendering to do in the main C4D viewport, default is multi-threaded progressive

## C4D Integration

Realtime progressive updating in the RenderView.

![RTOW](https://plugins4d.com/img/funray/fr_movingcamera.gif)

This picture shows rendering to the viewport and the picture viewer. The spinning movie was renderered directly out of Cinema 4D as an mp4 file. It is not real time.

![RTOW Render](https://plugins4d.com/img/funray/fr_videopost.gif)

This 1280x720 image took 12 minutes to render. F-Stop of 0.8, 400 samples per pixel and Max Depth of 50. Rendered on a 10 core i7.

![RTOW Render2](https://plugins4d.com/img/funray/funray_400_samples_1280.jpg)


## Additional help
- Learn how to compile C++ plugins for Cinema 4D by watching the first two tutorials here: https://www.youtube.com/playlist?list=PLEPTxkpDVvX0r292yn8xL39Cm3Wi3E69i
- Maxon Development Support: https://plugincafe.maxon.net/
- Maxon Development Website: https://developers.maxon.net/
- C4D C++ Development Help: https://developers.maxon.net/docs/Cinema4DCPPSDK/html/index.html
