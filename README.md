# RubikSim

RubikSim is an interactive 3D Rubik's Cube built with C++ and OpenGL. It supports mouse-driven turns, camera controls, scrambles, timed solves, and animated resets.

## Demo

[Watch the demo](demo/rubik_demo.mov)

## Features

- Turn cube layers with the mouse
- Orbit and zoom the camera
- Generate a 20-move scramble
- Track solve time and move history
- Undo all moves with an animated reset
- Detect when the cube returns to its solved state

## Build

You need CMake 3.20 or newer, a C++20 compiler, and Git. CMake downloads GLFW, GLM, and Dear ImGui during setup.

```sh
cmake -S . -B build
cmake --build build
./build/rubiksim
```

## Controls

| Input | Action |
| --- | --- |
| Drag a cubie | Turn a layer |
| Drag empty space | Orbit the camera |
| `W` / `S` | Zoom in or out |
| `X` | Scramble the cube |
| `V` | Undo all recorded moves |
| `Return` | Start or stop the timer |
| `Backspace` | Reset the cube |
| `Escape` | Close the app |

## Stack

- C++20
- OpenGL 3.3
- GLFW
- GLM
- Dear ImGui

The project is an early step toward a tool for synthetic computer vision data. Dataset export and support for other cube sizes are not implemented yet.
