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



// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// camera
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
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
    //glm::vec3 pingvinPosition = glm::vec3(1.0f);

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
    stbi_set_flip_vertically_on_load(true);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    ImGui::StyleColorsLight();
    ImGui_ImplGlfw_InitForOpenGL(window,true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);

    programState = new ProgramState;
    programState->LoadFromDisk("resources/programState.txt");

    glfwSetInputMode(window,GLFW_CURSOR,GLFW_CURSOR_DISABLED);
    if(programState->ImGuiEnabled){
        glfwSetInputMode(window,GLFW_CURSOR,GLFW_CURSOR_NORMAL);
    }

    // build and compile our shader zprogram
    // ------------------------------------
    Shader ourShader("resources/shaders/verShader.vs",
                     "resources/shaders/fragShader.fs");
    Shader poklonShader("resources/shaders/poklon.vs",
                        "resources/shaders/poklon.fs");
    Shader skyboxShader("resources/shaders/skybox.vs",
                        "resources/shaders/skybox.fs");


    //ucitavanje modela
    Model mainOstrvo(FileSystem::getPath("resources/objects/Ostrvo/plain.obj"));
    mainOstrvo.SetShaderTextureNamePrefix("material.");
    Model levoOstrvo(FileSystem::getPath("resources/objects/Ostrvo/untitled2.obj"));
    levoOstrvo.SetShaderTextureNamePrefix("material.");
    Model desnoOstrvo(FileSystem::getPath("resources/objects/Ostrvo/untitled2.obj"));
    desnoOstrvo.SetShaderTextureNamePrefix("material.");
    Model dalekoOstrvo(FileSystem::getPath("resources/objects/Ostrvo/plain.obj"));
    dalekoOstrvo.SetShaderTextureNamePrefix("material.");
    Model snezana(FileSystem::getPath("resources/objects/SneskoGorl/sneska.obj"));
    snezana.SetShaderTextureNamePrefix("material.");
    Model sanke(FileSystem::getPath("resources/objects/Sanke/sanke.obj"));
    sanke.SetShaderTextureNamePrefix("material.");
    Model jelka(FileSystem::getPath("resources/objects/Jelka2/jelka.obj"));
    jelka.SetShaderTextureNamePrefix("material.");
    Model putokaz(FileSystem::getPath("resources/objects/Putokaz/poll.obj"));
    putokaz.SetShaderTextureNamePrefix("material.");
    Model pingvin(FileSystem::getPath("resources/objects/Pingvin/penguin/pingvin.obj"));
    pingvin.SetShaderTextureNamePrefix("material.");
    Model dedaMraz(FileSystem::getPath("resources/objects/Santa/dedamraz.obj"));
    dedaMraz.SetShaderTextureNamePrefix("material.");


    PointLight pointLight;
    pointLight.ambient = glm::vec3(1.2, 1.0, 1.0);
    pointLight.diffuse = glm::vec3(0.6, 0.5, 0.6);
    pointLight.specular = glm::vec3(1.0, 1.0, 1.0);
    pointLight.constant = 1.0f;
    pointLight.linear = 0.09f;
    pointLight.quadratic = 0.032f;
    pointLight.position = glm::vec3(4.0, 4.0, 4.0);




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

    unsigned int VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    unsigned int skyboxVAO, skyboxVBO;
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);


    unsigned int diffuseMap = loadTexture(FileSystem::getPath("resources/textures/gift.jpg").c_str());

    vector<std::string> faces
    {
                    FileSystem::getPath("resources/textures/skybox/cottoncandy/cottoncandy_rt.png"),
                    FileSystem::getPath("resources/textures/skybox/cottoncandy/cottoncandy_lf.png"),
                    FileSystem::getPath("resources/textures/skybox/cottoncandy/cottoncandy_up.png"),
                    FileSystem::getPath("resources/textures/skybox/cottoncandy/cottoncandy_dn.png"),
                    FileSystem::getPath("resources/textures/skybox/cottoncandy/cottoncandy_ft.png"),
                    FileSystem::getPath("resources/textures/skybox/cottoncandy/cottoncandy_bk.png")

    };

    unsigned int cubemapTexture = loadCubemap(faces);

    //ourShader.use();

    poklonShader.use();
    poklonShader.setInt("material.diffuse",0);

    skyboxShader.use();
    skyboxShader.setInt("skybox",0);


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

        // render
        // ------
        glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
        //glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


        // bind textures on corresponding texture units


        // activate shader
        ourShader.use();

        pointLight.position = glm::vec3(4.0f, 4.0f, 4.0f);
        ourShader.setVec3("pointLight.position", pointLight.position);
        ourShader.setVec3("pointLight.ambient", pointLight.ambient);
        ourShader.setVec3("pointLight.diffuse", pointLight.diffuse);
        ourShader.setVec3("pointLight.specular", pointLight.specular);
        ourShader.setFloat("pointLight.constant", pointLight.constant);
        ourShader.setFloat("pointLight.linear", pointLight.linear);
        ourShader.setFloat("pointLight.quadratic", pointLight.quadratic);
        ourShader.setVec3("viewPosition", camera.Position);
        ourShader.setFloat("material.shininess", 64.0f);

        // pass projection matrix to shader (note that in this case it could change every frame)
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        ourShader.setMat4("projection", projection);

        // camera/view transformation
        glm::mat4 view = camera.GetViewMatrix();
        ourShader.setMat4("view", view);

        //renderovanje modela
        // main ostrvo
        glm::mat4 model = glm::mat4(1.0f);
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


        //deda
        /*model = glm::mat4(1.0f);
        model = glm::translate(model,glm::vec3(-5.0f,-1.55f,-3.0f));
        model = glm::scale(model, glm::vec3(0.005f, 0.005f, 0.005f));
        ourShader.setMat4("model", model);
        dedaMraz.Draw(ourShader);*/



        //renderovanje poklona
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
        glBindTexture(GL_TEXTURE_2D, diffuseMap);
        model = glm::mat4(1.0f);
        model = glm::translate(model,glm::vec3(-1.0f,-0.5f,0.3f));
        model = glm::scale(model,glm::vec3(0.4f,0.4f,0.4f));
        model = glm::rotate(model,glm::radians((float)-65),glm::vec3(0.0f,1.0f,0.0f));
        poklonShader.setMat4("model",model);
        glDrawArrays(GL_TRIANGLES,0,36);


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

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
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