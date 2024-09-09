# Little Vulkan App

It's like writing an OS

## Stuff Done

- [x] graphics pipeline abstractions
- [x] basic game entity abstractions
- [x] object viewer window
    - [x] a fancy profiler UI
- [x] global instancing -- instance everything
    - [ ] instance clustered frustum culling


## TODO
- [ ] deferred shading

# Results

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
