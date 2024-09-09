# Little Vulkan App

It's like writing an OS

## Stuff Done

- [x] graphics pipeline abstractions
- [x] basic game entity abstractions
- [x] object viewer window
    - [x] a fancy profiler UI
- [x] global instancing -- instance everything
    - [ ] instance clustered frustum culling
- [ ] indirect rendering


## TODO
- [ ] deferred shading

# Results

## Global Instanced&Indirect Rendering

Instanced rendering of 5000 cows. Every entity is backed by instanced rendering so the programmer
doens't need to worry about doing instancing themselves. Instance data such as transform & texture, live in GPU and the engine
provides nice abstractions to update them.

Creating 3d objects is as:

```cpp
extern InstancedRendererSystem* renderer;

for (int i = 0; i < INSTANCE_NUM; i++) {
    Entity* cow = new Entity("cow");
    InstanceRenderingComponent* instanceComponent = 
        renderer->MakeInstanceComponent("cow.obj", "cow.png");
    cow->AddComponent(instanceComponent);
    cow->AddComponent(new TransformComponent());

    // demo to update position
    auto position = RandomPosition()
    cow->GetComponent<TransformComponent>()->position = position;
    cow->GetComponent<InstanceRenderingComponent>()->FlagAsDirty(cow); 
    // this tells the renderer that we have modified some structure that affects instanced rendering. 
    // The renderer will take care of the rest                                                                                                                             
}
```

![cows](images/instance_everything.gif)

### Global Instanced&Indirect Rendering Overview

Global instanced&indirect rendering offloads what would be handled by the CPU --
the `bindVertexBuffer()`, `bindIndexBuffer()`, `setUniform()`, and `bindTexture()` calls,
to the GPU.

Instead of directly binding the buffers, the GPU uses a series of indices to index
into multiple global arrays to retrieve information relevant to the mesh.

#### Implementation

Mesh instances differ in 3 types of resources they traditionally need to
bind to / get from uniform:
- Vertex & Index buffer
- texture(normals, albedo, phong, pbr, etc...)
- instance-specific data -- data that is seldomly shared among multiple instances
    - e.g. model matrix, transparency

Each of the above types, can be generalized as a "data structure",
that we put into an array. The shaders/GPU APIs are then capable of indexing 
into them.

Specifically:

1. for each different vertex/index, reserve an array element on GPU to store them.

Giant vertex buffer that contains vertices of all meshes
```
Mesh 1(offset=0)                   Mesh 2(offset=4)           Mesh 3(offset=7)
|                                   |                          |
|                                   |                          |
|Vertex 1|Vertex 2|Vertex 3|Vertex 4|Vertex 5|Vertex 6|Vertex 7| ....|Vertex N
```
Giant index buffer that contains indices of all meshes
```
Mesh 1(offset=0)        Mesh 2(offset=3)                              Mesh 3(offset=9)
|                       |                                               |
|                       |                                               | 
|Index 1|Index 2|Index 3|Index 4|Index 5|Index 6|Index 7|Index 8|Index 9|.... | Index N
```

2. for each texture, reserve an array element for store.
```
|Texture 1|Texture 2|Texture 3| ....
```
3. for each instance-specific data of an instance, pack them into
a struct and store them a giant SSBO. Also pack in offset to textures:

```cpp
struct Instance
{
    mat4 model;
    float transparency;
    int albedoOffset;
    int normalOffset;
    int roughnessOffset;
    ...
}
```
```
|Instance 1|Instance 2|Instance 3| ....
```

4. For each mesh, create a `vk::DrawIndexedIndirectCommand` that contains:
- number of instances to draw
- number of indices to use, starting from the base index
- base index to start, when reading the `index buffer array`
- offset to the base vertex, when reading the `vertex buffer array`

```
|DrawCMD1 |DrawCMD2 |DrawCMD3 |DrawCMD4 ...
```
Note that all the above arrays are store on the GPU

## Fancy Profiler

A fancy profiler GUI to show performance metrics, inspired by my own work on an nvidia internal profiler

![profiler](images/profiler_lineplot.gif)

Profiling entries are declarative and bound to scope. 
This means one can easily profil a function by simply inserting a macro as follows:
```cpp
void UpdateEnginePhysics(Context* ctx) {
    PROFILE_SCOPE(ctx->profiler, "engine physics update");
    // .. bunch of transforms, culling, buffer shoving around etc
}
// profiler goes out of scope here, and the entry "engine physics update" will get recorded
```

# Rant

## Strange Memory Issue??????

just had the strangest memory bug i've ever experienced.

```cpp
const std::array<VkVertexInputBindingDescription, 2>* Vertex::
    GetBindingDescriptionsInstanced() {
    static std::array<VkVertexInputBindingDescription, 2> bindingDescriptionsInstanced;

    static bool initialized = false;
    if (!initialized) {
        // bind point 0: just use the non-instanced counterpart
        bindingDescriptionsInstanced[0] = GetBindingDescription();

        // bind point 1: instanced data
        bindingDescriptionsInstanced[1].binding = 1;
        bindingDescriptionsInstanced[1].stride = sizeof(VertexInstancedData);
        bindingDescriptionsInstanced[1].inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;

        initialized = true;
    }

    return std::addressof(bindingDescriptionsInstanced);
}
```

the above function returns an address to a stack-allocated static variable

All the values are correct initialized and written into. 
However as soon as the function returns, the variable gets re-initialized into its original values.

![wtf](wtf.png)

Same bug happened to my TextureManager singleton, where the stack-allocated singleton seems to have
gotten zero-initialized.

Update: turns out it's a memory corruption of me trying to write to another stack-allocated static
variable that overwrites into other parts of the code section. Segfault didn't happen because all
the memory operations the program was doing was smearing over the code section.
