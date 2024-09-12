# Little Vulkan App

It's like writing an OS

## Build

### Windows

nobody uses windows anyways

### Linux

Grab [Vulkan SDK](https://vulkan.lunarg.com/doc/view/latest/linux/getting_started_ubuntu.html)
```
git submodule update --init --recursive 
mkdir build
cd build
cmake -G "Ninja" ../
ninja
```

### Apple

Grab [Vulkan SDK](https://vulkan.lunarg.com/doc/sdk/1.3.290.0/mac/getting_started.html)

Specify path to the library, as well as include folder, in `CMakeLists.txt`

Note that Apple build uses MoltenVK to link to metal. 
Extensions such as `VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME` must be enabled to setup
the translation layer.

Some extensions may not work.

## TODOs

- [x] graphics pipeline abstractions
- [x] basic game entity abstractions
- [x] object viewer window
    - [x] a fancy profiler UI
- [x] global instancing -- instance everything
    - [ ] instance clustered frustum culling
- [ ] indirect rendering
    - [ ] a performant object instance GC?


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
};
```
```
|Instance 1|Instance 2|Instance 3| ....
```

4. For each mesh, create a `vk::DrawIndexedIndirectCommand` that contains:
- number of instances to draw
- number of indices to use, starting from the base index
- base index to start, when reading the `index buffer array`
    - for example, for `Mesh 2` above, set the offset to 3
- offset to the base vertex, when reading the `vertex buffer array`
    - for example, for `Mesh 2` above, set the offset to 4.

```
|DrawCMD1 |DrawCMD2 |DrawCMD3 |DrawCMD4 |DrawCMDN
```
Note that all the above arrays are store on the GPU

At render time, the vulkan API provides the `vkCmdDrawIndexedIndirect` call,
that iterates over all the draw commands, in parallel, and executes the draws.

Note that the tight layout of mesh instances in the SSBO also provides opportunities
for compute shader to modify instance data -- such as transforms, in parallel.

In the shaders, `gl_InstanceIndex` can be used to access into the correct instance 
lookup array, and therefore instance data array.

#### Handling Complexities -- Perf Analysis

The above implementation may work well when we have a 
__fixed number of instances__. But let's also shed light
on the following complex runtime cases:

##### Assumptions

1. we assume the GPU is capable of having large enough VRAM to
store all vertex & index buffers of a scene. The cost is estimated to be small;
estimating a rough 1,000 different meshes, with 3,000 vertex + indices in each,
leads to 3,000,000 vertices. Giving the conservative assumption that, each vertex/index consists of 5 `vec3` -- 
60 bytes each, storing all vertices/indices on GPU would cost `60 * 3,000,000 / 1e-6` = `180 mb` of
vram -- a relatively small tax on the GPU, if we put electron apps into context.


##### Runtime addition/deletion of mesh instances

Instanced&indirect rendering adds complexity to choosing which instance to render.
This is important for unloading mesh instances from the scene, and more importantly,
instance culling.

A traditional & native rendering pipeline would do the following:

```cpp
extern std::vector<MeshInstance*> _instances;
for (MeshInstance* instance: _instances) {
    setUniform("model", mesh->modelMat);
    bindVertexBuffer(instance->vertexBuffer);
    bindIndexBuffer(instance->indexBuffer);
    bindTexture("albedo", instance->albedoTexture);
    //...
    drawCall();
}
```
Removing an instance is as simple as popping it from the vector on the CPU side.

However, when doing indirect rendering, we only have control over the number of 
instances to render, as well as write access to the instance data array.

Given that we wish to indirect & instance render everything, the following each presents
a solution and tradeoffs:

1. "Replace and decrement"
This is a rather simple solution. As we keep track of the total number of instances to render,
when removing an arbitrary instance, we:  
    a. copy over the last instance's data to the removed instance.  
    b. decrement the number of instances in the instance array by one.  

Before:  
```
|Instance 1|Instance 2|Instance 3| ....
```
After(remove 1):  
```
|Instance 3|Instance 2| ....
```
The main overhead of the solution comes from copying over instance data -- 
which would become a problem when done in quick succession: for example, when the frustum-culled
camera rapidly turns around, many instance data end up getting copied over and over.
The CPU also has to re-flush the instance data back to the buffer, after their removal.

However, for simple removal of mesh instance(implying the instance is never to be rendered
again), the method works well.

2. "Replace and decrement" with an additional layer of indirection  
We create an additional "Instance Index" array, and similar to the "Replace and decrement" method -- we
perform such operation on the array. The index array points to the instance datas, whereas the
instance data array remains unchanged.

The overhead of this method has been significantly reduced -- as one only needs to shuffle around 
the index array of 4 byte entries.

The instance lookup array scheme almost sound like vm page table; maybe we can take a page(aha) from
it?

To summarize, an object instance can have three states:
1. it wants to be rendered
2. it does not want to be rendered(culled)
3. it wants to be deleted

the "Replace and decrement" method handles case 2 by doing a 8-byte memory write per instance:
4 byte to copy over the instance data at the end of the instance lookup array to the "empty" slot,
and 4 byte to update the corresponding draw command's instance count.

as for case 3, in addition to the "replace and decrement", we also flag the instance's data in the
instance data array as free(currently using a free list). So that new instance data creation can
simply use the slot.

For instance addition, one can simply look at the instance data array and either append or insert to
the free list, then create a corresponding entry in the instance lookup array.


#### GPU-based Culling

__Occulusion culling__ and __frustum culling__ can be parallelized on the GPU as well. To do so, we need 
an additional array that points to all instances to be culled. 

We also forgo the previous "replace and decrement" design; observing that __most__ of the mesh instances
of a 3d scene would be culled away by frustum culling. We opt the opposite: "add and increment":

CPP pseudocode:

```cpp
extern vector<MeshInstance> instances; // actual array of all instances
extern vector<DrawCmd> drawCmds; // actual array of all drawcmds
extern vector<int> instanceIndices;

struct DrawCmd
{
    int firstInstance;
    int instanceNumber;
    //...
};

struct IndexS
{
    int dataIndex;
    int drawCmdIndex;
};
extern vector<IndexS> activeInstances;

extern bool IsVisible(MeshInstance& instance); // culling function

for (const IndexS i: activeInstances) {
    int dataIndex = i.dataIndex;
    int drawCmdIndex = i.drawCmdIndex;
    if (IsVisible(instances[dataIndex)) {
        // atomic add returns the number before add, which we use as index 
        int slot = atomicAdd(drawCmds[drawCmdIndex].instanceNumber, 1);
        int slotOffset = drawCmds[drawCmdIndex].firstInstance + slot; // offset in the global instance array
        instanceIndices[slotOffset] = dataIndex; // so that drawCmd would reach the data index
    }
}
```

The `for` loop above can be parallelized using compute shaders.

To simplify the number of data structures, we may choose to integrate `IndexS` into `MeshInstance` data structure.
Each `MeshInstance` therefore contains additional metadata other than that required for rendering.

Note this design invalidates the "free list" mesh instance deletion method. Namely we need to
perform "copy and decrement" on mesh instance deletion.


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
the memory operations the program was doing was smearing over the static section.
