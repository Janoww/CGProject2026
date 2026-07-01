# How Things Get Displayed — Medieval Tavern Framework Walkthrough

This document explains, step by step, how a pixel ends up on screen in this project. It uses the actual framework this project is built on (`Starter.hpp`, which defines `BaseProject`) and traces everything through `App.cpp`, where the `MedievalTavern` app lives.

The idea to keep in mind throughout: **the framework owns the render loop; your app only fills in the blanks.** `BaseProject` does all the raw Vulkan bookkeeping (instance, device, swapchain, sync, command submission). Your class `MedievalTavern` overrides a handful of virtual methods that the framework calls at the right moments. `App.cpp` *is* those blanks.

---

## 0. The cast of objects

Before the flow, here are the framework wrapper types you use in `App.cpp` (declared in `App.hpp`), each of which hides a cluster of raw Vulkan handles:

| Wrapper (in `App.hpp`) | Wraps (raw Vulkan) | Role |
|---|---|---|
| `VertexDescriptor VD`, `VDoverlay` | `VkVertexInputBindingDescription` + attributes | Describes how vertex bytes are laid out |
| `DescriptorSetLayout DSLglobal/DSLlocal/DSLoverlay` | `VkDescriptorSetLayout` | The *shape* of data a shader expects (which bindings, which stages) |
| `RenderPass RP` | `VkRenderPass` + framebuffers + attachments | The target you draw into |
| `Pipeline P`, `Poverlay` | `VkPipeline` + `VkPipelineLayout` + shaders | A shader program + all fixed-function state |
| `DescriptorSet DSglobal`, `DSsplash` | `VkDescriptorSet` + uniform buffers | The *actual data* bound for a draw |
| `Model Msplash` | `VkBuffer` (vertex + index) | Geometry on the GPU |
| `Texture Tsplash` | `VkImage` + view + sampler | An image the shader can sample |
| `Scene SC` | many of the above | Loads models/textures/instances from `scene.json` |
| `TextMaker txt` | its own pipeline + meshes | On-screen text (the FPS counter) |

Distinction worth burning in early: a **DescriptorSetLayout** is the *schema* ("set 1 has a uniform buffer at binding 0 and a texture at binding 1"), while a **DescriptorSet** is a filled-in *instance* of that schema pointing at real buffers and images. Layouts are created once; sets are created per swapchain image and updated every frame.

---

## 1. Program entry — `main.cpp`

```cpp
int main() {
    MedievalTavern app;
    app.run(false);   // false = no raytracing
}
```

`run()` lives in `BaseProject` (`Starter.hpp`). It is the whole program:

```cpp
void BaseProject::run(bool _includeRaytracing) {
    setWindowParameters();   // <-- your override
    initWindow();            // GLFW window
    initVulkan();            // build everything
    mainLoop();              // draw forever
    cleanup();               // tear down
}
```

Three of those five call into your code. `setWindowParameters()` is the first: in `App.cpp` it just sets width/height/title and the aspect ratio `Ar`. Nothing is drawn yet.

---

## 2. One-time setup — `initVulkan()`

`initVulkan()` runs a fixed sequence. The framework parts are greyed out below; the **bold** ones are where `App.cpp` gets called.

```cpp
void BaseProject::initVulkan() {
    createInstance();        // framework: connect to the Vulkan loader
    setupDebugMessenger();   // framework: validation layers
    createSurface();         // framework: GLFW ↔ Vulkan window surface
    pickPhysicalDevice();    // framework: choose a GPU
    createLogicalDevice();   // framework: open the GPU + queues
    createSwapChain();       // framework: the images we present to the screen
    createImageViews();      // framework
    createCommandPool();     // framework: allocator for command buffers

    localInit();                       // *** YOUR CODE ***
    createDescriptorPool();            // framework: uses sizes you set in localInit
    pipelinesAndDescriptorSetsInit();  // *** YOUR CODE ***

    createSyncObjects();     // framework: fences + semaphores
}
```

Two things to notice about ordering. First, `localInit()` runs **before** `createDescriptorPool()` — that is why, inside `localInit`, you must set the pool sizes (`DPSZs`) *before* loading the scene; the framework reads those sizes immediately afterward. Second, pipelines and descriptor **sets** are created in a separate step (`pipelinesAndDescriptorSetsInit`) because they depend on the swapchain and therefore must be re-created on every window resize (see §6).

### 2.1 `localInit()` — declare what exists

This is the biggest method in `App.cpp`. It declares every resource, in dependency order:

```cpp
void MedievalTavern::localInit() {
    DSLInit();                 // 1. descriptor set LAYOUTS (schemas)
    VDInit();                  // 2. vertex layouts
    RP.init(this);             // 3. the render pass (draw target)
    RP.properties[0].clearValue = {0.0f, 0.9f, 1.0f, 1.0f};  // sky-blue background
    PipelineInit();            // 4. pipelines (shader + state)

    DPSZs.uniformBlocksInPool = 3;   // 5. how big the descriptor pool must be
    DPSZs.texturesInPool      = 2;
    DPSZs.setsInPool          = 3;

    // 6. scene: models, textures, instances from JSON
    VDRs.resize(1); VDRs[0].init("VDposUV", &VD);
    PRs.resize(1);  PRs[0].init("BlinnPos", { ... }, 1, &VD);
    SC.init(this, 1, VDRs, PRs, "assets/scenes/scene.json");

    // 7. the overlay (splash screen) resources
    Tsplash.init(this, "assets/textures/TavernLogo.png");
    OverlayMeshInit(windowWidth, windowHeight, -1.0f, -1.0f, &Msplash, 1);

    // 8. text output
    txt.init(this, windowWidth, windowHeight);

    // 9. record the main command buffer for the first time
    submitCommandBuffer("main", 0, populateCommandBufferAccess, this);
    txt.print(...);   // queue the "FPS:" label
}
```

Let's unpack the four sub-inits, because they define *everything the GPU will later use*.

**`DSLInit()` — the shader interface contracts.** Each `DescriptorSetLayout::init` lists the bindings a shader will read:

```cpp
DSLlocal.init(this, {
    {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         VK_SHADER_STAGE_VERTEX_BIT,   sizeof(UniformBufferObject), 1},
    {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0,                          1}
});
```

Read this as: "a per-object set has a uniform buffer at binding 0 (used by the vertex shader, holding a `UniformBufferObject` = the MVP matrices) and a texture at binding 1 (used by the fragment shader)." Compare it to `toChangeSimplePos.vert`, which declares `layout(binding = 0, set = 1) uniform UniformBufferObject {...}` — the layout and the shader must agree. `DSLglobal` holds the light/camera globals (set 0); `DSLoverlay` holds the splash's `visible` flag + its texture.

**`VDInit()` — how to read vertex bytes.** `VD.init` says: one buffer, stride `sizeof(Vertex)`, with attribute 0 = a `vec3` position at `offsetof(Vertex,pos)` and attribute 1 = a `vec2` UV at `offsetof(Vertex,UV)`. `VDoverlay` does the same for the 2D `VertexOverlay` (position is a `vec2`, no depth). This is the metadata that the byte-packing in `OverlayMeshInit` (see §5) relies on.

**`RP.init(this)` — the render pass.** This creates the object that represents *drawing into the screen*: a color attachment (the swapchain image) plus a depth attachment. `RP.properties[0].clearValue` sets what the screen is wiped to at the start of each frame — here sky blue.

**`PipelineInit()` — the shader programs.** A `Pipeline` marries a `VertexDescriptor`, two compiled shaders (`.spv`), and the descriptor-set layouts it will use:

```cpp
P.init(this, &VD, "shaders/toChangeSimplePos.vert.spv",
                  "shaders/toChangeBlinnFromPos.frag.spv",
                  {&DSLglobal, &DSLlocal});   // set 0 = global, set 1 = local

Poverlay.init(this, &VDoverlay, "shaders/Overlay.vert.spv",
                                "shaders/Overlay.frag.spv",
                                {&DSLoverlay});
Poverlay.setCompareOp(VK_COMPARE_OP_ALWAYS);  // ignore depth: always draw on top
Poverlay.setCullMode(VK_CULL_MODE_NONE);      // a 2D quad has no back face
Poverlay.setTransparency(true);               // honor the texture's alpha
```

Note that `init` only *describes* the pipeline; the actual `VkPipeline` isn't built until `create()` in the next step, because it needs the render pass.

### 2.2 `pipelinesAndDescriptorSetsInit()` — build the GPU objects

Everything above only *described* resources. This method actually allocates the swapchain-dependent GPU objects:

```cpp
void MedievalTavern::pipelinesAndDescriptorSetsInit() {
    RP.create();                 // build the VkRenderPass + framebuffers
    P.create(&RP);               // compile the pipeline against this render pass
    Poverlay.create(&RP);

    DSglobal.init(this, &DSLglobal, {});                       // allocate the global set
    DSsplash.init(this, &DSLoverlay, {Tsplash.getViewAndSampler()});  // splash: UBO + texture

    SC.pipelinesAndDescriptorSetsInit();   // the scene builds its own sets
    txt.pipelinesAndDescriptorSetsInit();
}
```

`DescriptorSet::init` allocates one `VkDescriptorSet` **per swapchain image** from the pool, and creates the uniform buffers behind each of them. After this method, the GPU knows every pipeline and every descriptor set. We are ready to draw.

---

## 3. The building blocks, in one place

Before the frame loop, here is what each wrapper's key methods do under the hood (from `Starter.hpp`), because the loop is just calls to these:

- **`RenderPass::begin(cb, img)`** → records `vkCmdBeginRenderPass`, binding framebuffer `img` and the clear values. `end()` records `vkCmdEndRenderPass`. Everything you draw must sit between them.
- **`Pipeline::bind(cb)`** → `vkCmdBindPipeline`. Selects which shaders + state are active.
- **`Model::bind(cb)`** → `vkCmdBindVertexBuffers` + `vkCmdBindIndexBuffer`. Selects the geometry.
- **`DescriptorSet::bind(cb, pipeline, setId, img)`** → `vkCmdBindDescriptorSets`. Selects the data (uniforms/textures) for `set = setId`, using image `img`'s copy.
- **`DescriptorSet::map(img, &src, slot)`** → `vkMapMemory` + `memcpy` + `vkUnmapMemory`. Uploads CPU struct `src` into the uniform buffer for image `img`. This is how a matrix gets from C++ into the shader.
- **`vkCmdDrawIndexed(cb, indexCount, ...)`** → the actual draw call.

So one object drawn = bind pipeline, bind model, bind its descriptor sets, draw. That pattern repeats for everything on screen.

---

## 4. The per-frame loop — `mainLoop()` → `drawFrame()`

```cpp
void BaseProject::mainLoop() {
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        drawFrame();          // once per frame, ~60+ times a second
    }
    vkDeviceWaitIdle(device);
}
```

`drawFrame()` (framework) is the heartbeat. Its structure:

```cpp
void BaseProject::drawFrame() {
    vkWaitForFences(...);                          // 1. wait until this frame's slot is free
    vkAcquireNextImageKHR(..., &imageIndex);       // 2. get the next swapchain image
        // (if out of date → recreateSwapChain(); return)

    updateUniformBuffer(imageIndex);               // 3. *** YOUR CODE: update the data ***
    updateCommandBuffers(buffers, imageIndex);     // 4. (re)record command buffers if needed

    vkQueueSubmit(graphicsQueue, ...);             // 5. run the commands on the GPU
    vkQueuePresentKHR(presentQueue, ...);          // 6. show the finished image
        // (if resized → recreateSwapChain())
    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}
```

Only **step 3** is yours. Steps 1, 5 and 6 are the synchronization dance: a fence keeps the CPU from getting more than `MAX_FRAMES_IN_FLIGHT` ahead of the GPU, and two semaphores order "image acquired → rendering → present." You never touch them.

There is a subtlety worth knowing: the command buffer that lists *what* to draw is recorded **once** (back in `localInit` via `submitCommandBuffer("main", ...)`), not every frame. `updateCommandBuffers` only re-records it when it's been invalidated (e.g. after a resize, or when the text changes). What actually changes 60 times a second is not the draw commands but the **uniform data** those commands read — and that's exactly what `updateUniformBuffer` rewrites.

### 4.1 What gets *drawn* — `populateCommandBuffer()`

This is the callback recorded into the "main" command buffer. It defines the draw order:

```cpp
void MedievalTavern::populateCommandBuffer(VkCommandBuffer cb, int currentImage) {
    RP.begin(cb, currentImage);                    // start drawing into the screen (clears to sky blue)

    SC.populateCommandBuffer(cb, 0, currentImage); // 1. the whole 3D scene (tavern, characters…)

    Poverlay.bind(cb);                             // 2. the splash overlay, on top
    Msplash.bind(cb);
    DSsplash.bind(cb, Poverlay, 0, currentImage);
    vkCmdDrawIndexed(cb, Msplash.indices.size(), 1, 0, 0, 0);

    RP.end(cb);                                    // finish
}
```

The order is the paint order: the scene first, then the overlay quad on top of it (which is why the overlay pipeline uses `VK_COMPARE_OP_ALWAYS` and transparency — it must ignore depth and blend over whatever is already there). Note the splash is drawn **every frame unconditionally**; whether you actually *see* it is decided in the shader, not here (see §5).

### 4.2 What gets *updated* — `updateUniformBuffer()`

Called every frame with the current image index. This is where your app logic writes fresh data into the descriptor sets:

```cpp
void MedievalTavern::updateUniformBuffer(uint32_t currentImage) {
    // input + state machine (ESC to quit, ENTER to leave the menu) ...

    OverlayUniformBuffer subo{};
    subo.visible = (gameState == MENU_STATE) ? 1.0f : 0.0f;
    DSsplash.map(currentImage, &subo, 0);      // upload the show/hide flag

    float deltaT = GameLogic();                // recompute View / ViewPrj from input

    GlobalUniformBufferObject gubo{};
    gubo.lightDir = ...; gubo.lightColor = ...; gubo.eyePos = ...;
    DSglobal.map(currentImage, &gubo, 0);      // upload light + camera

    UniformBufferObject ubo{};
    for (each instance in SC.TI[0]) {
        ubo.mMat   = instance.Wm;              // model matrix
        ubo.mvpMat = ViewPrj * ubo.mMat;       // full transform
        instance.DS[0][0]->map(currentImage, &gubo, 0);  // set 0 = globals
        instance.DS[0][1]->map(currentImage, &ubo,  0);  // set 1 = this object's matrices
    }

    // FPS counter → txt.print(...) → txt.updateCommandBuffer();
}
```

Every `map()` call is a CPU→GPU upload into that image's uniform buffer. Because there is a separate buffer per swapchain image, the GPU can still be reading last frame's copy while you write this frame's — no stall, no tearing.

This method is where the two matrices in `toChangeSimplePos.vert` come from: `ubo.mvpMat` positions the vertex on screen (`gl_Position = ubo.mvpMat * vec4(inPosition,1.0)`), and `ubo.mMat` gives the fragment shader the world-space position it needs for Blinn lighting.

---

## 5. Following one object end-to-end: the splash quad

To tie it together, here is the full life of the tavern-logo splash, from CPU data to lit pixel.

**a. Geometry is built** in `OverlayMeshInit` (`App.cpp`). Four vertices form a rectangle in Normalized Screen Coords, with UVs 0→1 mapping the whole texture onto it, and indices `{0,1,2, 1,2,3}` splitting the quad into two triangles. Each vertex struct is copied **byte-for-byte** into the model's raw vertex buffer:

```cpp
const auto *bytes = reinterpret_cast<const unsigned char *>(&v);
model[i].vertices.insert(model[i].vertices.end(), bytes, bytes + sizeof(VertexOverlay));
...
model[i].initMesh(this, &VDoverlay);   // upload to a GPU VkBuffer, using the VDoverlay layout
```

`VDoverlay` (from `VDInit`) is what tells the GPU how to interpret those bytes: "vec2 position, then vec2 UV."

**b. The texture is loaded** — `Tsplash.init(this, "assets/textures/TavernLogo.png")` creates a `VkImage` + sampler.

**c. Data is wired** — `DSsplash.init(this, &DSLoverlay, {Tsplash.getViewAndSampler()})` allocates a descriptor set matching `DSLoverlay`: binding 0 = the `visible` uniform, binding 1 = the splash texture.

**d. Each frame, data is refreshed** — `updateUniformBuffer` sets `subo.visible` to 1 on the menu, 0 in-game, and `map`s it in.

**e. Each frame, it's drawn** — `populateCommandBuffer` binds `Poverlay`, binds `Msplash`, binds `DSsplash`, and `vkCmdDrawIndexed`.

**f. The GPU runs the shaders.** `Overlay.vert` passes the 2D position straight through (`gl_Position = vec4(pos,0,1)` — already in clip space, no camera). `Overlay.frag` reads the flag and either shows or hides the pixel:

```glsl
if (ubo.visible < 0.5) discard;   // in-game: every pixel thrown away → nothing appears
outColor = texture(tex, fragUV);  // on the menu: sample the logo
```

That `discard` is the trick mentioned in §4.1: the draw call always runs, but the fragment shader decides at pixel level whether the splash is visible. No branching on the CPU, no re-recording the command buffer to hide it.

**g. Present.** `drawFrame` submits the command buffer and `vkQueuePresentKHR` hands the finished image to the screen. Pixel delivered.

---

## 6. Resize — why setup is split in two

When the window changes size, GLFW fires `framebufferResizeCallback` → your `onWindowResize` (updates `Ar` and `RP.width/height`), and `drawFrame` detects the stale swapchain and calls `recreateSwapChain()`:

```cpp
void BaseProject::recreateSwapChain() {
    // wait idle, tear down the old swapchain ...
    createSwapChain();
    // ...
    pipelinesAndDescriptorSetsInit();   // rebuild pipelines + descriptor sets
}
```

This is the reason §2 separated "declare" (`localInit`, run once) from "build against the swapchain" (`pipelinesAndDescriptorSetsInit`, re-run on every resize). Pipelines bake in the viewport size and descriptor sets are tied to the swapchain image count, so both must be rebuilt; models, textures and layouts survive untouched.

---

## 7. Cleanup — reverse order

At shutdown `run()` calls `cleanup()`, which invokes your two teardown hooks. The comment in `App.cpp` states the rule: *pipelines before pipeline layouts before descriptor set layouts, framebuffers before render passes, everything before the device.*

- **`pipelinesAndDescriptorSetsCleanup()`** — the swapchain-dependent half. Destroys pipelines (`P`, `Poverlay`), the render pass framebuffers (`RP.cleanup`), and the descriptor sets (`DSglobal`, `DSsplash`, scene, text). This also runs before every resize.
- **`localCleanup()`** — the permanent half. Destroys descriptor set *layouts*, the pipeline shader modules (`P.destroy`), models, textures, the render pass itself (`RP.destroy`), the scene and text.

Descriptor *sets* are never destroyed one-by-one here — they die with the descriptor pool that the framework owns. You only explicitly clean up the layouts, pipelines, render pass, models and textures you created.

---

## 8. Cheat sheet — which method runs when

| Your method (`App.cpp`) | Called by | When | Purpose |
|---|---|---|---|
| `setWindowParameters` | `run` | once, first | window size/title/aspect |
| `localInit` | `initVulkan` | once | declare all resources, record main cmd buffer |
| `pipelinesAndDescriptorSetsInit` | `initVulkan` + `recreateSwapChain` | once + every resize | build pipelines & descriptor sets |
| `populateCommandBuffer` | via `submitCommandBuffer` | once, then on invalidation | record *what* to draw |
| `updateUniformBuffer` | `drawFrame` | **every frame** | update matrices, lights, flags |
| `onWindowResize` | GLFW callback | on resize | update aspect & render-pass size |
| `pipelinesAndDescriptorSetsCleanup` | `cleanup` + `recreateSwapChain` | shutdown + every resize | destroy swapchain-dependent objects |
| `localCleanup` | `cleanup` | once, last | destroy permanent objects |

**The one-sentence summary:** `localInit` describes everything once, `pipelinesAndDescriptorSetsInit` turns those descriptions into GPU objects, `updateUniformBuffer` feeds them fresh data every frame, `populateCommandBuffer` lists the draw calls, and the framework's `drawFrame` submits and presents — over and over — until the window closes.
