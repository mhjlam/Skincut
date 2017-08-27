Real-time Simulation and Visualization of Cutting Wounds
----------

C++ source code for master thesis **Real-time Simulation and Visualization of Cutting Wounds**.

**Author:** Maurits Lam  
**License:** this source code is released under the [CC BY-SA 4.0](https://creativecommons.org/licenses/by-sa/4.0) license.

### Minimum system requirements
* Windows 8 or later (x86/x64).
* 1 GHz processor (minimum for Windows 8).
* 1 GB memory (minimum for Windows 8).
* DirectX 11.0 compatible video card.

Possibly works under Windows 7 too, but this has not been tested.

### Build requirements
* Visual Studio 2015.
* Windows SDK.

Simply open the provided Visual Studio solution and build the solution. Required libraries are included.

### Libraries used
* [DirectX Tool Kit](http://directxtk.codeplex.com/) (January 5, 2016)
* [DirectXTex texture processing library](http://directxtex.codeplex.com/) (November 30, 2015)
* [ImGui](https://github.com/ocornut/imgui) (March 26, 2015; v1.37)
* [SimpleJSON](https://github.com/MJPA/SimpleJSON) (May 26, 2012)


### Resource files
The application requires several resources to be present. By default it looks for a directory relative to the executable at `..\..\resources\`. This directory and all subdirectories must remain intact. An alternate (relative or absolute) path to the `resources` directory can also be supplied as an argument to the program executable. Files `resources\config.json` and `resources\scene.json` can be modified to make changes to the configuration and scene definition, respectively.

### User manual
Hold **shift key** and click **left mouse button** to select the start of a new cut. Repeat to select the end of the cut. Note that a cut must span at least two mesh faces. Pressing **P** cycles through the available pick modes (draw wound texture only, draw wound texture and merge cutting line, or draw wound texture and carve incision).

The camera can be operated with the mouse. Hold **left mouse button** to rotate around the model. **right mouse button** zooms in and out. **Middle mouse button** pans the camera.

Rendering options can be modified by using the panels on the left side of the screen. Change the renderer with the drop down panel. For the primary shader, *Kelemen/Szirmay-Kalos*, several rendering features can be toggled on or off, its shading can be fine-tuned, and the intensity of the scene lights can be modified. The user interface itself can be toggled on and off by pressing **F1**.

Hold **control** and click **left mouse button** to subdivide the face under the mouse cursor with the selected subdivision mode (3-split, 4-split, or 6-split). The current mode is shown in the bottom-right and can be cycled through with the **S** key.

Press **R** to reload the scene at any time. In case of a critical runtime error, the application will ask to reload the scene as well. Note that this process may take a few seconds during which the application becomes unresponsive.

The performance test described in the thesis can be executed by pressing **T**. Running times (in milliseconds) are written to the console window. Note that this process takes several minutes during which the application becomes unresponsive.