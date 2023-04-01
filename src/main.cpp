#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_glfw.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb_image.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/filesystem.h>
#include <learnopengl/shader_m.h>
#include <learnopengl/model.h>
#include <learnopengl/camera.h>

#include <iostream>
#include <cmath>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void key_callback(GLFWwindow* window,int key,int scancode,int action,int mods);
void processInput(GLFWwindow *window);
unsigned int loadTexture(const char *path);
unsigned int loadCubemap(vector<std::string> faces);
void renderQuad();


bool blinn = true;
bool blinnKeyPressed = false;
bool hdr = false;
bool hdrKeyPressed = false;
float exposure = 1.0f;
bool bloom = false;
bool bloomKeyPressed = false;

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// camera
Camera camera(glm::vec3(-0.5f, 1.0f, 12.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

glm::vec3 lightPos(1.2f, 1.0f, 2.0f);

struct PointLight {
    glm::vec3 position;

    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;

    float constant;
    float linear;
    float quadratic;
};

struct ProgramState{
    bool ImGuiEnabled = false;
    glm::vec3 pingvinPosition = glm::vec3(-3.5f,0.0f,-4.0f);

    glm::vec3 directDirection = glm::vec3(-1.0f,-0.2f,0.0f);
    glm::vec3 directAmbient = glm::vec3(0.2f, 0.2f, 0.2f);
    glm::vec3 directDiffuse = glm::vec3(0.5f, 0.5f, 0.5f);
    glm::vec3 directSpecular = glm::vec3(0.5f,0.5f,0.5f);


    glm::vec3 pointPosition = glm::vec3(0.0f,0.0f,0.0f);
    glm::vec3 pointAmbient = glm::vec3(0.05f, 0.05f, 0.05f);
    glm::vec3 pointDiffuse = glm::vec3(0.8f, 0.6f, 0.6f);
    glm::vec3 pointSpecular = glm::vec3(1.0f, 1.0f, 1.0f);
    float pointConstant = 1.0f;
    float pointLinear = 0.75f;
    float pointQuadratic = 0.032f;

    PointLight pointLight;
    
    void LoadFromDisk(std::string path);
    void SaveToDisk(std::string path);
};
void ProgramState::SaveToDisk(std::string path) {
    std::ofstream out(path);
    out << ImGuiEnabled << '\n'
        << pingvinPosition.x << '\n'
        << pingvinPosition.y << '\n'
        << pingvinPosition.z << '\n';
}
void ProgramState::LoadFromDisk(std::string path) {
    std::ifstream in(path);
    if(in){
        in >> ImGuiEnabled
        >> pingvinPosition.x
        >> pingvinPosition.y
        >> pingvinPosition.z;
    }
}
ProgramState* programState;
void drawImgui(ProgramState* programState);

// timing
float deltaTime = 0.0f;	// time between current frame and last frame
float lastFrame = 0.0f;

int main()
{
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "WinterWonderland", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window,key_callback);

    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // tell stb_image.h to flip loaded texture's on the y-axis (before loading model).
    //stbi_set_flip_vertically_on_load(true);


    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    ImGui::StyleColorsLight();
    ImGui_ImplGlfw_InitForOpenGL(window,true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    // TODO: vidi ovo dole
   // glEnable(GL_BLEND);
   // glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);



    programState = new ProgramState;
    programState->LoadFromDisk("resources/programState.txt");

    glfwSetInputMode(window,GLFW_CURSOR,GLFW_CURSOR_DISABLED);
    if(programState->ImGuiEnabled){
        glfwSetInputMode(window,GLFW_CURSOR,GLFW_CURSOR_NORMAL);
    }

    // build and compile our shader program
    // ------------------------------------
    Shader ourShader("resources/shaders/verShader.vs",
                     "resources/shaders/fragShader.fs");
    Shader poklonShader("resources/shaders/poklon.vs",
                        "resources/shaders/poklon.fs");
    Shader skyboxShader("resources/shaders/skybox.vs",
                        "resources/shaders/skybox.fs");
    Shader flakeShader("resources/shaders/flake.vs",
                       "resources/shaders/flake.fs");
    Shader hdrShader("resources/shaders/hdr.vs",
                     "resources/shaders/hdr.fs");
    Shader bloomShader("resources/shaders/blur.vs",
                       "resources/shaders/blur.fs");


    //ucitavanje modela
    Model mainOstrvo(FileSystem::getPath("resources/objects/Ostrvo/plain.obj"),true);
    mainOstrvo.SetShaderTextureNamePrefix("material.");
    Model levoOstrvo(FileSystem::getPath("resources/objects/Ostrvo/untitled2.obj"),true);
    levoOstrvo.SetShaderTextureNamePrefix("material.");
    Model desnoOstrvo(FileSystem::getPath("resources/objects/Ostrvo/untitled2.obj"), true);
    desnoOstrvo.SetShaderTextureNamePrefix("material.");
    Model dalekoOstrvo(FileSystem::getPath("resources/objects/Ostrvo/plain.obj"), true);
    dalekoOstrvo.SetShaderTextureNamePrefix("material.");
    Model snezana(FileSystem::getPath("resources/objects/SneskoGorl/sneska.obj"), true);
    snezana.SetShaderTextureNamePrefix("material.");
    Model sanke(FileSystem::getPath("resources/objects/Sanke/sanke.obj"), true);
    sanke.SetShaderTextureNamePrefix("material.");
    Model jelka(FileSystem::getPath("resources/objects/Jelka2/jelka.obj"), true);
    jelka.SetShaderTextureNamePrefix("material.");
    Model putokaz(FileSystem::getPath("resources/objects/Putokaz/poll.obj"), true);
    putokaz.SetShaderTextureNamePrefix("material.");
    Model pingvin(FileSystem::getPath("resources/objects/Pingvin/penguin/penguin.obj"), true);
    pingvin.SetShaderTextureNamePrefix("material.");
    //Model dedaMraz(FileSystem::getPath("resources/objects/Santa/dedamraz.obj"));
    //dedaMraz.SetShaderTextureNamePrefix("material.");


//-4.6f,-1.25f,3.5f
//5.3f,-1.5f,0.8f
//-2.9f, -0.53f, 0.0f
    // positions of the point lights
    glm::vec3 pointLightPositions[] = {
            glm::vec3( 0.1f, 1.4f, 0.0f),
            glm::vec3( 0.3f, 1.4f, 0.0f),
            glm::vec3( 0.3f, 0.8f, 1.0f),
            glm::vec3( 0.3f, 0.8f, 1.0f),
            glm::vec3( 0.3f, 0.8f, -2.1f),
            glm::vec3( 0.3f, 0.8f, -2.1f),
    };




    // set up vertex data (and buffer(s)) and configure vertex attributes
    // ------------------------------------------------------------------
    float vertices[] = {
            -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,
            0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  0.0f,
            0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  1.0f,
            0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  1.0f,
            -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  1.0f,
            -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,

            -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,
            0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  0.0f,
            0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,
            0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,
            -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  1.0f,
            -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,

            -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
            -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  1.0f,
            -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
            -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
            -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
            -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  0.0f,

            0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
            0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  1.0f,
            0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
            0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
            0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
            0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,

            -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  1.0f,
            0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  1.0f,
            0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  0.0f,
            0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  0.0f,
            -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,
            -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  1.0f,

            -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f,
            0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  1.0f,
            0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,
            0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,
            -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,
            -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f
    };

    float skyboxVertices[] = {
            // positions
            -1.0f,  1.0f, -1.0f,
            -1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,

            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,

            -1.0f, -1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,

            -1.0f,  1.0f, -1.0f,
            1.0f,  1.0f, -1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
            1.0f, -1.0f,  1.0f
    };

    float transparentVertices[] = {
            // positions         // texture Coords (swapped y coordinates because texture is flipped upside down)
            0.0f,  0.5f,  0.0f,  0.0f,  0.0f,
            0.0f, -0.5f,  0.0f,  0.0f,  1.0f,
            1.0f, -0.5f,  0.0f,  1.0f,  1.0f,

            0.0f,  0.5f,  0.0f,  0.0f,  0.0f,
            1.0f, -0.5f,  0.0f,  1.0f,  1.0f,
            1.0f,  0.5f,  0.0f,  1.0f,  0.0f
    };

    unsigned int VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), &vertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    //skybox
    unsigned int skyboxVAO, skyboxVBO;
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    //transparent
    unsigned int transparentVAO, transparentVBO;
    glGenVertexArrays(1, &transparentVAO);
    glGenBuffers(1, &transparentVBO);
    glBindVertexArray(transparentVAO);
    glBindBuffer(GL_ARRAY_BUFFER, transparentVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(transparentVertices), transparentVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glBindVertexArray(0);

    vector<glm::vec3> flakes
            {
                    glm::vec3(-6.5f, 2.0f, -0.48f),
                    glm::vec3( 3.5f, 4.0f, 1.51f),
                    glm::vec3( 2.0f, 1.5f, 0.7f),
                    glm::vec3(-4.3f, 3.5f, -2.3f),
                    glm::vec3( 6.0f, 4.5f, -1.6f),
                    glm::vec3( 0.0f, 4.0f, -1.6f)

            };

    unsigned int diffuseMap = loadTexture(FileSystem::getPath("resources/textures/gift.jpg").c_str());
    unsigned int transparentTexture = loadTexture(FileSystem::getPath("resources/textures/snowflake.png").c_str());


    vector<std::string> faces
            {
                    FileSystem::getPath("resources/textures/skybox/cottoncandy/cottoncandy_lf.tga"),
                    FileSystem::getPath("resources/textures/skybox/cottoncandy/cottoncandy_rt.tga"),
                    FileSystem::getPath("resources/textures/skybox/cottoncandy/cottoncandy_up.tga"),
                    FileSystem::getPath("resources/textures/skybox/cottoncandy/cottoncandy_dn.tga"),
                    FileSystem::getPath("resources/textures/skybox/cottoncandy/cottoncandy_ft.tga"),
                    FileSystem::getPath("resources/textures/skybox/cottoncandy/cottoncandy_bk.tga")

            };


    unsigned int cubemapTexture = loadCubemap(faces);

    PointLight& pointLight = programState->pointLight;

    ourShader.use();
    ourShader.setInt("material.texture_diffuse1",0);
    ourShader.setInt("material.texture_specular1",1);

    poklonShader.use();
    poklonShader.setInt("material.diffuse",0);

    skyboxShader.use();
    skyboxShader.setInt("skybox",0);

    flakeShader.use();
    flakeShader.setInt("texture1", 0);

    bloomShader.use();
    bloomShader.setInt("image",0);

    hdrShader.use();
    hdrShader.setInt("hdrBuffer",0);
    hdrShader.setInt("bloomBlur",1);


    //hdr
    unsigned int hdrFBO;
    glGenFramebuffers(1, &hdrFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);

    unsigned int colorBuffers[2];
    glGenTextures(2, colorBuffers);
    for (unsigned int i = 0; i < 2; i++)
    {
        glBindTexture(GL_TEXTURE_2D, colorBuffers[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        // attach texture to framebuffer
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, colorBuffers[i], 0);
    }
    // create and attach depth buffer (renderbuffer)
    unsigned int rboDepth;
    glGenRenderbuffers(1, &rboDepth);
    glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, SCR_WIDTH, SCR_HEIGHT);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);
    // color attachments we'll use for rendering
    unsigned int attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
    glDrawBuffers(2, attachments);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "Framebuffer not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // ping-pong-framebuffer for blurring
    unsigned int pingpongFBO[2];
    unsigned int pingpongColorbuffers[2];
    glGenFramebuffers(2, pingpongFBO);
    glGenTextures(2, pingpongColorbuffers);
    for (unsigned int i = 0; i < 2; i++)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[i]);
        glBindTexture(GL_TEXTURE_2D, pingpongColorbuffers[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // we clamp to the edge as the blur filter would otherwise sample repeated texture values!
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pingpongColorbuffers[i], 0);
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            std::cout << "Framebuffer not complete!" << std::endl;
    }

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window))
    {
        // per-frame time logic
        // --------------------
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        // -----
        processInput(window);

        //sort
        // -----
        std::sort(flakes.begin(), flakes.end(),
                  [cameraPosition = camera.Position](const glm::vec3& a, const glm::vec3& b) {
                      float d1 = glm::distance(a, cameraPosition);
                      float d2 = glm::distance(b, cameraPosition);
                      return d1 > d2;
                  });

        // render
        // ------
        glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // activate shader
        ourShader.use();

        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();
        ourShader.setVec3("viewPosition", camera.Position);
        ourShader.setMat4("projection", projection);
        ourShader.setMat4("view", view);
        ourShader.setInt("blinn",blinn);

        //std::cout << (blinn ? "Blinn-Phong" : "Phong") << std::endl;

        ourShader.setFloat("material.shininess", 32.0f);

        ourShader.setVec3("dirLight.direction",programState->directDirection);
         ourShader.setVec3("dirLight.ambient",programState->directAmbient);
         ourShader.setVec3("dirLight.diffuse",programState->directDiffuse);
         ourShader.setVec3("dirLight.specular",programState->directSpecular);

        //postavljanje boje svetla
        glm::vec3 lightColor;
        lightColor.x = static_cast<float>(sin(glfwGetTime() * 2.0));
        lightColor.y = static_cast<float>(sin(glfwGetTime() * 0.7));
        lightColor.z = static_cast<float>(sin(glfwGetTime() * 1.3));
        glm::vec3 diffuseColor = lightColor * glm::vec3(0.5f);


        ourShader.setVec3("pointLights[0].position", pointLightPositions[0]);
        ourShader.setVec3("pointLights[0].ambient", programState->pointAmbient * diffuseColor);
        ourShader.setVec3("pointLights[0].diffuse", diffuseColor);
        ourShader.setVec3("pointLights[0].specular", programState->pointSpecular);
        ourShader.setFloat("pointLights[0].constant", programState->pointConstant);
        ourShader.setFloat("pointLights[0].linear", programState->pointLinear);
        ourShader.setFloat("pointLights[0].quadratic", programState->pointQuadratic);

        // point light 2
        ourShader.setVec3("pointLights[1].position", pointLightPositions[1]);
        ourShader.setVec3("pointLights[1].ambient", programState->pointAmbient * diffuseColor);
        ourShader.setVec3("pointLights[1].diffuse", diffuseColor);
        ourShader.setVec3("pointLights[1].specular", programState->pointSpecular);
        ourShader.setFloat("pointLights[1].constant", programState->pointConstant);
        ourShader.setFloat("pointLights[1].linear", programState->pointLinear);
        ourShader.setFloat("pointLights[1].quadratic", programState->pointQuadratic);
        // point light 3
        ourShader.setVec3("pointLights[2].position", pointLightPositions[2]);
        ourShader.setVec3("pointLights[2].ambient", programState->pointAmbient * diffuseColor);
        ourShader.setVec3("pointLights[2].diffuse", diffuseColor);
        ourShader.setVec3("pointLights[2].specular", programState->pointSpecular);
        ourShader.setFloat("pointLights[2].constant", programState->pointConstant);
        ourShader.setFloat("pointLights[2].linear", programState->pointLinear);
        ourShader.setFloat("pointLights[2].quadratic", programState->pointQuadratic);
        // point light 4
        ourShader.setVec3("pointLights[3].position", pointLightPositions[3]);
        ourShader.setVec3("pointLights[3].ambient", programState->pointAmbient);
        ourShader.setVec3("pointLights[3].diffuse", programState->pointDiffuse);
        ourShader.setVec3("pointLights[3].specular", programState->pointSpecular);
        ourShader.setFloat("pointLights[3].constant", programState->pointConstant);
        ourShader.setFloat("pointLights[3].linear", programState->pointLinear);
        ourShader.setFloat("pointLights[3].quadratic", programState->pointQuadratic);

         // point light 5
        ourShader.setVec3("pointLights[4].position", pointLightPositions[4]);
        ourShader.setVec3("pointLights[4].ambient", programState->pointAmbient);
        ourShader.setVec3("pointLights[4].diffuse", programState->pointDiffuse);
        ourShader.setVec3("pointLights[4].specular", programState->pointSpecular);
        ourShader.setFloat("pointLights[4].constant", programState->pointConstant);
        ourShader.setFloat("pointLights[4].linear", programState->pointLinear);
        ourShader.setFloat("pointLights[4].quadratic", programState->pointQuadratic);

         // point light 6
        ourShader.setVec3("pointLights[5].position", pointLightPositions[5]);
        ourShader.setVec3("pointLights[5].ambient", programState->pointAmbient);
        ourShader.setVec3("pointLights[5].diffuse", programState->pointDiffuse);
        ourShader.setVec3("pointLights[5].specular", programState->pointSpecular);
        ourShader.setFloat("pointLights[5].constant", programState->pointConstant);
        ourShader.setFloat("pointLights[5].linear", programState->pointLinear);
        ourShader.setFloat("pointLights[5].quadratic", programState->pointQuadratic);




        glm::mat4 model = glm::mat4 (1.0f);

        ourShader.use();

        ourShader.setMat4("model", model);


        //renderovanje modela
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        // main ostrvo
        model = glm::translate(model, glm::vec3(0.0f, -3.0f, 0.0f));
        model = glm::scale(model, glm::vec3(0.8f, 0.8f, 0.8f));
        model = glm::rotate(model,glm::radians((float)-90),glm::vec3(0.0f,1.0f,0.0f));
        ourShader.setMat4("model", model);
        mainOstrvo.Draw(ourShader);

        //levo ostrvo
        model = glm::mat4 (1.0f);
        model = glm::translate(model,glm::vec3(-5.0f,-3.0f,2.5f));
        model = glm::scale(model, glm::vec3(0.6f, 0.6f, 0.6f));
        ourShader.setMat4("model", model);
        levoOstrvo.Draw(ourShader);

        //desno ostrvo
        model = glm::mat4 (1.0f);
        model = glm::translate(model,glm::vec3(5.0f,-3.0f,0.0f));
        model = glm::scale(model, glm::vec3(0.5f, 0.5f, 0.5f));
        ourShader.setMat4("model", model);
        desnoOstrvo.Draw(ourShader);

        //daleko ostrvo
        model = glm::mat4(1.0f);
        model = glm::translate(model,glm::vec3(-5.0f,-3.0f,-3.0f));
        model = glm::scale(model, glm::vec3(0.5f, 0.5f, 0.5f));
        ourShader.setMat4("model", model);
        dalekoOstrvo.Draw(ourShader);

        //snezana
        model = glm::mat4 (1.0f);
        model = glm::translate(model,glm::vec3(5.3f,-1.5f,0.8f));
        model = glm::scale(model, glm::vec3(0.2f, 0.2f, 0.2f));
        //model = glm::rotate(model,(float)glfwGetTime(),glm::vec3(1.0f,0.0f,0.0f));
        ourShader.setMat4("model", model);
        snezana.Draw(ourShader);


        //sanke
        model = glm::mat4 (1.0f);
        model = glm::translate(model,glm::vec3(-4.6f,-1.25f,3.5f));
        model = glm::scale(model, glm::vec3(0.17f, 0.17f, 0.17f));
        model = glm::rotate(model,(float)glfwGetTime(),glm::vec3(0.0f,1.0f,0.0f));
        ourShader.setMat4("model", model);
        sanke.Draw(ourShader);

        //jelka
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(-2.9f, -0.53f, 0.0f));
        model = glm::scale(model, glm::vec3(0.5f, 0.5f, 0.5f));
        ourShader.setMat4("model", model);
        jelka.Draw(ourShader);

        //putokaz

        model = glm::mat4 (1.0f);
        model = glm::translate(model,glm::vec3(6.2f,-1.5f,-0.9f));
        model = glm::scale(model, glm::vec3(0.6f, 0.6f, 0.6f));
        ourShader.setMat4("model", model);
        putokaz.Draw(ourShader);


        //pingvin
        model = glm::mat4(1.0f);
        //model = glm::translate(model,glm::vec3(-3.5f,0.0f,-4.0f));
        model = glm::translate(model,programState->pingvinPosition);
        model = glm::scale(model,glm::vec3(0.3f,0.3f,0.3f));
        //model = glm::rotate(model,(float)glfwGetTime(),glm::vec3(1.0f,0.0f,0.0f));
        ourShader.setMat4("model", model);
        pingvin.Draw(ourShader);
        glDisable(GL_CULL_FACE);

        //deda
        /*model = glm::mat4(1.0f);
        model = glm::translate(model,glm::vec3(-5.0f,-1.55f,-3.0f));
        model = glm::scale(model, glm::vec3(0.005f, 0.005f, 0.005f));
        ourShader.setMat4("model", model);
        dedaMraz.Draw(ourShader);*/


        //renderovanje poklona
        //glEnable(GL_CULL_FACE);
        //glCullFace(GL_BACK);
        poklonShader.use();
        poklonShader.setMat4("projection",projection);
        poklonShader.setMat4("view",view);

        poklonShader.setVec3("light.position", lightPos);
        poklonShader.setVec3("viewPos", camera.Position);

        // light properties
        poklonShader.setVec3("light.ambient", 0.2f, 0.2f, 0.2f);
        poklonShader.setVec3("light.diffuse", 0.5f, 0.5f, 0.5f);
        poklonShader.setVec3("light.specular", 1.0f, 1.0f, 1.0f);

        // material properties
        poklonShader.setVec3("material.specular", 0.5f, 0.5f, 0.5f);
        poklonShader.setFloat("material.shininess", 64.0f);


        glBindVertexArray(VAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, diffuseMap);
        model = glm::mat4(1.0f);
        model = glm::translate(model,glm::vec3(-1.0f,-0.5f,0.3f));
        model = glm::scale(model,glm::vec3(0.4f,0.4f,0.4f));
        //model = glm::rotate(model,glm::radians((float)-65),glm::vec3(0.0f,1.0f,0.0f));
        poklonShader.setMat4("model",model);
        glDrawArrays(GL_TRIANGLES,0,36);
        //glDisable(GL_CULL_FACE);




        //snowflakes
        flakeShader.use();
        flakeShader.setMat4("projection",projection);
        flakeShader.setMat4("view",view);
        glBindVertexArray(transparentVAO);
        glBindTexture(GL_TEXTURE_2D, transparentTexture);
        for (const glm::vec3& f : flakes)
        {
            model = glm::mat4(1.0f);
            model = glm::translate(model, f);
            model = glm::scale(model,glm::vec3(0.5f,0.5f,0.5f));
            flakeShader.setMat4("model", model);
            glDrawArrays(GL_TRIANGLES, 0, 6);

        }

        // draw skybox as last
        glDepthFunc(GL_LEQUAL);
        skyboxShader.use();
        view = glm::mat4(glm::mat3(camera.GetViewMatrix()));
        skyboxShader.setMat4("view", view);
        skyboxShader.setMat4("projection", projection);
        // skybox cube
        glBindVertexArray(skyboxVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
        glDepthFunc(GL_LESS);

        //load ping-pong
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        bool horizontal = true, first_iteration = true;
        unsigned int amount = 10;
        bloomShader.use();
        for (unsigned int i = 0; i < amount; i++)
        {
            glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[horizontal]);
            bloomShader.setInt("horizontal", horizontal);
            glBindTexture(GL_TEXTURE_2D, first_iteration ? colorBuffers[1] : pingpongColorbuffers[!horizontal]);

            renderQuad();

            horizontal = !horizontal;
            if (first_iteration)
                first_iteration = false;
        }

        // load hdr and bloom
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        hdrShader.use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, colorBuffers[0]);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, pingpongColorbuffers[!horizontal]);
        hdrShader.setBool("hdr", hdr);
        hdrShader.setBool("bloom",bloom);
        hdrShader.setFloat("exposure", exposure);
        renderQuad();

       // std::cout << "bloom: " << (bloom ? "on" : "off") << "| exposure: " << exposure << std::endl;


        if(programState->ImGuiEnabled){
            drawImgui(programState);
        }


        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }


    // optional: de-allocate all resources once they've outlived their purpose:
    // ------------------------------------------------------------------------
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteVertexArrays(1, &skyboxVAO);
    glDeleteBuffers(1, &skyboxVAO);

    programState->SaveToDisk("resources/programState.txt");

    //ImGui cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    delete programState;

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

unsigned int quadVAO = 0;
unsigned int quadVBO;
void renderQuad()
{
    if (quadVAO == 0)
    {
        float quadVertices[] = {
                // positions        // texture Coords
                -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
                -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
                1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
                1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        };
        // setup plane VAO
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    }
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);

    if (glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS && !blinnKeyPressed)
    {
        blinn = !blinn;
        blinnKeyPressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_B) == GLFW_RELEASE)
    {
        blinnKeyPressed = false;
    }
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && !hdrKeyPressed)
    {
        hdr = !hdr;
        hdrKeyPressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_RELEASE)
    {
        hdrKeyPressed = false;
    }

    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
    {
        if (exposure > 0.0f)
            exposure -= 0.001f;
        else
            exposure = 0.0f;
    }
    else if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
    {
        exposure += 0.001f;
    }
    if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS && !bloomKeyPressed)
    {
        bloom = !bloom;
        bloomKeyPressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_F) == GLFW_RELEASE)
    {
        bloomKeyPressed = false;
    }
}
// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    if(programState->ImGuiEnabled == false) {
        camera.ProcessMouseMovement(xoffset, yoffset);
    }
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(yoffset);
}
unsigned int loadTexture(char const * path)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, format == GL_RGBA ? GL_CLAMP_TO_EDGE : GL_REPEAT); // for this tutorial: use GL_CLAMP_TO_EDGE to prevent semi-transparent borders. Due to interpolation it takes texels from next repeat
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, format == GL_RGBA ? GL_CLAMP_TO_EDGE : GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}
void drawImgui(ProgramState* programState){
    //init
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    {
        ImGui::Begin("Window");
        ImGui::Text("You can move the penguin");
        ImGui::DragFloat3("Penguin position",(float*)&programState->pingvinPosition);
        ImGui::End();
    }


    //render
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}
void key_callback(GLFWwindow* window,int key,int scancode,int action,int mods){
    if(key == GLFW_KEY_F1 && action == GLFW_PRESS){
        programState->ImGuiEnabled = !programState->ImGuiEnabled;
        if(programState->ImGuiEnabled){
            glfwSetInputMode(window,GLFW_CURSOR,GLFW_CURSOR_NORMAL);
        }
        else{
            glfwSetInputMode(window,GLFW_CURSOR,GLFW_CURSOR_DISABLED);
        }
    }
}
unsigned int loadCubemap(vector<std::string> faces)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;
    for (unsigned int i = 0; i < faces.size(); i++)
    {

        unsigned char *data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        }
        else
        {
            std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}