# Advanced Graphics Programming

Albert Garcia Belerda
Marc Rosell Hernandez


## Techniques implemented:
* Environment mapping (skybox, fake reflections, and diffuse IBL)
![Environment mapping](https://github.com/MarcRosellH/Advanced_Graphics_Programming/blob/main/screenshots/Advanced_Graphics_Programming_08_06_2022_21_51_04.png)
Bulit in skybox, which uses cubic projection.
The objects use fake reflections to reflect the skybox and diffuse reflection to 
simulate environment lightning via image based lightning.

* Water effect
![Environment mapping](https://github.com/MarcRosellH/Advanced_Graphics_Programming/blob/main/screenshots/Advanced_Graphics_Programming_08_06_2022_21_44_19.png)
The water effect has been implemented using a clipping plane. Some refractions do not work as expected, yet reflactions do work.

## Usage:
The best way to see how the techniques work, the render method can be changed between forward and deferred. This might show a glance of the difference between applying or not the techniques. (Just a glance, because Environment Mapping has been also implemented in forward rendering).

As well, the user is able to change in scene positions, scales and rotations of entities, as for lights. Also the reflection of entities and other light properties.
<br>
![Buttons](https://github.com/MarcRosellH/Advanced_Graphics_Programming/blob/main/screenshots/unknown2.png)
![Buttons 2](https://github.com/MarcRosellH/Advanced_Graphics_Programming/blob/main/screenshots/unknown.png)

## Shaders:
* ConvolutionShader.glsl: Used to convolute the skybox and have a lower quality skybox as an irradiance map.
* Skybox.glsl: Used to render the skybox on screen
* shaders.glsl: Has every other shader, seperated using shader names, so it contains the basic forward and deferred rendering shaders, as well as the clipping plane shader and the water effect shader


[Presentation Link](https://docs.google.com/presentation/d/1o06jpdYUI6HVedFqyzqb0EbrdpFqXT-Ttzwwgacdbjw/edit#slide=id.ge49cd861c2_0_11)
