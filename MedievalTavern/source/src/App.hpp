#pragma once
#include <sstream>
#include <json.hpp>

#include "modules/Starter.hpp"
#include "modules/TextMaker.hpp"
#include "modules/Scene.hpp"
#include "App.hpp"


// The "screens" the app can be on.
enum GameState { MENU_STATE, GAME_STATE };

// The uniform buffer object used in this example
struct UniformBufferObject {
    alignas(16) glm::mat4 mvpMat;
    alignas(16) glm::mat4 mMat;
};

struct GlobalUniformBufferObject {
    alignas(16) glm::vec3 lightDir;
    alignas(16) glm::vec4 lightColor;
    alignas(16) glm::vec3 eyePos;
};

// Uniform sent to the overlay shaders. For now just a show/hide flag.
struct OverlayUniformBuffer {
    alignas(16) float visible;
};

struct Vertex {
    glm::vec3 pos;
    glm::vec2 UV;
};

struct VertexOverlay {
    glm::vec2 pos;
    glm::vec2 UV;
};


class MedievalTavern : public BaseProject
{
protected:

    // Here you list all the Vulkan objects you need:

    // Descriptor Layouts [what will be passed to the shaders]
    DescriptorSetLayout DSLlocal, DSLglobal;

    // Vertex formants, Pipelines [Shader couples] and Render passes
    VertexDescriptor VD;
    RenderPass RP;
    Pipeline P;
    Pipeline Poverlay;


    // Models, textures and Descriptors (values assigned to the uniforms)
    DescriptorSet DSglobal;

    // To support loading assets from a scene.json file
    Scene SC;
    std::vector<VertexDescriptorRef>  VDRs;
    std::vector<TechniqueRef> PRs;

    // to provide textual feedback
    TextMaker txt;

    // Other application parameters
    float Ar;	// Aspect ratio

    glm::mat4 ViewPrj;
    glm::mat4 View;

    // --- Overlay (menu / splash) objects ---
    DescriptorSetLayout DSLoverlay;
    VertexDescriptor VDoverlay;
    // for the main menu
    Texture TsplashScreen, Tlogo, TexploreButton, TsettingsButton, TquitButton;// the splash image
    Model MsplashScreen, Mlogo, MexploreButton, MsettingsButton, MquitButton;            // a full-screen quad to paint it on
    DescriptorSet DSsplashScreen, DSlogo, DSexploreButton, DSsettingsButton, DSquitButton;
    // for dialogues
    Texture TdialogueBox;
    Model MdialogueBox;
    DescriptorSet DSdialogueBox;

    // Which screen we're on; we start on the menu.
    GameState gameState = MENU_STATE;


    void setWindowParameters();

    void onWindowResize(int w, int h);

    void localInit();

    void pipelinesAndDescriptorSetsInit();

    void pipelinesAndDescriptorSetsCleanup();

    void localCleanup();

    static void populateCommandBufferAccess(VkCommandBuffer commandBuffer, int currentImage, void *Params);

    void populateCommandBuffer(VkCommandBuffer commandBuffer, int currentImage);

    void updateUniformBuffer(uint32_t currentImage);

    float GameLogic();

private:
    void DSLInit();

    void VDInit();

    void PipelineInit();

    void SceneInit();

    void TexturesInit();

    ///
    /// @param pxWidth: desired quad width in pixels
    /// @param pxHeight: desired quad height in pixels
    /// @param NSC_X: top-left corner X coord in Normalized Screen Coords (-1..1)
    /// @param NSC_Y: top-left corner Y coord in Normalized Screen Coords (-1..1)
    /// @param model: pointer to the model to initialize
    /// @param count: how many identical copies to build (e.g. 2 for a two-state button)
    void OverlayMeshInit(float pxWidth, float pxHeight, float NSC_X, float NSC_Y, Model *model, int count);

    ///
    /// @param x cursor ScreenCoord x
    /// @param y cursor ScreenCoord y
    /// @param model model to check
    /// @return
    bool isCursorInsideButton(float x, float y, Model *model);

};