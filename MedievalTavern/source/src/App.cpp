#include "App.hpp"

// Here you set the main application parameters
void MedievalTavern::setWindowParameters() {
	// window size, titile and initial background
	windowWidth = 800;
	windowHeight = 600;
	windowTitle = "Medieval Tavern 3D - CG Project";
    windowResizable = GLFW_TRUE;

	// Initial aspect ratio
	Ar = 4.0f / 3.0f;
}

// What to do when the window changes size
void MedievalTavern::onWindowResize(int w, int h) {
	std::cout << "Window resized to: " << w << " x " << h << "\n";
	Ar = (float)w / (float)h;
	// Update Render Pass
	RP.width = w;
	RP.height = h;

	// updates the textual output
	txt.resizeScreen(w, h);
}

// Here you load and setup all your Vulkan Models and Texutures.
// Here you also create your Descriptor set layouts and load the shaders for the pipelines
void MedievalTavern::localInit() {

//	1. We initialize the Descriptor Set Layouts
	DSLInit();

//	2. We initialize the Vertex Descriptors
	VDInit();

//	3. We initialize the render passes
	RP.init(this);
	// sets the blue sky
	RP.properties[0].clearValue = {0.0f,0.9f,1.0f,1.0f};

// 4. We initialize the render passes
	PipelineInit();

	// sets the size of the Descriptor Set Pool (it MUST be done before loading the scene)
	DPSZs.uniformBlocksInPool = 10;
	DPSZs.texturesInPool = 10;
	DPSZs.setsInPool = 10;

	// to support scene
	VDRs.resize(1);
	VDRs[0].init("VDposUV",  &VD);

	PRs.resize(1);
	PRs[0].init("BlinnPos", {
						{&P, {//Pipeline and DSL for the main pass
						 /*DSLglobal*/{},
						 /*DSLlocal*/{
								/*t0*/{true,  0, {}}
							  }
							 }
							}
					  }, /*TotalNtextures*/1, &VD);

	if(SC.init(this, 1, VDRs, PRs, "assets/scenes/scene.json") != 0) {
		std::cout << "ERROR LOADING THE SCENE\n";
		exit(0);
	}

	TexturesInit();

	OverlayMeshInit(windowWidth, windowHeight, -1.0f, -1.0f, &MsplashScreen, 1);
	OverlayMeshInit(windowWidth/2, windowHeight/2, -1.0f, -1.0f, &Mlogo, 1);
	OverlayMeshInit(windowWidth/5, windowHeight/5, -1.0f, 0.0f, &MexploreButton, 1);
	OverlayMeshInit(windowWidth/5, windowHeight/5, -0.2f, 0.0f, &MsettingsButton, 1);
	OverlayMeshInit(windowWidth/5, windowHeight/5, 0.6f, 0.0f, &MquitButton, 1);


	// initializes the textual output
	txt.init(this, windowWidth, windowHeight);

	// submits the main command buffer
	submitCommandBuffer("main", 0, populateCommandBufferAccess, this);

	// Prepares for showing the FPS count
	txt.print(1.0f, 1.0f, "FPS:",1,"CO",false,false,true,TAL_RIGHT,TRH_RIGHT,TRV_BOTTOM,{1.0f,0.0f,0.0f,1.0f},{0.8f,0.8f,0.0f,1.0f});
}

// Here you create your pipelines and Descriptor Sets!
void MedievalTavern::pipelinesAndDescriptorSetsInit() {
	// creates the render passes
	RP.create();

	// This creates a new pipeline (with the current surface), using its shaders for the provided render pass
	P.create(&RP);
	Poverlay.create(&RP);

	DSglobal.init(this, &DSLglobal, {});

	// Splash overlay descriptor set: binding 0 = OverlayUniformBuffer, binding 1 = the splash texture
	DSsplashScreen.init(this, &DSLoverlay, {TsplashScreen.getViewAndSampler()});
	DSlogo.init(this, &DSLoverlay, {Tlogo.getViewAndSampler()});
	DSexploreButton.init(this, &DSLoverlay, {TexploreButton.getViewAndSampler()});
	DSsettingsButton.init(this, &DSLoverlay, {TsettingsButton.getViewAndSampler()});
	DSquitButton.init(this, &DSLoverlay, {TquitButton.getViewAndSampler()});

	// Here you define the data set
	// If the scene has textures coming from a render pass, the corresponding element of the technique must be
	// updated before calling SC.pipelinesAndDescriptorSetsInit();

	SC.pipelinesAndDescriptorSetsInit();
	txt.pipelinesAndDescriptorSetsInit();
}

// Here you destroy your pipelines and Descriptor Sets!
void MedievalTavern::pipelinesAndDescriptorSetsCleanup() {
	// Vulkan cleanup is order-sensitive: pipelines before pipeline layouts before descriptor set layouts, framebuffers before render passes, and everything before the device/allocator.

	P.cleanup();
	Poverlay.cleanup();

	RP.cleanup();

	DSglobal.cleanup();
	DSsplashScreen.cleanup();
	DSlogo.cleanup();
	DSexploreButton.cleanup();
	DSsettingsButton.cleanup();
	DSquitButton.cleanup();


	SC.pipelinesAndDescriptorSetsCleanup();
	txt.pipelinesAndDescriptorSetsCleanup();
}

// Here you destroy all the Models, Texture and Desc. Set Layouts you created!
// You also have to destroy the pipelines
void MedievalTavern::localCleanup() {
	DSLlocal.cleanup();
	DSLglobal.cleanup();
	DSLoverlay.cleanup();

	P.destroy();
	Poverlay.destroy();

	// release models
	MsplashScreen.cleanup();
	Mlogo.cleanup();
	MexploreButton.cleanup();
	MsettingsButton.cleanup();
	MquitButton.cleanup();

	// release textures
	TsplashScreen.cleanup();
	Tlogo.cleanup();
	TexploreButton.cleanup();
	TsettingsButton.cleanup();
	TquitButton.cleanup();

	RP.destroy();

	SC.localCleanup();
	txt.localCleanup();

}

// Here it is the creation of the command buffer:
// You send to the GPU all the objects you want to draw,
// with their buffers and textures
void MedievalTavern::populateCommandBufferAccess(VkCommandBuffer commandBuffer, int currentImage, void *Params) {
	// Simple trick to avoid having always 'T->'
	// in che code that populates the command buffer!
	MedievalTavern *T = (MedievalTavern *)Params;
	T->populateCommandBuffer(commandBuffer, currentImage);
}

void MedievalTavern::populateCommandBuffer(VkCommandBuffer commandBuffer, int currentImage) {

	// Offscreen pass - always required
	// begin standard pass
	RP.begin(commandBuffer, currentImage);

	SC.populateCommandBuffer(commandBuffer, 0, currentImage);

	// Draw the overlay splash on top of the 3D scene
	Poverlay.bind(commandBuffer);

	MsplashScreen.bind(commandBuffer);
	DSsplashScreen.bind(commandBuffer, Poverlay, 0, currentImage);
	vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(MsplashScreen.indices.size()), 1, 0, 0, 0);

	Mlogo.bind(commandBuffer);
	DSlogo.bind(commandBuffer, Poverlay, 0, currentImage);
	vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(Mlogo.indices.size()), 1, 0, 0, 0);

	MexploreButton.bind(commandBuffer);
	DSexploreButton.bind(commandBuffer, Poverlay, 0, currentImage);
	vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(MexploreButton.indices.size()), 1, 0, 0, 0);

	MsettingsButton.bind(commandBuffer);
	DSsettingsButton.bind(commandBuffer, Poverlay, 0, currentImage);
	vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(MsettingsButton.indices.size()), 1, 0, 0, 0);

	MquitButton.bind(commandBuffer);
	DSquitButton.bind(commandBuffer, Poverlay, 0, currentImage);
	vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(MquitButton.indices.size()), 1, 0, 0, 0);

	RP.end(commandBuffer);
}

// Here is where you update the uniforms.
// Very likely this will be where you will be writing the logic of your application.
void MedievalTavern::updateUniformBuffer(uint32_t currentImage) {
	float cursorX;
	float cursorY;

	double mx, my;
	glfwGetCursorPos(window, &mx, &my);
	cursorX = (float)(mx / windowWidth) * 2.0f - 1.0f;
	cursorY = (float)(my / windowHeight) * 2.0f - 1.0f;

	// handle the ESC key to exit the app
	if(glfwGetKey(window, GLFW_KEY_ESCAPE)) {
		glfwSetWindowShouldClose(window, GL_TRUE);
	}

	// --- State machine ---
	if (gameState == MENU_STATE) {
		static bool clickDebounce = false;
		bool leftclicked = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
		if (leftclicked && !clickDebounce) {          // fire once on
			if      (isCursorInsideButton(cursorX, cursorY, &MexploreButton))  gameState = GAME_STATE;
			else if (isCursorInsideButton(cursorX, cursorY, &MsettingsButton)) { /* settings */ }
			else if (isCursorInsideButton(cursorX, cursorY, &MquitButton))     glfwSetWindowShouldClose(window, GL_TRUE);
		} else if (!leftclicked) {
			clickDebounce = false;
		}
	}

	if (gameState == GAME_STATE)
	{
		static bool key3debounce = false;
		if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS && !key3debounce) {
			gameState = MENU_STATE;
		} else if (glfwGetKey(window, GLFW_KEY_3) == GLFW_RELEASE) {
			key3debounce = false;
		}
	}

	// Splash is visible only on the menu. (The command buffer always draws it;
	// the shader discards every pixel when visible < 0.5.)
	OverlayUniformBuffer subo{};
	subo.visible = (gameState == MENU_STATE) ? 1.0f : 0.0f;
	DSsplashScreen.map(currentImage, &subo, 0);
	DSlogo.map(currentImage, &subo, 0);
	DSexploreButton.map(currentImage, &subo, 0);
	DSsettingsButton.map(currentImage, &subo, 0);
	DSquitButton.map(currentImage, &subo, 0);
	//-----------------

	// moves the view
	float deltaT = GameLogic();

	// defines the global parameters for the uniform
	static float lightRotationAngle = 0.0f; // Static variable to keep track of rotation
	lightRotationAngle += -0.5f * deltaT; // Increment rotation angle based on time

	const glm::mat4 lightView = glm::rotate(glm::mat4(1), glm::radians(lightRotationAngle), glm::vec3(0.0f, 1.0f, 0.0f)) *
								glm::rotate(glm::mat4(1), glm::radians(-45.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	const glm::vec3 lightDir =  glm::vec3(lightView * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f));

	GlobalUniformBufferObject gubo{};

	gubo.lightDir = lightDir;
	gubo.lightColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f)*5.0f;
	gubo.eyePos = glm::vec3(glm::inverse(View)[3]);

	DSglobal.map(currentImage, &gubo, 0);

	// defines the local parameters for the uniforms
	UniformBufferObject ubo{};

	int instanceId;
	// character
	for(instanceId = 0; instanceId < SC.TI[0].InstanceCount; instanceId++) {
		ubo.mMat = SC.TI[0].I[instanceId].Wm;
		ubo.mvpMat = ViewPrj * ubo.mMat;

		// DS[1] = Pchar pass (main render): set0=DSLglobal, set1=DSLlocal
		SC.TI[0].I[instanceId].DS[0][0]->map(currentImage, &gubo, 0); // global (light/camera)
		SC.TI[0].I[instanceId].DS[0][1]->map(currentImage, &ubo, 0); // camera MVPs
	}

	// updates the FPS
	static float elapsedT = 0.0f;
	static int countedFrames = 0;

	countedFrames++;
	elapsedT += deltaT;
	if(elapsedT > 1.0f) {
		float Fps = (float)countedFrames / elapsedT;

		std::ostringstream oss;
		oss << "FPS: " << Fps << "\n";

		txt.print(1.0f, 1.0f, oss.str(), 1, "CO", false, false, true,TAL_RIGHT,TRH_RIGHT,TRV_BOTTOM,{1.0f,0.0f,0.0f,1.0f},{0.8f,0.8f,0.0f,1.0f});

		elapsedT = 0.0f;
	    countedFrames = 0;
	}

	txt.updateCommandBuffer();
}

float MedievalTavern::GameLogic() {
	// Camera FOV-y, Near Plane and Far Plane
	const float FOVy = glm::radians(45.0f);
	const float nearPlane = 0.1f;
	const float farPlane = 100.f;

	// Integration with the timers and the controllers
	float deltaT;
	glm::vec3 m = glm::vec3(0.0f), r = glm::vec3(0.0f);
	bool fire = false;
	getSixAxis(deltaT, m, r, fire);

	// Projection
	glm::mat4 Prj = glm::perspective(FOVy, Ar, nearPlane, farPlane);
	Prj[1][1] *= -1;

	// View
	View = glm::lookAt(glm::vec3(0.0f, 1.0f, 5.0f), // Pos
					   glm::vec3(0.0f),				// Target
					   glm::vec3(0.0f, 1.0f, 0.0f));

	// View-Projection
	ViewPrj = Prj * View;

	return deltaT;
}

void MedievalTavern::DSLInit()
{
	// Descriptor Layouts [what will be passed to the shaders]
	DSLlocal.init(this, {
				// this array contains the binding:
				// first  element : the binding number
				// second element : the type of element (buffer or texture)
				// third  element : the pipeline stage where it will be used
				{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, sizeof(UniformBufferObject), 1},
				{1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 1}
			  });
	DSLoverlay.init(this, {
				{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS, sizeof(OverlayUniformBuffer), 1},
				{1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 1}
			  });
	DSLglobal.init(this, {
				// this array contains the binding:
				// first  element : the binding number
				// second element : the type of element (buffer or texture)
				// third  element : the pipeline stage where it will be used
				{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS, sizeof(GlobalUniformBufferObject), 1}
			  });
}

void MedievalTavern::VDInit()
{
	VD.init(this, {
			  {0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX}
			}, {
			  {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, pos),
					 sizeof(glm::vec3), POSITION},
			  {0, 1, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, UV),
					 sizeof(glm::vec2), UV}
			});
	VDoverlay.init(this, {
			{0, sizeof(VertexOverlay), VK_VERTEX_INPUT_RATE_VERTEX}
		}, {
			{0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(VertexOverlay, pos), sizeof(glm::vec2), OTHER},
			{0, 1, VK_FORMAT_R32G32_SFLOAT, offsetof(VertexOverlay, UV),  sizeof(glm::vec2), UV}
		});
}

void MedievalTavern::PipelineInit()
{
	// Pipelines [Shader couples]
	// The last array, is a vector of pointer to the layouts of the sets that will
	// be used in this pipeline. The first element will be set 0, and so on..

	P.init(this, &VD, "shaders/toChangeSimplePos.vert.spv",
					  "shaders/toChangeBlinnFromPos.frag.spv",
					  {&DSLglobal, &DSLlocal});

	Poverlay.init(this, &VDoverlay, "shaders/Overlay.vert.spv",
						"shaders/Overlay.frag.spv",
						{&DSLoverlay});
	Poverlay.setCompareOp(VK_COMPARE_OP_ALWAYS);   // ignore depth — always draw on top
	Poverlay.setCullMode(VK_CULL_MODE_NONE);       // a 2D quad has no "back face"
	Poverlay.setTransparency(true);                // respect the texture's alpha
}

void MedievalTavern::TexturesInit()
{
	TsplashScreen.init(this, "assets/textures/Overlay/MenuSplashImage.png");
	Tlogo.init(this, "assets/textures/Overlay/TavernLogo.png");
	TexploreButton.init(this, "assets/textures/Overlay/ExploreButton.png");
	TsettingsButton.init(this, "assets/textures/Overlay/SettingsButton.png");
	TquitButton.init(this, "assets/textures/Overlay/QuitButton.png");
}

void MedievalTavern::OverlayMeshInit(float pxWidth, float pxHeight, float NSC_X, float NSC_Y, Model *model, int count){
	// Pixels -> NSC. The screen spans 2 NSC units (-1..1), so a fraction f
	// of the screen width is f*2 units wide.
	float widthNDC  = (pxWidth / windowWidth  ) * 2.0f;
	float heightNDC = (pxHeight / windowHeight ) * 2.0f;

	// Defines 4 vertices for a rectangle (quad). Each VertexOverlay holds two things: a 2D position {x, y} and a texture coordinate {u, v}.
	std::vector<VertexOverlay> vertices = {
		{{NSC_X,            NSC_Y            }, {0.0f, 0.0f}},
		{{NSC_X,            NSC_Y + heightNDC}, {0.0f, 1.0f}},
		{{NSC_X + widthNDC, NSC_Y            }, {1.0f, 0.0f}},
		{{NSC_X + widthNDC, NSC_Y + heightNDC}, {1.0f, 1.0f}}
	};

	// The quad is drawn as 2 triangles, first with vertices 0,1,2 and and second with vertices 1,2,3
	std::vector<uint32_t> indices = {0, 1, 2, 1, 2, 3};

	for (int i = 0; i < count; i++) {
		for (const auto &v : vertices) {
			// cast to byte
			const auto *bytes = reinterpret_cast<const unsigned char *>(&v);
			//append each vertex to the end of the "vertex buffer" of the model
			model[i].vertices.insert(model[i].vertices.end(), bytes, bytes + sizeof(VertexOverlay));
		}
		model[i].indices = indices;
		model[i].initMesh(this, &VDoverlay);
	}
}

bool MedievalTavern::isCursorInsideButton(float x, float y, Model *model)
{
	// The overlay model's vertex buffer is raw VertexOverlay bytes, and their
	// positions are already in NSC — the same space as (cursorX, cursorY).
	const auto *v = reinterpret_cast<const VertexOverlay *>(model->vertices.data());
	size_t n = model->vertices.size() / sizeof(VertexOverlay);
	if (n == 0) return false;

	// Axis-aligned bounds of the quad
	glm::vec2 lo = v[0].pos;
	glm::vec2 hi = v[0].pos;
	for (size_t i = 1; i < n; ++i) {
		lo = glm::min(lo, v[i].pos);
		hi = glm::max(hi, v[i].pos);
	}

	return x >= lo.x && x <= hi.x && y >= lo.y && y <= hi.y;
}
