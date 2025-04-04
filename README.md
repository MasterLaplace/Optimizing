# Optimizing

Implementation of World Partition System using Octree in CPP with SFML for rendering.

![Demo](./img/World_Partition-Octree_(Ubuntu).gif)

# Building

## Prerequisites
- SFML (for the rendering and the controls)
- glm (for the raytracing experimental feature)

## Build

compile the code using the following command:
```bash
g++ -std=c++20 -o optimizing main.cpp -I/usr/include -lsfml-graphics -lsfml-window -lsfml-system
```

or use the provided xmakefile:
```bash
xmake build -y
```

## Run

After building, you can run the program using:
```bash
./optimizing
```

or
```bash
xmake run
```

# Controls
- `Arrow Keys` - Move the camera
- `Escape` - Exit the program
