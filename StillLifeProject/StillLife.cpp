#include <iostream>         // cout, cerr
#include <cstdlib>          // EXIT_FAILURE
#include <GL/glew.h>        // GLEW library
#include <GLFW/glfw3.h>     // GLFW library
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>      // Image loading Utility functions

// GLM Math Header inclusions
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnOpengl/camera.h> // Camera class
#include <learnOpengl/Sphere.h> 


using namespace std; // Standard namespace

/*Shader program Macro*/
#ifndef GLSL
#define GLSL(Version, Source) "#version " #Version " core \n" #Source
#endif

// Unnamed namespace
namespace
{
    const char* const WINDOW_TITLE = "Still Life Project"; // Macro for window title

    // Variables for window width and height
    const int WINDOW_WIDTH = 800;
    const int WINDOW_HEIGHT = 600;

    // Stores the GL data relative to a given mesh
    struct GLMesh
    {
        GLuint vao;         // Handle for the vertex array object
        GLuint vbos[2];         // Handle for the vertex buffer object
        GLuint nIndices;    // Number of indices of the mesh
    };

    // Main GLFW window
    GLFWwindow* gWindow = nullptr;

    // Triangle mesh data
    GLMesh gMesh;  // bowl base
    GLMesh gMesh2; // bowl
    GLMesh gMesh3; // vase base
    GLMesh gMesh4; // plane
    GLMesh gMesh5; // vase base
    GLMesh gMesh6; // vase stem
    GLMesh gMesh7; // vase mouth
    GLMesh gMesh8; // ramekin lip

    // Textures
    GLuint gTextureId; // mortar (2)
    GLuint gTextureId2; // wood
    GLuint gTextureId3; // glass
    GLuint gTextureId4; // porcelain
    GLuint gTextureId5; // detail

    glm::vec2 gUVScale(5.0f, 5.0f);
    GLint gTexWrapMode = GL_REPEAT;

    // Shader programs
    GLuint gProgramId;
    GLuint gLampProgramId;  // LAMP ONE
    GLuint gLampProgramId2; // LAMP TWO
    GLuint gLampProgramId3; // LAMP THREE

    // camera
    Camera gCamera(glm::vec3(0.0f, 0.0f, 3.0f));
    float gLastX = WINDOW_WIDTH / 2.0f;
    float gLastY = WINDOW_HEIGHT / 2.0f;
    bool gFirstMouse = true;
    bool Is3D = true; // variable to set 2D or 3D

    // timing
    float gDeltaTime = 0.0f; // time between current frame and last frame
    float gLastFrame = 0.0f;
    float modifier = 0.002f; // variable for spinning

    // Object Color
    glm::vec3 gObjectColor(1.f,1.0f, 1.0f);

    // Light Positions
    glm::vec3 gLightPosition(0.0f,0.0f,4.0f); // ambien
    glm::vec3 gLightPosition2(2.0f,1.0f,-4.0f); // diffuse
    glm::vec3 gLightPosition3(-2.0f,0.0f,-5.0f); // spectral

    // Light Colors
    glm::vec3 gLightColor(1.0f,0.9f,0.9f);
    glm::vec3 gLightColor2(0.7f,0.0f,0.0f);
    glm::vec3 gLightColor3(1.0f,1.0f,1.0f);

}

/* User-defined Function prototypes to:
 * initialize the program, set the window size,
 * redraw graphics on the window when resized,
 * and render graphics on the screen
 */
bool UInitialize(int, char* [], GLFWwindow** window);
void UResizeWindow(GLFWwindow* window, int width, int height);
void UProcessInput(GLFWwindow* window);
void UMousePositionCallback(GLFWwindow* window, double xpos, double ypos);
void UMouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
void UCreateMesh(GLMesh& mesh, GLMesh& mesh2, GLMesh& mesh3, GLMesh& mesh4, GLMesh& mesh5, GLMesh& mesh6, GLMesh& mesh7, GLMesh& mesh8);
void UDestroyMesh(GLMesh& mesh, GLMesh& mesh2, GLMesh& mesh3, GLMesh& mesh4, GLMesh& mesh5, GLMesh& mesh6, GLMesh& mesh7, GLMesh& mesh8);
bool UCreateTexture(const char* filename, GLuint& textureId, char wrapType);
void UDestroyTexture(GLuint textureId);
void URender2D();
void URender3D();
bool UCreateShaderProgram(const char* vtxShaderSource, const char* fragShaderSource, GLuint& programId);
void UDestroyShaderProgram(GLuint programId);

/* Vertex Shader Source Code*/
const GLchar* vertexShaderSource = GLSL(440,
    layout(location = 0) in vec3 position; // (location 1 omitted b/c vert color not used)
    layout(location = 1) in vec3 normal; // VAP position 1 for normals
    layout(location = 2) in vec2 textureCoordinate;

    out vec3 vertexNormal; // For outgoing normals to fragment shader
    out vec3 vertexFragmentPos; // For outgoing color / pixels to fragment shader
    out vec2 vertexTextureCoordinate;

    //Global variables for the  transform matrices
    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;

    void main()
    {
        gl_Position = projection * view * model * vec4(position, 1.0f); // transforms vertices to clip coordinates
        vertexFragmentPos = vec3(model * vec4(position, 1.0f)); // Gets fragment / pixel position in world space only (exclude view and projection)
        vertexNormal = mat3(transpose(inverse(model))) * normal; // get normal vectors in world space only and exclude normal translation properties
        vertexTextureCoordinate = textureCoordinate;
    }
    );


/* Fragment Shader Source Code*/
const GLchar* fragmentShaderSource = GLSL(440,

    in vec3 vertexNormal; // For incoming normals
    in vec3 vertexFragmentPos; // For incoming fragment position
    in vec2 vertexTextureCoordinate;

    out vec4 fragmentColor;

    // Uniform / Global variables for object color, light color, light position, and camera/view position
    uniform vec3 objectColor;
    uniform vec3 viewPosition;
    // LAMP ONE (ambient)
    uniform vec3 lightColor; 
    uniform vec3 lightPos; 
    // LAMP TWO (diffuse)
    uniform vec3 lightColor2;
    uniform vec3 lightPos2;
    // LAMP THREE (spectral)
    uniform vec3 lightColor3;
    uniform vec3 lightPos3;

    uniform sampler2D uTexture;
    uniform vec2 uvScale;

    void main() /*Phong lighting model calculations to generate ambient, diffuse, and specular components*/
    {
        // LAMP ONE 
        //Calculate Ambient lighting
        float ambientStrength =.30f; // Set ambient or global lighting strength
        vec3 ambient = ambientStrength * lightColor; // Generate ambient light color
        vec3 lightDirection = normalize(lightPos - vertexFragmentPos); // Calculate distance (light direction) between light source and fragments/pixels on cube

        // LAMP TWO
        //Calculate Diffuse lighting
        vec3 norm = normalize(vertexNormal); // Normalize vectors to 1 unit
        vec3 lightDirection2 = normalize(lightPos2 - vertexFragmentPos); // Calculate distance (light direction) between light source and fragments/pixels on cube
        float impact = max(dot(norm, lightDirection2), 0.0);// Calculate diffuse impact by generating dot product of normal and light
        vec3 diffuse = impact * lightColor2+ .35; // Generate diffuse light color

        // LAMP THREE
        //Calculate Specular lighting
        float specularIntensity = 0.6f; // Set specular light strength
        float highlightSize = 6.0f; // Set specular highlight size
        vec3 lightDirection3 = normalize(lightPos3 - vertexFragmentPos); // Calculate distance (light direction) between light source and fragments/pixels on cube
        vec3 viewDir = normalize(viewPosition - vertexFragmentPos); // Calculate view direction
        vec3 reflectDir = reflect(-lightDirection3, norm);// Calculate reflection vector
        //Calculate specular component
        float specularComponent = pow(max(dot(viewDir, reflectDir), 0.0), highlightSize);
        vec3 specular = specularIntensity * specularComponent * lightColor3;

       // Texture holds the color to be used for all three components
        vec4 textureColor = texture(uTexture, vertexTextureCoordinate * uvScale);

        // Calculate phong result
        vec3 phong = ((ambient+diffuse) + (diffuse) + (specular) )* textureColor.xyz;

        fragmentColor = vec4(phong, 1.0); // Send lighting results to GPU
    }
    );


/* Lamp Shader Source Code*/
const GLchar* lampVertexShaderSource = GLSL(440,

    layout(location = 0) in vec3 position; // VAP position 0 for vertex position data

    //Uniform / Global variables for the  transform matrices
    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;

    void main()
    {
        gl_Position = projection * view * model * vec4(position, 1.0f); // Transforms vertices into clip coordinates
    }
    );

/* Fragment Shader Source Code*/
const GLchar* lampFragmentShaderSource = GLSL(440,

    out vec4 fragmentColor; // For outgoing lamp color to the GPU

    void main()
    {
        fragmentColor = vec4(1.0f); // Set color to white (1.0f,1.0f,1.0f) with alpha 1.0
    }
    );

    // Images are loaded with Y axis going down, but OpenGL's Y axis goes up, so let's flip it
    void flipImageVertically(unsigned char* image, int width, int height, int channels)
    {
        for (int j = 0; j < height / 2; ++j)
        {
            int index1 = j * width * channels;
            int index2 = (height - 1 - j) * width * channels;

            for (int i = width * channels; i > 0; --i)
            {
                unsigned char tmp = image[index1];
                image[index1] = image[index2];
                image[index2] = tmp;
                ++index1;
                ++index2;
            }
        }
    }


int main(int argc, char* argv[])
{
    if (!UInitialize(argc, argv, &gWindow))
        return EXIT_FAILURE;

    // Create the mesh
    UCreateMesh(gMesh,gMesh2,gMesh3,gMesh4,gMesh5,gMesh6, gMesh7, gMesh8); // Calls the function to create the Vertex Buffer Object

    // Create the shader program
    if (!UCreateShaderProgram(vertexShaderSource, fragmentShaderSource, gProgramId)) 
        return EXIT_FAILURE;

    // Sets the background color of the window to black (it will be implicitely used by glClear)
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    // render loop
    // -----------
    while (!glfwWindowShouldClose(gWindow))
    {
        // per-frame timing
        // --------------------
        float currentFrame = glfwGetTime();
        gDeltaTime = currentFrame - gLastFrame;
        gLastFrame = currentFrame;

        // input
        // -----
        UProcessInput(gWindow);
        glfwPollEvents();
        glfwWaitEvents();


    }

    // Release mesh data
    UDestroyMesh(gMesh,gMesh2,gMesh3,gMesh4,gMesh5, gMesh6, gMesh7, gMesh8);

    // Release texture
    UDestroyTexture(gTextureId);
    UDestroyTexture(gTextureId2);
    UDestroyTexture(gTextureId3);
    UDestroyTexture(gTextureId4);
    UDestroyTexture(gTextureId5);

    // Release shader program
    UDestroyShaderProgram(gProgramId);

    exit(EXIT_SUCCESS); // Terminates the program successfully
}

// Initialize GLFW, GLEW, and create a window
bool UInitialize(int argc, char* argv[], GLFWwindow** window)
{
    // GLFW: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // GLFW: window creation
    // ---------------------
    * window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE, NULL, NULL);
    if (*window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }
    glfwMakeContextCurrent(*window);
    glfwSetFramebufferSizeCallback(*window, UResizeWindow);
    glfwSetCursorPosCallback(*window, UMousePositionCallback);
    glfwSetScrollCallback(*window, UMouseScrollCallback);

    // tell GLFW to capture our mouse
    glfwSetInputMode(*window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // GLEW: initialize
    // ----------------
    // Note: if using GLEW version 1.13 or earlier
    glewExperimental = GL_TRUE;
    GLenum GlewInitResult = glewInit();

    if (GLEW_OK != GlewInitResult)
    {
        std::cerr << glewGetErrorString(GlewInitResult) << std::endl;
        return false;
    }

    // Displays GPU OpenGL version
    cout << "INFO: OpenGL Version: " << glGetString(GL_VERSION) << endl;

    return true;
}


// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
void UProcessInput(GLFWwindow* window)
{
    static const float cameraSpeed = 2.5f;

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        gCamera.ProcessKeyboard(FORWARD, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        gCamera.ProcessKeyboard(BACKWARD, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        gCamera.ProcessKeyboard(LEFT, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        gCamera.ProcessKeyboard(RIGHT, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        gCamera.ProcessKeyboard(UP, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        gCamera.ProcessKeyboard(DOWN, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS)
        Is3D = !Is3D;
    if (Is3D) {

        URender3D();

    }
    else
    {
        URender2D();
    }
}


// glfw: whenever the window size changed (by OS or user resize) this callback function executes
void UResizeWindow(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void UMousePositionCallback(GLFWwindow* window, double xpos, double ypos)
{
    if (gFirstMouse)
    {
        gLastX = xpos;
        gLastY = ypos;
        gFirstMouse = false;
    }

    float xoffset = xpos - gLastX;
    float yoffset = gLastY - ypos; // reversed since y-coordinates go from bottom to top

    gLastX = xpos;
    gLastY = ypos;

    gCamera.ProcessMouseMovement(xoffset, yoffset);
}


// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void UMouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    gCamera.ProcessMouseScroll(yoffset);
}


// Functioned called to render a frame
void URender2D()
{
    // Enable z-depth
    glEnable(GL_DEPTH_TEST);

    // Clear the frame and z buffers
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // 1. Scales the object by 0.5
    glm::mat4 scale = glm::scale(glm::vec3(0.5f, 0.5f, 0.5f));
    // 2. Rotates shape by 15 degrees in the x axis
    glm::mat4 rotation = glm::rotate(1.1f, glm::vec3(-1.0, 0.0f, 0.0f));
    // 3. Place object at the origin
    glm::mat4 translation = glm::translate(glm::vec3(.40f, 0.20f, -2.0f));
    // Model matrix: transformations are applied right-to-left order
    glm::mat4 model = translation * rotation * scale;  
    // camera/view transformation
    glm::mat4 view = gCamera.GetViewMatrix();
    // Creates a perspective projection
    glm::mat4 orthoProjection = glm::ortho(-2.0f, 2.0f, -2.0f, 2.0f, 0.1f, 100.0f);

    // Set the shader to be used
    glUseProgram(gProgramId);

    // Retrieves and passes transform matrices to the Shader program
    GLint modelLoc = glGetUniformLocation(gProgramId, "model");
    GLint viewLoc = glGetUniformLocation(gProgramId, "view");
    GLint projLoc = glGetUniformLocation(gProgramId, "projection");

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(orthoProjection));

    // Reference matrix uniforms from the Cube Shader program for the cub color, light color, light position, and camera position 
    // LAMP ONE
    GLint objectColorLoc = glGetUniformLocation(gProgramId, "objectColor");
    GLint lightColorLoc = glGetUniformLocation(gProgramId, "lightColor");
    GLint lightPositionLoc = glGetUniformLocation(gProgramId, "lightPos");
    // LAMP TWO
    GLint lightColorLoc2 = glGetUniformLocation(gProgramId, "lightColor2");
    GLint lightPositionLoc2 = glGetUniformLocation(gProgramId, "lightPos2");
    // LAMP THREE
    GLint objectColorLoc2 = glGetUniformLocation(gProgramId, "objectColor");
    GLint lightColorLoc3 = glGetUniformLocation(gProgramId, "lightColor3");
    GLint lightPositionLoc3 = glGetUniformLocation(gProgramId, "lightPos3");
    GLint viewPositionLoc = glGetUniformLocation(gProgramId, "viewPosition");

    // Pass color, light, and camera data to the Cube Shader program's corresponding uniforms
    // LAMP ONE
    glUniform3f(objectColorLoc, gObjectColor.r, gObjectColor.g, gObjectColor.b);
    glUniform3f(lightColorLoc, gLightColor.r, gLightColor.g, gLightColor.b);
    glUniform3f(lightPositionLoc, gLightPosition.x, gLightPosition.y, gLightPosition.z);
    // LAMP TWO
    glUniform3f(lightColorLoc2, gLightColor2.r, gLightColor2.g, gLightColor2.b);
    glUniform3f(lightPositionLoc2, gLightPosition2.x, gLightPosition2.y, gLightPosition2.z);
    // LAMP THREE
    glUniform3f(lightColorLoc3, gLightColor3.r, gLightColor3.g, gLightColor3.b);
    glUniform3f(lightPositionLoc3, gLightPosition3.x, gLightPosition3.y, gLightPosition3.z);

    const glm::vec3 cameraPosition = gCamera.Position;
    glUniform3f(viewPositionLoc, cameraPosition.x, cameraPosition.y, cameraPosition.z);

    GLint UVScaleLoc = glGetUniformLocation(gProgramId, "uvScale");
    glUniform2fv(UVScaleLoc, 1, glm::value_ptr(gUVScale));

    glUseProgram(gProgramId);

    // chars to specify which type of texture wrap to use in UCreateTexture
    char mirroredRepeat = 'm';
    char clampToEdge= 'c';

    // Load texture files
    const char* texFileName = "mortar.jpg";
    const char* texFileName2 = "wood.jpg";
    const char* texFileName3 = "glass.jpg";
    const char* texFileName4 = "porcelain.jpg";

    // call UCreateTexture to set textures
    UCreateTexture(texFileName,  gTextureId,  mirroredRepeat);
    UCreateTexture(texFileName2, gTextureId2, mirroredRepeat);
    UCreateTexture(texFileName3, gTextureId3, mirroredRepeat);
    UCreateTexture(texFileName4, gTextureId4, mirroredRepeat);
    UCreateTexture(texFileName2, gTextureId5, clampToEdge);


    // BIND VERTEX ARRAYS & ACTIVATE AND BIND EACH TEXTURES TO DRAW EACH SHAPE
    // gMesh
    glBindVertexArray(gMesh.vao); // Activate the VBOs
    glActiveTexture(GL_TEXTURE0);    // bind texture
    glBindTexture(GL_TEXTURE_2D, gTextureId);
    glDrawElements(GL_TRIANGLES, gMesh.nIndices, GL_UNSIGNED_SHORT, NULL);    // Draws the triangles for the bowl

    // gMesh2 
    glBindVertexArray(gMesh2.vao); // Activate the VBOS
    glBindTexture(GL_TEXTURE_2D, gTextureId);
    glDrawElements(GL_TRIANGLES, gMesh2.nIndices, GL_UNSIGNED_SHORT, NULL);    // Draws the triangles for the bowl base

    // gMesh3
    glBindVertexArray(gMesh3.vao); // Activate VBOS
    glBindTexture(GL_TEXTURE_2D, gTextureId4);
    glDrawElements(GL_TRIANGLES, gMesh3.nIndices, GL_UNSIGNED_SHORT, NULL); // Draws the triangle for the ramekin

    // gMesh4
    glBindVertexArray(gMesh4.vao); // Activate VBOS
    glBindTexture(GL_TEXTURE_2D, gTextureId3);
    glDrawElements(GL_TRIANGLES, gMesh4.nIndices, GL_UNSIGNED_SHORT, NULL); // Draws the triangle for the vase mouth

    // gMesh5
    glBindVertexArray(gMesh5.vao); // Activate VBOS
    glBindTexture(GL_TEXTURE_2D, gTextureId3);
    glDrawElements(GL_TRIANGLES, gMesh5.nIndices, GL_UNSIGNED_SHORT, NULL); // Draws the triangle for the vase stem

    // gMesh6
    glBindVertexArray(gMesh6.vao); // Activate VBOS
    glBindTexture(GL_TEXTURE_2D, gTextureId3);
    glDrawElements(GL_TRIANGLES, gMesh6.nIndices, GL_UNSIGNED_SHORT, NULL); // Draws the triangle for the vase base

    // gMesh7
    glBindVertexArray(gMesh7.vao); // Activate VBOS
    glBindTexture(GL_TEXTURE_2D, gTextureId2);
    glDrawElements(GL_TRIANGLES, gMesh7.nIndices, GL_UNSIGNED_SHORT, NULL); // Draws the triangle for the table

    // gMesh8
    glBindVertexArray(gMesh8.vao); // Activate VBOS
    glBindTexture(GL_TEXTURE_2D, gTextureId5);
    glDrawElements(GL_TRIANGLES, gMesh8.nIndices, GL_UNSIGNED_SHORT, NULL); // Draws the triangle for ramekin detail

    // Deactivate the Vertex Array Object
    glBindVertexArray(0);

    // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
    glfwSwapBuffers(gWindow);    // Flips the the back buffer with the front buffer every frame.

}



// Functioned called to render a frame
void URender3D()
{
    // Enable z-depth
    glEnable(GL_DEPTH_TEST);

    // Clear the frame and z buffers
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // 1. Scales the object by 0.5
    glm::mat4 scale = glm::scale(glm::vec3(0.5f, 0.5f, 0.5f));
    // 2. Rotates shape by 15 degrees in the x axis
    glm::mat4 rotation = glm::rotate(1.1f, glm::vec3(-1.0, 0.0f, 0.0f));
    // 3. Place object at the origin
    glm::mat4 translation = glm::translate(glm::vec3(.40f, 0.20f, -2.0f));
    // Model matrix: transformations are applied right-to-left order
    glm::mat4 model = translation * rotation * scale;
    // camera/view transformation
    glm::mat4 view = gCamera.GetViewMatrix();
    // Creates a perspective projection
    glm::mat4 projection = glm::perspective(glm::radians(gCamera.Zoom), (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);

    // Set the shader to be used
    glUseProgram(gProgramId);

    // Retrieves and passes transform matrices to the Shader program
    GLint modelLoc = glGetUniformLocation(gProgramId, "model");
    GLint viewLoc = glGetUniformLocation(gProgramId, "view");
    GLint projLoc = glGetUniformLocation(gProgramId, "projection");

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    // Reference matrix uniforms from the Cube Shader program for the cub color, light color, light position, and camera position 
    // LAMP ONE
    GLint objectColorLoc = glGetUniformLocation(gProgramId, "objectColor");
    GLint lightColorLoc = glGetUniformLocation(gProgramId, "lightColor");
    GLint lightPositionLoc = glGetUniformLocation(gProgramId, "lightPos");
    // LAMP TWO
    GLint lightColorLoc2 = glGetUniformLocation(gProgramId, "lightColor2");
    GLint lightPositionLoc2 = glGetUniformLocation(gProgramId, "lightPos2");
    // LAMP THREE
    GLint objectColorLoc2 = glGetUniformLocation(gProgramId, "objectColor");
    GLint lightColorLoc3 = glGetUniformLocation(gProgramId, "lightColor3");
    GLint lightPositionLoc3 = glGetUniformLocation(gProgramId, "lightPos3");
    GLint viewPositionLoc = glGetUniformLocation(gProgramId, "viewPosition");

    // Pass color, light, and camera data to the Cube Shader program's corresponding uniforms
    // LAMP ONE
    glUniform3f(objectColorLoc, gObjectColor.r, gObjectColor.g, gObjectColor.b);
    glUniform3f(lightColorLoc, gLightColor.r, gLightColor.g, gLightColor.b);
    glUniform3f(lightPositionLoc, gLightPosition.x, gLightPosition.y, gLightPosition.z);
    // LAMP TWO
    glUniform3f(lightColorLoc2, gLightColor2.r, gLightColor2.g, gLightColor2.b);
    glUniform3f(lightPositionLoc2, gLightPosition2.x, gLightPosition2.y, gLightPosition2.z);
    // LAMP THREE
    glUniform3f(lightColorLoc3, gLightColor3.r, gLightColor3.g, gLightColor3.b);
    glUniform3f(lightPositionLoc3, gLightPosition3.x, gLightPosition3.y, gLightPosition3.z);

    const glm::vec3 cameraPosition = gCamera.Position;
    glUniform3f(viewPositionLoc, cameraPosition.x, cameraPosition.y, cameraPosition.z);

    GLint UVScaleLoc = glGetUniformLocation(gProgramId, "uvScale");
    glUniform2fv(UVScaleLoc, 1, glm::value_ptr(gUVScale));

    glUseProgram(gProgramId);

    // chars to specify which type of texture wrap to use in UCreateTexture
    char mirroredRepeat = 'm';
    char clampToEdge = 'c';

    // Load texture files
    const char* texFileName = "mortar.jpg";
    const char* texFileName2 = "wood.jpg";
    const char* texFileName3 = "glass.jpg";
    const char* texFileName4 = "porcelain.jpg";


    // call UCreateTexture to set textures
    UCreateTexture(texFileName, gTextureId, mirroredRepeat);
    UCreateTexture(texFileName2, gTextureId2, mirroredRepeat);
    UCreateTexture(texFileName3, gTextureId3, mirroredRepeat);
    UCreateTexture(texFileName4, gTextureId4, mirroredRepeat);
    UCreateTexture(texFileName2, gTextureId5, clampToEdge);


    // BIND VERTEX ARRAYS & ACTIVATE AND BIND EACH TEXTURES TO DRAW EACH SHAPE
    // gMesh
    glBindVertexArray(gMesh.vao); // Activate the VBOs
    glActiveTexture(GL_TEXTURE0);    // bind texture
    glBindTexture(GL_TEXTURE_2D, gTextureId);
    glDrawElements(GL_TRIANGLES, gMesh.nIndices, GL_UNSIGNED_SHORT, NULL);    // Draws the triangles for the bowl

    // gMesh2 
    glBindVertexArray(gMesh2.vao); // Activate the VBOS
    glBindTexture(GL_TEXTURE_2D, gTextureId);
    glDrawElements(GL_TRIANGLES, gMesh2.nIndices, GL_UNSIGNED_SHORT, NULL);    // Draws the triangles for the bowl base

    // gMesh3
    glBindVertexArray(gMesh3.vao); // Activate VBOS
    glBindTexture(GL_TEXTURE_2D, gTextureId4);
    glDrawElements(GL_TRIANGLES, gMesh3.nIndices, GL_UNSIGNED_SHORT, NULL); // Draws the triangle for the ramekin

    // gMesh4
    glBindVertexArray(gMesh4.vao); // Activate VBOS
    glBindTexture(GL_TEXTURE_2D, gTextureId3);
    glDrawElements(GL_TRIANGLES, gMesh4.nIndices, GL_UNSIGNED_SHORT, NULL); // Draws the triangle for the vase mouth

    // gMesh5
    glBindVertexArray(gMesh5.vao); // Activate VBOS
    glBindTexture(GL_TEXTURE_2D, gTextureId3);
    glDrawElements(GL_TRIANGLES, gMesh5.nIndices, GL_UNSIGNED_SHORT, NULL); // Draws the triangle for the vase stem

    // gMesh6
    glBindVertexArray(gMesh6.vao); // Activate VBOS
    glBindTexture(GL_TEXTURE_2D, gTextureId3);
    glDrawElements(GL_TRIANGLES, gMesh6.nIndices, GL_UNSIGNED_SHORT, NULL); // Draws the triangle for the vase base

    // gMesh7
    glBindVertexArray(gMesh7.vao); // Activate VBOS
    glBindTexture(GL_TEXTURE_2D, gTextureId2);
    glDrawElements(GL_TRIANGLES, gMesh7.nIndices, GL_UNSIGNED_SHORT, NULL); // Draws the triangle for the table

    // gMesh8
    glBindVertexArray(gMesh8.vao); // Activate VBOS
    glBindTexture(GL_TEXTURE_2D, gTextureId5);
    glDrawElements(GL_TRIANGLES, gMesh8.nIndices, GL_UNSIGNED_SHORT, NULL); // Draws the triangle for ramekin detail

    // Deactivate the Vertex Array Object
    glBindVertexArray(0);

    // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
    glfwSwapBuffers(gWindow);    // Flips the the back buffer with the front buffer every frame.

}

// method for drawing cylider
void DrawCylinder(float cx, float cy, float z, float r, int num_segments, float length, float u, float v, GLfloat vertexArray[], GLushort indiceArray[])
{
  
    // fill vertex array for top circle of cylinder
    for (int i = 0; i < num_segments + 1; i++)
    {
        float theta = 2.0f * 3.1415926f * float(i) / float(num_segments);//get the current angle
        float x = r * cosf(theta);//calculate the x component
        float y = r * sinf(theta);//calculate the y component

        vertexArray[(i * 5) + 0] = x + cx;
        vertexArray[(i * 5) + 1] = y + cy;
        vertexArray[(i * 5) + 2] = z;
        vertexArray[(i * 5) + 3] = ((x + r) / (2 * r));
        vertexArray[(i * 5) + 4] = ((y + r) / (2 * r));

        //cout << u << "|" << v << endl;
    }

    // connect triangles for top circle
    for (GLushort i = 0; i < num_segments - 2; i++)
    {
        indiceArray[(i * 3) + 0] = 0;
        indiceArray[(i * 3) + 1] = i + 1;
        // logic for connecting last triangle to fist triangle
        if (i + 1 == num_segments) { 
            indiceArray[(i * 3) + 2] = (i + 1) - num_segments;
        }
        else
        {
            indiceArray[(i * 3) + 2] = i + 2;
        }
    }

    z = length; // value for setting z for bottom circle of cylinder
    int vertexArrayOffset = (num_segments) * 5; // value to offset vertex Array for second circle
    int indiceArrayOffset = (num_segments - 2) * 3; // value to offset indice array for second circle
    int totalCircleSegments = vertexArrayOffset / 5; // value for calculating cylinder slat pairings
 
    // fill vertex array
    for (int i = 0; i < num_segments; i++)
    {
        float theta = 2.0f * 3.1415926f * float(i) / float(num_segments);//get the current angle
        float x = r * cosf(theta);//calculate the x component
        float y = r * sinf(theta);//calculate the y component

        vertexArray[vertexArrayOffset + (i * 5) + 0] = x + cx;
        vertexArray[vertexArrayOffset + (i * 5) + 1] = y + cy;
        vertexArray[vertexArrayOffset + (i * 5) + 2] = z;
        vertexArray[vertexArrayOffset + (i * 5) + 3] = ((x + r) / (2 * r));
        vertexArray[vertexArrayOffset + (i * 5) + 4] = ((y + r) / (2 * r));
        //cout << u << "|" << v << endl;
 
    }

    // connect triangles for bottom circle
    for (GLushort i = 0; i < num_segments - 2; i++)
    {
        indiceArray[indiceArrayOffset + (i * 3) + 0] = totalCircleSegments + 0;
        indiceArray[indiceArrayOffset + (i * 3) + 1] = totalCircleSegments + (i + 1);
        // logic for connecting last triangle to first
        if (i + 1 == num_segments) { 
            indiceArray[indiceArrayOffset + (i * 3) + 2] = (totalCircleSegments + (i + 1)) - num_segments;
        }
        else
        {
            indiceArray[indiceArrayOffset + (i * 3) + 2] = totalCircleSegments + (i + 2);
        }
    }

    int indiceOffsetSlats = indiceArrayOffset * 2; // value to offset indice array values for cylinder slats

    for (int i = 0; i < num_segments; i++)
    {
        indiceArray[indiceOffsetSlats + (i * 6)] = i;
        indiceArray[indiceOffsetSlats + (i * 6) + 3] = i;
        // logic for connecting last triangle to first
        if (i + 1 == num_segments) { 
            indiceArray[indiceOffsetSlats + (i * 6) + 1] = num_segments;
            indiceArray[indiceOffsetSlats + (i * 6) + 2] = (i + 1) + totalCircleSegments - 1;
            indiceArray[indiceOffsetSlats + (i * 6) + 4] = 0;
            indiceArray[indiceOffsetSlats + (i * 6) + 5] = totalCircleSegments;
        }
        else
        {
            indiceArray[indiceOffsetSlats + (i * 6) + 1] = i + 1;
            indiceArray[indiceOffsetSlats + (i * 6) + 2] = (i + 1) + totalCircleSegments;
            indiceArray[indiceOffsetSlats + (i * 6) + 4] = i + (totalCircleSegments);
            indiceArray[indiceOffsetSlats + (i * 6) + 5] = i + totalCircleSegments + 1;
        }
    }
}


void DrawTorus(float x, float y, float z, vector <GLfloat>& vertices, vector <GLushort>& indices, float r, float R, int nr, int nR) {

    const float pi = 3.1415926f; // variable for calulating pi
    float du = 2 * pi / nR; // calculate arch for outer ring
    float dv = 2 * pi / nr; // calculate arch for inner ring
    int counter = 1; // counter for sending vertices to index array


    for (size_t i = 0; i < nR; i++) {

        float u = i * du;

        for (size_t j = 0; j <= nr; j++) {

            float v = (j % nr) * dv;
            // float values used to adjust torus position
            float xSkew = 0.75;
            float ySkew = 1.90;
            float zSkew = 3.20;

            for (size_t k = 0; k < 2; k++)
            {              
                float uu = u + k * du;
                // compute vertex
                 x = (R + r * cos(v)) * cos(uu) + xSkew ;
                 y = (R + r * cos(v)) * sin(uu) - ySkew;
                 z = r * sin(v) + zSkew;

                float tx = uu / (2 * pi); // calculate texCoordinate u
                float ty = v / (2 * pi);  // calculate texCooridnate v

                // add vertex to vertice vector
                vertices.push_back(x);
                vertices.push_back(y);
                vertices.push_back(z);
                vertices.push_back(tx); 
                vertices.push_back(ty);

                counter++;

                // logic for adding values to indice array
                if (counter % 4 == 0) // adds to vector only after obtaining 4 values (two triangles)
                {
                    indices.push_back(counter - 1);
                    indices.push_back(counter - 2);
                    indices.push_back(counter - 3);
                    indices.push_back(counter - 2);
                    indices.push_back(counter - 3);
                    indices.push_back(counter - 4);
                }
                else if (counter % 2 == 0) { // connects the triangles that mod 4 skips
                    if (counter != 2) { // logic to ensure mod 2 is only used AFTER the first four points are added to the vector
                        indices.push_back(counter - 4);
                        indices.push_back(counter - 3);
                        indices.push_back(counter - 2);
                        indices.push_back(counter - 2);
                        indices.push_back(counter - 3);
                        indices.push_back(counter - 1);
                    }
                }
            }
            // increment angle
            v += dv;
        }
    }
}



// Implements the UCreateMesh function
void UCreateMesh(GLMesh& mesh, GLMesh& mesh2, GLMesh& mesh3, GLMesh& mesh4, GLMesh& mesh5, GLMesh& mesh6, GLMesh& mesh7, GLMesh& mesh8)
{

    const int numSegments = 100; // value to determine number of triangles for cydlinder shapes

    // BOWL (cylinder)
    GLfloat bowlVerts[((numSegments + 1) * 5) * 2]; // vertex values for bowl
    GLushort bowlIndices[((numSegments * 3) * 4) - 12]; // indice values for bowl
    DrawCylinder(-2.0f, -2.0f, 0.5f, 1.5f, numSegments, 2.00f, 1.0f, 0.0f, bowlVerts, bowlIndices); // draw cylinder

    // BOWL BASE (cyldinder)
    GLfloat bowlBaseVerts[((numSegments + 1) * 5) * 2];  // vertex valus for bowl basae=
    GLushort indices[((numSegments * 3) * 4) - 12]; // indice values for bowl base
    DrawCylinder(-2.0f, -2.0f, 0.0f, 1.0f, numSegments, 1.5f, 1.0f, 1.0f, bowlBaseVerts, indices); // draw cylinder

    // RAMEKIN (cylinder)
    GLfloat ramekinVerts[((numSegments + 1) * 5) * 2];  // vertex values for ramekin
    GLushort ramekinIndices[((numSegments * 3) * 4) - 12]; // indice values for ramekin
    DrawCylinder(-0.5f, -3.4, 0.0f, 0.5f, numSegments, 0.5f, 1.0f, 1.0f, ramekinVerts, ramekinIndices); // draw cylinder

    // RAMEKIN LIP (cylinder)
    GLfloat ramekinLipVerts[((numSegments + 1) * 5) * 2]; // vertex values for ramekin lip
    GLushort ramekinLipIndices[((numSegments * 3) * 4) - 12]; // indice values for ramekin lip
    DrawCylinder(-.5, -3.4, .501 , 0.41, numSegments, 0.0, 1.0, 1.0, ramekinLipVerts, ramekinLipIndices); // draw cylinder

    // VASE MOUTH (torus)
    vector <GLfloat> vaseMouthVerts; // vertex values for vase mouth
    vector <GLushort> vaseMouthIndices; // indice values for vase mouth
    const int innerSegments = 16; // value for inner ring of value torus
    const int outerSegments = 16; // value for outer ring value of torus
    DrawTorus(0.0, 0.0, 0.0, vaseMouthVerts, vaseMouthIndices, 0.1, .5, innerSegments, outerSegments); // draw torus

    // VASE STEM (cylinder)
    GLfloat stemVerts[((numSegments + 1) * 5) * 2];  // vertex values for vase stem
    GLushort stemIndices[((numSegments * 3) * 4) - 12]; // indice values for vase stem
    DrawCylinder(.75f, -1.90, 2.0f, 0.45f, numSegments, 3.2f, 1.0f, 1.0f, stemVerts, stemIndices); // draw cylinder

    // VASE BASE (sphere)
    Sphere vaseBase(1.20, 100, 20, true); // instantiate Sphere object with radius, sector, and stack values


    // TABLE (plane)
    // vertices, color values, and texture coordinates for plane
    GLfloat planeVerts[] = {
        -4.0f,-5.0f, 0.0f,      1.0f,1.0f,1.0f,1.0f,        0.0,0.0,    //a
         2.5f,-5.0f, 0.0f,      1.0f,1.0f,1.0f,1.0f,        0.0,1.0,    //b
         2.5f, 2.0f, 0.0f,      0.0f,0.0f,0.0f,1.0f,        1.0,0.0,    //c
        -4.0f, 2.0f, 0.0f,      1.0f,1.0f,1.0f,1.0f,        1.0,1.0     //d
    };

    // Indice values for plane
    GLushort planeIndices[] = {
        0,3,2,
        0,1,2
    };

    const GLuint floatsPerVertex = 3;
    const GLuint floatsPerUV = 2;
    const GLuint floatsPerNormal = 3;

    // Strides between vertex coordinates is 6 (x, y, z, r, g, b, a). A tightly packed stride is 0.
    GLint stride = sizeof(float) * (floatsPerVertex + floatsPerUV);// calculate stride for cylinders
    GLint planeStride = sizeof(float) * (floatsPerVertex + floatsPerUV + 4); // calculate stride for plane
    int sphereStride = vaseBase.getInterleavedStride(); // calculate stride for sphere


    // GENERATE AND CREATE BUFFERS AND VERTEX POINTERS FOR 3D SHAPES
    // BEGIN FIRST SHAPE //
    // CYLINDER (bowl) generate VAOs 
    glGenVertexArrays(1, &mesh.vao);
    glBindVertexArray(mesh.vao);

    // Create 2 buffers
    glGenBuffers(2, mesh.vbos);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbos[0]); // Activates the buffer
    glBufferData(GL_ARRAY_BUFFER, sizeof(bowlVerts), bowlVerts, GL_STATIC_DRAW); // Sends verteX data to the GPU
    mesh.nIndices = sizeof(bowlIndices) / sizeof(bowlIndices[0]); // calculate the number of indices
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.vbos[1]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(bowlIndices), bowlIndices, GL_STATIC_DRAW);

    // Create Vertex Attribute Pointers
    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * floatsPerVertex));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * floatsPerVertex));
    glEnableVertexAttribArray(2);
    // END FIRST SHAPE //

    // BEGIN SECOND SHAPE //
    //CYLINDER (base) generate VAOs 
    glGenVertexArrays(1, &mesh2.vao); 
    glBindVertexArray(mesh2.vao);

    // Create 2 buffers 
    glGenBuffers(2, mesh2.vbos);
    glBindBuffer(GL_ARRAY_BUFFER, mesh2.vbos[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(bowlBaseVerts), bowlBaseVerts, GL_STATIC_DRAW);
    mesh2.nIndices = sizeof(indices) / sizeof(indices[0]);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh2.vbos[1]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // Create Vertex Attribute Pointers 
    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * floatsPerVertex));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * floatsPerVertex));
    glEnableVertexAttribArray(2);
    // END SECOND SHAPE //
    
    // BEGIN THIRD SHAPE //
    //CYLINDER (bowl) generate VAOs 
    glGenVertexArrays(1, &mesh3.vao);
    glBindVertexArray(mesh3.vao);

    // Create 2 buffers
    glGenBuffers(2, mesh3.vbos);
    glBindBuffer(GL_ARRAY_BUFFER, mesh3.vbos[0]); 
    glBufferData(GL_ARRAY_BUFFER, sizeof(ramekinVerts), ramekinVerts, GL_STATIC_DRAW);
    mesh3.nIndices = sizeof(ramekinIndices) / sizeof(ramekinIndices[0]);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh3.vbos[1]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(ramekinIndices), ramekinIndices, GL_STATIC_DRAW);

    // Create Vertex Attribute Pointers
    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * floatsPerVertex));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * floatsPerVertex));
    glEnableVertexAttribArray(2);
    // END THIRD SHAPE //

    // BEGIN FOURTH SHAPE //
    // RAMEKIN LIP (TORUS)
    glGenVertexArrays(1, &mesh8.vao); 
    glBindVertexArray(mesh8.vao); 

    // Create 2 buffers
    glGenBuffers(2, mesh8.vbos);
    glBindBuffer(GL_ARRAY_BUFFER, mesh8.vbos[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(ramekinLipVerts), ramekinLipVerts, GL_STATIC_DRAW);
    mesh8.nIndices = sizeof(ramekinLipIndices) / sizeof(ramekinLipIndices[0]);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh8.vbos[1]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(ramekinLipIndices), ramekinLipIndices, GL_STATIC_DRAW);

    // Create Vertex Attribute Pointers 
    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float)* floatsPerVertex));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float)* floatsPerVertex));
    glEnableVertexAttribArray(2);
    // END FOURTH SHAPE //

    // BEGIN FIFTH SHAPE
    // VASE MOUTH (TORUS)
    glGenVertexArrays(1, &mesh4.vao); 
    glBindVertexArray(mesh4.vao); 

    // Create 2 buffers
    glGenBuffers(2, mesh4.vbos);
    glBindBuffer(GL_ARRAY_BUFFER, mesh4.vbos[0]); 
    glBufferData(GL_ARRAY_BUFFER, sizeof(float)*vaseMouthVerts.size(), vaseMouthVerts.data(), GL_STATIC_DRAW);
    mesh4.nIndices = vaseMouthIndices.size(); // calculate the number of indices for a torus
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh4.vbos[1]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLushort)*vaseMouthIndices.size(), vaseMouthIndices.data(), GL_STATIC_DRAW);

    // Create Vertex Attribute Pointers 
    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float)* floatsPerVertex));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float)* floatsPerVertex));
    glEnableVertexAttribArray(2);
    //END FIFTH SHAPE //

    // BEGIN SIXTH SHAPE
    // VASE STEM (CYLINDER)
    glGenVertexArrays(1, &mesh5.vao);
    glBindVertexArray(mesh5.vao);
    // Create 2 buffers
    glGenBuffers(2, mesh5.vbos);
    glBindBuffer(GL_ARRAY_BUFFER, mesh5.vbos[0]); 
    glBufferData(GL_ARRAY_BUFFER, sizeof(stemVerts), stemVerts, GL_STATIC_DRAW);
    mesh5.nIndices = sizeof(stemIndices) / sizeof(stemIndices[0]);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh5.vbos[1]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(stemIndices), stemIndices, GL_STATIC_DRAW);

    // Create Vertex Attribute Pointers
    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float)* floatsPerVertex));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float)* floatsPerVertex));
    glEnableVertexAttribArray(2);
    //END SIXTH SHAPE //

    // BEGIN SEVENTH SHAPE
    // VASE BASE (SPHERE)
    glGenVertexArrays(1, &mesh6.vao); 
    glBindVertexArray(mesh6.vao);

    // Create 2 buffers
    glGenBuffers(2, mesh6.vbos);
    glBindBuffer(GL_ARRAY_BUFFER, mesh6.vbos[0]); 
    glBufferData(GL_ARRAY_BUFFER, vaseBase.getInterleavedVertexSize(), vaseBase.getInterleavedVertices(), GL_STATIC_DRAW); 
    mesh6.nIndices = vaseBase.getIndexSize(); // calculate number of indices for a sphere
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh6.vbos[1]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, vaseBase.getIndexSize(), vaseBase.getIndices(), GL_STATIC_DRAW);

    // Create Vertex Attribute Pointers 
    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, sphereStride, 0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, sphereStride, (void*)(sizeof(float)* floatsPerVertex));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, sphereStride, (void*)(sizeof(float)* floatsPerVertex));
    glEnableVertexAttribArray(2);
    //END SEVENTH SHAPE //

    //BEGIN EIGHT (FINAL) SHAPE //
   // PLANE (table) generate VAOs
    glGenVertexArrays(1, &mesh7.vao); 
    glBindVertexArray(mesh7.vao); 

    // Create 2 buffers
    glGenBuffers(2, mesh7.vbos);
    glBindBuffer(GL_ARRAY_BUFFER, mesh7.vbos[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(planeVerts), planeVerts, GL_STATIC_DRAW); 
    mesh7.nIndices = sizeof(planeIndices) / sizeof(planeIndices[0]); // calculate number of indices for a plane
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh7.vbos[1]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(planeIndices), planeIndices, GL_STATIC_DRAW);

    // Create Vertex Attribute Pointers
    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, planeStride, 0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float)* floatsPerVertex));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float)* floatsPerVertex));
    glEnableVertexAttribArray(2);
    //END FINAL SHAPE //
}


void UDestroyMesh(GLMesh& mesh, GLMesh& mesh2, GLMesh& mesh3, GLMesh& mesh4, GLMesh& mesh5, GLMesh& mesh6, GLMesh& mesh7, GLMesh& mesh8)
{
    glDeleteVertexArrays(1, &mesh.vao); // BOWL
    glDeleteBuffers(2, mesh.vbos);

    glDeleteVertexArrays(1, &mesh2.vao); // BOWL BASE
    glDeleteBuffers(2, mesh2.vbos);

    glDeleteVertexArrays(1, &mesh3.vao); // RAMEKIN
    glDeleteBuffers(2, mesh3.vbos);

    glDeleteVertexArrays(1, &mesh4.vao); // VASE MOUTH
    glDeleteBuffers(2, mesh4.vbos);

    glDeleteVertexArrays(1, &mesh5.vao); // VASE STEM
    glDeleteBuffers(2, mesh5.vbos);

    glDeleteVertexArrays(1, &mesh6.vao); // VASE BASE
    glDeleteBuffers(2, mesh6.vbos);

    glDeleteVertexArrays(1, &mesh7.vao); // TABLE
    glDeleteBuffers(2, mesh7.vbos);

    glDeleteVertexArrays(1, &mesh8.vao); // RAMEKIN LIP
    glDeleteBuffers(2, mesh8.vbos);

}

/*Generate and load the texture*/
bool UCreateTexture(const char* filename, GLuint& textureId, char wrapType)
{

    char wrap = wrapType;
    int width, height, channels;
    unsigned char* image = stbi_load(filename, &width, &height, &channels, 0);
    if (image)
    {
        flipImageVertically(image, width, height, channels);

        glGenTextures(1, &textureId);
        glBindTexture(GL_TEXTURE_2D, textureId);

        if (wrap == 'm') // mirrored repeat wrapping method
        {
            // set the texture wrapping parameters
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
            // set texture filtering parameters
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        }

        else if (wrap == 'c') // clamp to edge wrapping method
        {
            // set the texture wrapping parameters
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            // set texture filtering parameters
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        }

        if (channels == 3)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
        else if (channels == 4)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
        else
        {
            cout << "Not implemented to handle image with " << channels << " channels" << endl;
            return false;
        }

        glGenerateMipmap(GL_TEXTURE_2D);

        stbi_image_free(image);
        glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

        return true;
    }

    // Error loading the image
    return false;
}


void UDestroyTexture(GLuint textureId)
{
    glGenTextures(1, &textureId);
}

// Implements the UCreateShaders function
bool UCreateShaderProgram(const char* vtxShaderSource, const char* fragShaderSource, GLuint& programId)
{
    // Compilation and linkage error reporting
    int success = 0;
    char infoLog[512];

    // Create a Shader program object.
    programId = glCreateProgram();

    // Create the vertex and fragment shader objects
    GLuint vertexShaderId = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragmentShaderId = glCreateShader(GL_FRAGMENT_SHADER);

    // Retrive the shader source
    glShaderSource(vertexShaderId, 1, &vtxShaderSource, NULL);
    glShaderSource(fragmentShaderId, 1, &fragShaderSource, NULL);

    // Compile the vertex shader, and print compilation errors (if any)
    glCompileShader(vertexShaderId); // compile the vertex shader
    // check for shader compile errors
    glGetShaderiv(vertexShaderId, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShaderId, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;

        return false;
    }

    glCompileShader(fragmentShaderId); // compile the fragment shader
    // check for shader compile errors
    glGetShaderiv(fragmentShaderId, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShaderId, sizeof(infoLog), NULL, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;

        return false;
    }

    // Attached compiled shaders to the shader program
    glAttachShader(programId, vertexShaderId);
    glAttachShader(programId, fragmentShaderId);

    glLinkProgram(programId);   // links the shader program
    // check for linking errors
    glGetProgramiv(programId, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(programId, sizeof(infoLog), NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;

        return false;
    }

    glUseProgram(programId);    // Uses the shader program

    return true;
}


void UDestroyShaderProgram(GLuint programId)
{
    glDeleteProgram(programId);
}

