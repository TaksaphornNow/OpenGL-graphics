#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb_image.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/filesystem.h>
#include <learnopengl/shader_m.h>
#include <learnopengl/camera.h>

#include <iostream>
#include <vector>
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);
unsigned int loadTexture(const char *path);

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// camera
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// lighting
glm::vec3 lightPos(1.2f, 1.0f, 2.0f);

struct Mesh {
    unsigned int vao = 0, vbo = 0, ebo = 0;
    int indexCount = 0;
};

static Mesh CreateCylinderMesh(int segments = 64) { // Make Cylinder
    Mesh m;
    std::vector<float> V; // pos(3) + normal(3) + uv(2) = 8 floats
    std::vector<unsigned int> I;

    auto push = [&](float x, float y, float z, float nx, float ny, float nz, float u, float v) {
        V.push_back(x); V.push_back(y); V.push_back(z);
        V.push_back(nx); V.push_back(ny); V.push_back(nz);
        V.push_back(u); V.push_back(v);
        };

    const float PI = 3.14159265358979323846f;
    const float halfZ = 0.5f;

    // TOP CAP
    int topCenter = (int)(V.size() / 8);
    push(0, 0, halfZ, 0, 0, 1, 0.5f, 0.5f); // center
    int topRingStart = (int)(V.size() / 8);
    for (int i = 0; i < segments; i++) {
        float a = (float)i / segments * 2 * PI;
        float x = cos(a), y = sin(a);
        push(x, y, halfZ, 0, 0, 1, x * 0.5f + 0.5f, y * 0.5f + 0.5f); // ring
    }
    for (int i = 0; i < segments; i++) {
        I.push_back(topCenter);
        I.push_back(topRingStart + i);
        I.push_back(topRingStart + ((i + 1) % segments));
    }

    // BOTTOM CAP
    int bottomCenter = (int)(V.size() / 8);
    push(0, 0, -halfZ, 0, 0, -1, 0.5f, 0.5f);
    int bottomRingStart = (int)(V.size() / 8);
    for (int i = 0; i < segments; i++) {
        float a = (float)i / segments * 2 * PI;
        float x = cos(a), y = sin(a);
        push(x, y, -halfZ, 0, 0, -1, x * 0.5f + 0.5f, y * 0.5f + 0.5f);
    }
    for (int i = 0; i < segments; i++) {
        I.push_back(bottomCenter);
        I.push_back(bottomRingStart + ((i + 1) % segments));
        I.push_back(bottomRingStart + i);
    }

    // SIDE
    int sideStart = (int)(V.size() / 8);
    for (int i = 0; i <= segments; i++) {
        float a = (float)i / segments * 2 * PI;
        float x = cos(a), y = sin(a);
        float u = (float)i / segments;
        // top row
        push(x, y, halfZ, x, y, 0, u, 1.0f);
        // bottom row
        push(x, y, -halfZ, x, y, 0, u, 0.0f);
    }
    for (int i = 0; i < segments; i++) {
        int t0 = sideStart + i * 2;
        int b0 = t0 + 1;
        int t1 = sideStart + (i + 1) * 2;
        int b1 = t1 + 1;
        I.push_back(t0); I.push_back(b0); I.push_back(t1);
        I.push_back(t1); I.push_back(b0); I.push_back(b1);
    }

    glGenVertexArrays(1, &m.vao);
    glGenBuffers(1, &m.vbo);
    glGenBuffers(1, &m.ebo);

    glBindVertexArray(m.vao);
    glBindBuffer(GL_ARRAY_BUFFER, m.vbo);
    glBufferData(GL_ARRAY_BUFFER, V.size() * sizeof(float), V.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, I.size() * sizeof(unsigned int), I.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    m.indexCount = (int)I.size();
    glBindVertexArray(0);
    return m;
}

// GEAR PARAMS
int   N1 = 18;      // teeth count gear 1
int   N2 = 28;      // teeth count gear 2
int   N3 = 22;      // teeth count gear 3
int   N4 = 7;      // teeth count gear 4
int   N5 = 11;      // teeth count gear 5
int   N6 = 8;      // teeth count gear 6
int   N7 = 11;      // teeth count gear 7

float R1 = 1.2f;    // pitch radius gear 1
float R2 = 1.9f;    // pitch radius gear 2
float R3 = 1.5f;    // pitch radius gear 3
float R4 = 0.5f;    // pitch radius gear 4
float R5 = 0.7f;    // pitch radius gear 5
float R6 = 0.5f;    // pitch radius gear 6
float R7 = 0.7f;    // pitch radius gear 7

float toothLen = 0.25f;   // radial
float toothHeight = 0.20f;   // Y
float thickness = 0.20f;   // Z

void DrawGearHub(Shader& shader, const Mesh& cyl,
    const glm::mat4& parent, glm::vec3 center,
    float radius, float thick)
{
    glm::mat4 model = parent;
    model = glm::translate(model, center);
    model = glm::scale(model, glm::vec3(radius, radius, thick));
    shader.setMat4("model", model);

    glBindVertexArray(cyl.vao);
    glDrawElements(GL_TRIANGLES, cyl.indexCount, GL_UNSIGNED_INT, 0);
}

void DrawGearTeeth(Shader& shader, unsigned int vao,
    const glm::mat4& parent, glm::vec3 center, int N,
    float baseRadius, float toothLen, float toothHeight,
    float thick, float angleRad)
{
    glBindVertexArray(vao);
    for (int i = 0; i < N; ++i) {
        float a = angleRad + (float)i * (2.0f * glm::pi<float>() / (float)N);
        glm::mat4 model = parent;
        model = glm::translate(model, center);
        model = glm::rotate(model, a, glm::vec3(0, 0, 1));
        float offset = baseRadius + toothLen * 0.5f; // ???????????????
        model = glm::translate(model, glm::vec3(offset, 0, 0));
        model = glm::scale(model, glm::vec3(toothLen, toothHeight, thick));
        shader.setMat4("model", model);
        glDrawArrays(GL_TRIANGLES, 0, 36);
    }
}

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
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
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

    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);

    // build and compile our shader zprogram
    // ------------------------------------
    Shader lightingShader("6.multiple_lights.vs", "6.multiple_lights.fs");
    Shader lightCubeShader("6.light_cube.vs", "6.light_cube.fs");

    // set up vertex data (and buffer(s)) and configure vertex attributes
    // ------------------------------------------------------------------
    float vertices[] = {
        // positions          // normals           // texture coords
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
    // positions all containers
    /*glm::vec3 cubePositions[] = {
        glm::vec3( 0.0f,  0.0f,  0.0f),
        glm::vec3( 2.0f,  5.0f, -15.0f),
        glm::vec3(-1.5f, -2.2f, -2.5f),
        glm::vec3(-3.8f, -2.0f, -12.3f),
        glm::vec3( 2.4f, -0.4f, -3.5f),
        glm::vec3(-1.7f,  3.0f, -7.5f),
        glm::vec3( 1.3f, -2.0f, -2.5f),
        glm::vec3( 1.5f,  2.0f, -2.5f),
        glm::vec3( 1.5f,  0.2f, -1.5f),
        glm::vec3(-1.3f,  1.0f, -1.5f)
    };*/
    // positions of the point lights
    glm::vec3 pointLightPositions[] = {
        glm::vec3( 3.0f,  3.0f,  0.0f),
        glm::vec3( -6.0f, 3.0f, 0.0f),
        glm::vec3(3.0f,  -3.0f, 0.0f),
        glm::vec3( -6.0f,  -3.0f, 0.0f)
    };
    // first, configure the cube's VAO (and VBO)
    unsigned int VBO, cubeVAO;
    glGenVertexArrays(1, &cubeVAO);
    glGenBuffers(1, &VBO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindVertexArray(cubeVAO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    // second, configure the light's VAO (VBO stays the same; the vertices are the same for the light object which is also a 3D cube)
    unsigned int lightCubeVAO;
    glGenVertexArrays(1, &lightCubeVAO);
    glBindVertexArray(lightCubeVAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    // note that we update the lamp's position attribute's stride to reflect the updated buffer data
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Mesh
    Mesh hubMesh = CreateCylinderMesh(64); // config segments more smooth (32–96)

    // load textures (we now use a utility function to keep the code more organized)
    // -----------------------------------------------------------------------------
    unsigned int diffuseMap = loadTexture(FileSystem::getPath("resources/textures/oxidized-coppper-roughness.png").c_str());
    unsigned int specularMap = loadTexture(FileSystem::getPath("resources/textures/oxidized-copper-albedo.png").c_str());

    // shader configuration
    // --------------------
    lightingShader.use();
    lightingShader.setInt("material.diffuse", 0);
    lightingShader.setInt("material.specular", 1);

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);   // ???? VSync ????????????

    const unsigned int NR_STARS = 50; // match shader
    glm::vec3 starPositions[NR_STARS];
    glm::vec3 starColors[NR_STARS];

    for (unsigned int i = 0; i < NR_STARS; i++) {
        float rad = 30.0f; // radius of sphere (distance from center/camera)
    
        float theta = glm::radians((float)(rand() % 360)); // 0 … 360°
        float phi   = glm::radians((float)(rand() % 180)); // 0 … 180°
    
        float x = (float)rad * sin(phi) * cos(theta);
        float y = (float)rad * cos(phi);
        float z = (float)rad * sin(phi) * sin(theta);
    
        starPositions[i] = glm::vec3(x, y, z);

        // random star color but still bright
        float hue = (rand() % 360) / 360.0f; // 0.0 – 1.0
        float s = 0.8f;  // saturation
        float v = 1.0f;  // brightness

        // Convert HSV ? RGB
        float c = v * s;
        float d = c * (1 - fabs(fmod(hue * 6.0f, 2.0f) - 1));
        float m = v - c;

        float r, g, b;
        if (hue < 1.0 / 6.0) { r = c; g = d; b = 0; }
        else if (hue < 2.0 / 6.0) { r = d; g = c; b = 0; }
        else if (hue < 3.0 / 6.0) { r = 0; g = c; b = d; }
        else if (hue < 4.0 / 6.0) { r = 0; g = d; b = c; }
        else if (hue < 5.0 / 6.0) { r = d; g = 0; b = c; }
        else { r = c; g = 0; b =d; }

        starColors[i] = glm::vec3(r + m, g + m, b + m);

    }

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window))
    {
        // per-frame time logic
        // --------------------
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        // -----
        processInput(window);

        // render
        // ------
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // be sure to activate shader when setting uniforms/drawing objects
        lightingShader.use();
        lightingShader.setVec3("viewPos", camera.Position);
        lightingShader.setFloat("material.shininess", 32.0f);

        /*
           Here we set all the uniforms for the 5/6 types of lights we have. We have to set them manually and index 
           the proper PointLight struct in the array to set each uniform variable. This can be done more code-friendly
           by defining light types as classes and set their values in there, or by using a more efficient uniform approach
           by using 'Uniform buffer objects', but that is something we'll discuss in the 'Advanced GLSL' tutorial.
        */
        // directional light
        /*lightingShader.setVec3("dirLight.direction", -0.2f, -1.0f, -0.3f);
        lightingShader.setVec3("dirLight.ambient", 0.05f, 0.05f, 0.05f);
        lightingShader.setVec3("dirLight.diffuse", 0.4f, 0.4f, 0.4f);
        lightingShader.setVec3("dirLight.specular", 0.5f, 0.5f, 0.5f);*/
        // Cool 
        /*lightingShader.setVec3("dirLight.direction", -0.5f, -1.0f, -0.5f);
        lightingShader.setVec3("dirLight.ambient", 0.1f, 0.1f, 0.2f);
        lightingShader.setVec3("dirLight.diffuse", 0.3f, 0.3f, 0.8f);
        lightingShader.setVec3("dirLight.specular", 0.5f, 0.5f, 1.0f);*/
        // Warm 
        lightingShader.setVec3("dirLight.direction", -0.3f, -1.0f, -0.1f);  
        lightingShader.setVec3("dirLight.ambient", 0.3f, 0.25f, 0.2f);     
        lightingShader.setVec3("dirLight.diffuse", 0.9f, 0.85f, 0.7f);    
        lightingShader.setVec3("dirLight.specular", 1.0f, 0.95f, 0.8f);


        // point light 1
        lightingShader.setVec3("pointLights[0].position", pointLightPositions[0]);
        lightingShader.setVec3("pointLights[0].ambient", 0.05f, 0.05f, 0.05f);
        lightingShader.setVec3("pointLights[0].diffuse", 0.8f, 0.8f, 0.8f);
        lightingShader.setVec3("pointLights[0].specular", 1.0f, 1.0f, 1.0f);
        lightingShader.setFloat("pointLights[0].constant", 1.0f);
        lightingShader.setFloat("pointLights[0].linear", 0.09f);
        lightingShader.setFloat("pointLights[0].quadratic", 0.032f);
        // point light 2
        lightingShader.setVec3("pointLights[1].position", pointLightPositions[1]);
        lightingShader.setVec3("pointLights[1].ambient", 0.05f, 0.05f, 0.05f);
        lightingShader.setVec3("pointLights[1].diffuse", 0.8f, 0.8f, 0.8f);
        lightingShader.setVec3("pointLights[1].specular", 1.0f, 1.0f, 1.0f);
        lightingShader.setFloat("pointLights[1].constant", 1.0f);
        lightingShader.setFloat("pointLights[1].linear", 0.09f);
        lightingShader.setFloat("pointLights[1].quadratic", 0.032f);
        // point light 3
        lightingShader.setVec3("pointLights[2].position", pointLightPositions[2]);
        lightingShader.setVec3("pointLights[2].ambient", 0.05f, 0.05f, 0.05f);
        lightingShader.setVec3("pointLights[2].diffuse", 0.8f, 0.8f, 0.8f);
        lightingShader.setVec3("pointLights[2].specular", 1.0f, 1.0f, 1.0f);
        lightingShader.setFloat("pointLights[2].constant", 1.0f);
        lightingShader.setFloat("pointLights[2].linear", 0.09f);
        lightingShader.setFloat("pointLights[2].quadratic", 0.032f);
        // point light 4
        lightingShader.setVec3("pointLights[3].position", pointLightPositions[3]);
        lightingShader.setVec3("pointLights[3].ambient", 0.05f, 0.05f, 0.05f);
        lightingShader.setVec3("pointLights[3].diffuse", 0.8f, 0.8f, 0.8f);
        lightingShader.setVec3("pointLights[3].specular", 1.0f, 1.0f, 1.0f);
        lightingShader.setFloat("pointLights[3].constant", 1.0f);
        lightingShader.setFloat("pointLights[3].linear", 0.09f);
        lightingShader.setFloat("pointLights[3].quadratic", 0.032f);
        
        // spotlight
        lightingShader.setVec3("spotLight.position", camera.Position);
        lightingShader.setVec3("spotLight.direction", camera.Front);
        lightingShader.setVec3("spotLight.ambient", 0.2f, 0.2f, 0.2f);
        lightingShader.setVec3("spotLight.diffuse", 1.5f, 1.5f, 1.5f);
        lightingShader.setVec3("spotLight.specular", 5.0f, 5.0f, 5.0f);
        lightingShader.setFloat("spotLight.constant", 1.0f);
        lightingShader.setFloat("spotLight.linear", 0.02f);
        lightingShader.setFloat("spotLight.quadratic", 0.001f);
        lightingShader.setFloat("spotLight.cutOff", glm::cos(glm::radians(5.0f)));
        lightingShader.setFloat("spotLight.outerCutOff", glm::cos(glm::radians(10.0f)));


        // view/projection transformations
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();
        lightingShader.setMat4("projection", projection);
        lightingShader.setMat4("view", view);

        // world transformation
        glm::mat4 model = glm::mat4(1.0f);
        lightingShader.setMat4("model", model);

        // bind diffuse map
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, diffuseMap);
        // bind specular map
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, specularMap);

        glBindVertexArray(cubeVAO);

        glm::mat4 I(1.0f);

        // position of gear (x,y,z)
        glm::vec3 G1 = glm::vec3(- (R1 + R2 + 0.25)* 0.5f, 0.0f, 0.05f);
        glm::vec3 G2 = glm::vec3((R1 + R2 + 0.25) * 0.5f, 0.0f, 0.05f);
        glm::vec3 G3 = glm::vec3(- (R1 + R2 + R3), 0.5f, 0.05f);
        glm::vec3 G4 = glm::vec3(-((R3 * 2.0f)+R1), ((R3 * 2.0f) - 0.25f), 0.05f);
        glm::vec3 G5 = glm::vec3((R1+R2) * 0.5f, (R1 * 2.0f)+0.5f, 0.05f);
        glm::vec3 G6 = glm::vec3(-((R3 * 2.0f) + R1 + 0.25f ), (-(R3 * 1.0f +0.25f)), 0.05f);
        glm::vec3 G7 = glm::vec3((R1 + R2) * 0.5f, -((R1 * 2.0f) + 0.5f), 0.05f);
     
        const double TWO_PI = 6.283185307179586;
        double phase2 = glm::pi<double>() / double(N2); // half of tooth gear
        double phase5 = glm::pi<double>() / double(N5); 
        double phase7 = glm::pi<double>() / double(N7); 
        
        // spin velocity
        const double t = glfwGetTime();
        const double omega1 = 0.5; // rad/s
        double ang1 = omega1 * t;
        double ang2 = -(omega1 * ((float)N1 / (float)N2)) * t;   // spin another way
        double ang3 = (- (omega1 * ((float)N1 / (float)N3)) * t ) + phase2;
        double ang4 = (omega1 * ((float)N1 / (float)N4)) * t ;
        double ang5 = (omega1 * ((float)N1 / (float)N5)) * t + phase5;
        double ang6 = (omega1 * ((float)N1 / (float)N6)) * t;
        double ang7 = (omega1 * ((float)N1 / (float)N7)) * t + phase7;

        // prevent oversize
        ang1 = std::fmod(ang1, TWO_PI);
        ang2 = std::fmod(ang2, TWO_PI);
        ang3 = std::fmod(ang3, TWO_PI);
        ang4 = std::fmod(ang4, TWO_PI);
        ang5 = std::fmod(ang5, TWO_PI);
        ang6 = std::fmod(ang6, TWO_PI);
        ang7 = std::fmod(ang7, TWO_PI);

        
        DrawGearHub(lightingShader, hubMesh, I, G1, R1, thickness);
        DrawGearTeeth(lightingShader, cubeVAO, I, G1, N1, R1, toothLen, toothHeight, thickness, (float)ang1);

        DrawGearHub(lightingShader, hubMesh, I, G2, R2, thickness);
        DrawGearTeeth(lightingShader, cubeVAO, I, G2, N2, R2, toothLen, toothHeight, thickness, (float)(ang2 + phase2));

        DrawGearHub(lightingShader, hubMesh, I, G3, R3, thickness);
        DrawGearTeeth(lightingShader, cubeVAO, I, G3, N3, R3, toothLen, toothHeight, thickness, (float)ang3);

        DrawGearHub(lightingShader, hubMesh, I, G4, R4, thickness);
        DrawGearTeeth(lightingShader, cubeVAO, I, G4, N4, R4, toothLen, toothHeight, thickness, (float)ang4);

        DrawGearHub(lightingShader, hubMesh, I, G5, R5, thickness);
        DrawGearTeeth(lightingShader, cubeVAO, I, G5, N5, R5, toothLen, toothHeight, thickness, (float)ang5);

        DrawGearHub(lightingShader, hubMesh, I, G6, R6, thickness);
        DrawGearTeeth(lightingShader, cubeVAO, I, G6, N6, R6, toothLen, toothHeight, thickness, (float)ang6);

        DrawGearHub(lightingShader, hubMesh, I, G7, R7, thickness);
        DrawGearTeeth(lightingShader, cubeVAO, I, G7, N7, R7, toothLen, toothHeight, thickness, (float)ang7);


         // also draw the lamp object(s)
         lightCubeShader.use();
         lightCubeShader.setMat4("projection", projection);
         lightCubeShader.setMat4("view", view);
    
         // we now draw as many light bulbs as we have point lights.
         glBindVertexArray(lightCubeVAO);
         for (unsigned int i = 0; i < 4; i++)
         {
             model = glm::mat4(1.0f);
             model = glm::translate(model, pointLightPositions[i]);
             model = glm::scale(model, glm::vec3(0.1f, 0.1f, 0.3f)); // cylinder slender shape
             lightCubeShader.setMat4("model", model);

             glBindVertexArray(hubMesh.vao);
             glDrawElements(GL_TRIANGLES, hubMesh.indexCount, GL_UNSIGNED_INT, 0);
         }
         
         for (unsigned int i = 0; i < NR_STARS; i++) {
             model = glm::mat4(1.0f);
             model = glm::translate(model, starPositions[i]);
             model = glm::scale(model, glm::vec3(0.07f)); // very tiny dot
             lightCubeShader.setMat4("model", model);

             glBindVertexArray(lightCubeVAO);
             glDrawArrays(GL_TRIANGLES, 0, 36);
         }


        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // optional: de-allocate all resources once they've outlived their purpose:
    // ------------------------------------------------------------------------
    glDeleteVertexArrays(1, &cubeVAO);
    glDeleteVertexArrays(1, &lightCubeVAO);
    glDeleteBuffers(1, &VBO);
    glDeleteVertexArrays(1, &hubMesh.vao);
    glDeleteBuffers(1, &hubMesh.vbo);
    glDeleteBuffers(1, &hubMesh.ebo);


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
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

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

    camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}

// utility function for loading a 2D texture from file
// ---------------------------------------------------
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
