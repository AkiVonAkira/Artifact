# Game Engine Architecture

## Overview
The Artifact Engine is a component-based game engine written in C++ using OpenGL for rendering. It utilizes a modern Entity-Component-System (ECS) architecture powered by `entt`.

## Core Systems

### Application Layer
- **Application**: The main entry point and run loop manager. Handles window creation, input polling, and system updates.
- **InputManager**: Abstraction over GLFW input events.
- **TimeManager**: Handles delta time calculation and frame limiting.

### Rendering System
- **RenderManager**: Responsible for drawing the scene. It iterates over all entities with `MeshComponent` and `MaterialComponent`.
- **OpenGL**: Used as the backend graphics API.
- **Shaders**: Custom shader support (Vertex/Fragment).
- **CameraSystem**: Manages the active camera and view/projection matrices.

### Scene Management
- **SceneManager**: Manages the active scene and transitions.
- **ECS (Entity Component System)**:
  - **Entities**: ID-based game objects.
  - **Components**: Data containers (Transform, Mesh, Material, Camera, etc.).
  - **Systems**: Logic that operates on entities with specific components.

### Asset Management
- **AssetManager**: Central hub for loading and retrieving assets.
- **MeshManager**: Loads and caches OBJ files.
- **TextureLoader**: Loads images using `stb_image`.

### UI System
- **ImGui**: Used for the editor interface and debug overlays.
- **InspectorPanel**: Allows real-time editing of entity components.

### Messaging System
- **MessageQueue**: Asynchronous communication bus between systems.
- **Messages**: Strongly-typed events (e.g., `EntityCreatedMessage`, `AssetLoadedMessage`).

### Memory Management
- **MemoryChecker**: Tracks memory allocations to detect leaks and monitor usage.

## Data Flow
1.  **Initialization**: Systems are initialized in `Application`.
2.  **Game Loop**:
    -   **Poll Input**: Update input state.
    -   **Update**: Update scripts, physics, and scene logic.
    -   **Render**: Clear screen, render scene from active camera, render UI.
    -   **Swap Buffers**: Display the frame.
3.  **Shutdown**: Cleanup resources.

