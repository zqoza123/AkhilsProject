#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <memory>
#include <vector>
#include <math.h>
#include <random>
#include <unordered_map>

// Global Variables (Mostly to fix screen ratio issues)
int screenSizeX = 800;
int screenSizeY = 600;
int cellsX;
int cellsY;
int scaleCoefficent = 100; //pixels per "meter"
float gravity = -9.8f * scaleCoefficent;
float radius = 4.0f;
int particles = 750;
int solverIterations = 3;

// Uniform grid for collision checks
int gridSize = 2 * radius;

// transform matricies
glm::mat4 view;
glm::mat4 projection;

unsigned int viewLoc;
unsigned int projLoc;
unsigned int modelLocation;
unsigned int shaderProgram;
unsigned int speedLoc;

// momentum
float restitution = 0.1f;
std::random_device seed;
std::mt19937 gen{seed()};
std::uniform_int_distribution<> dist{10, 590};

// Callback Functions
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    screenSizeX = width;
    screenSizeY = height;
    cellsX = screenSizeX / gridSize;
    cellsY = screenSizeY / gridSize;
    projection = glm::ortho(0.0f, (float) screenSizeX, 0.0f, (float) screenSizeY, -1.0f, 1.0f);
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
    glViewport(0, 0, width, height);
}

// Fluid Sim Interactons
void freezeAnimation()
{
    // Not implemented
}

// Periphial Interactions
void processInput(GLFWwindow *window)
{
    if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if(glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS)
        freezeAnimation();
}

std::string readFile(const char* path)
{
    std::ifstream file(path);
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// Particle Class
class Particle {
public:
    unsigned int VAO, VBO;
    glm::vec3 position, velocity, acceleration;
    float rotation, scaleX, scaleY, mass, radius;
    int resolution;
    glm::mat4 modelMatrix;

    Particle(
        float* vertex,
        float _radius,
        int _resolution,
        glm::vec3 _velocity = glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3 _acceleration = glm::vec3(0.0f, gravity, 0.0f),
        float _rotation = 0.0f,
        float _mass = 1.0f ) : 

            velocity(_velocity),
            acceleration(_acceleration),
            rotation(_rotation),
            scaleX(1.0f),
            scaleY(1.0f),
            mass(_mass),
            radius(_radius),
            resolution(_resolution)

    {
        position = glm::vec3(vertex[0], vertex[1], 0.0f);
        modelMatrix = buildMatrix();
        std::vector<float> edgePoints;
        edgePoints.push_back(0.0f);
        edgePoints.push_back(0.0f);

        for (int i = 0; i <= resolution; i++)
        {
            float angle = i * 2.0f * M_PI / resolution;
            edgePoints.push_back(radius* std::cos(angle));
            edgePoints.push_back(radius * std::sin(angle));
        }

        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);

        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, edgePoints.size() * sizeof(float), edgePoints.data(), GL_STATIC_DRAW);

        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        glBindVertexArray(0); // unbind after setup
    }

    // Avoid Copying and Collisions
    Particle(const Particle&) = delete;
    Particle& operator=(const Particle&) = delete;

    ~Particle()
    {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
    }

    void draw()
    {
        glBindVertexArray(VAO);
        glUniform1f(speedLoc, glm::dot(velocity, velocity));  
        glDrawArrays(GL_TRIANGLE_FAN, 0, resolution + 2);
        glBindVertexArray(0);
    }

    glm::mat4 buildMatrix()
    {
        glm::mat4 matrix = glm::mat4(1.0f);
        matrix = glm::translate(matrix, position);
        matrix = glm::rotate(matrix, glm::radians(rotation), glm::vec3(0,0,1));
        matrix = glm::scale(matrix, glm::vec3(scaleX, scaleY, 1.0f));
        return matrix;
    }

    void updatePositions(float deltaTime)
    {
        position += velocity * deltaTime + 0.5f * acceleration * deltaTime * deltaTime;
        velocity += acceleration * deltaTime;

        // Light damping to reduce energy buildup
        velocity *= 0.99f;

        // Clamp velocity magnitude to avoid numerical explosion
        const float maxSpeed = 1000.0f;
        float speedSq = glm::dot(velocity, velocity);
        if (speedSq > maxSpeed * maxSpeed) {
            velocity = glm::normalize(velocity) * maxSpeed;
        }

        boundaryCollisions();
        modelMatrix = buildMatrix();
    }

    void boundaryCollisions()
    {
        if (position.x + radius >= (float) screenSizeX) {
            position.x = (float) screenSizeX - radius;
            velocity.x *= -restitution;
        }

        if (position.x - radius <= 0.0f) {
            position.x = radius;
            velocity.x *= -restitution;
        }

        if (position.y + radius >= (float) screenSizeY) {
            position.y = (float) screenSizeY - radius;
            velocity.y *= -restitution;
        }
        
        if (position.y - radius <= 0.0f) {
            position.y = radius;
            velocity.y *= -restitution;
        }
    }

    void computeAcceleration()
    {
        acceleration = glm::vec3(0.0f, 0.0f + gravity, 0.0f);
    }
};

// Grid Functions
void generateGrid(std::vector<std::unique_ptr<Particle>>& objectArray, std::vector<std::vector<Particle*>>& grid)
{
    for (const auto& object : objectArray)
    {
        glm::vec3 objectPos = object->position;
        int gridX = static_cast<int>(objectPos.x / gridSize); //indices
        int gridY = static_cast<int>(objectPos.y / gridSize);

        int key = gridX + gridY * cellsX;
        grid[key].push_back(object.get());
    }
}

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    

    GLFWwindow* window = glfwCreateWindow(screenSizeX, screenSizeY, "Particle Simulation", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // VSYNC (For Smoother Collisions)
    glfwSwapInterval(0);

    // Create vertex and fragment shaders
    std::string vertexCode = readFile("src/shaders/vertexShader.glsl");
    const char* vertexShaderSource = vertexCode.c_str();
    unsigned int vertexShader;
    vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    std::string fragmentCode = readFile("src/shaders/fragmentShader.glsl");
    const char* fragmentShaderSource = fragmentCode.c_str();
    unsigned int fragmentShader;
    fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    // Create Shader Program
    shaderProgram = glCreateProgram();

    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    // Screen setup

    glViewport(0, 0, screenSizeX, screenSizeY);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // Object Context
    {
        // Declare object manager
        std::vector<std::unique_ptr<Particle>> objectArray;

        // Particle Creation
        for (int i = 0; i < particles; i++)
        {
            float vertex[] = {(float) dist(gen), (float) dist(gen), 0.0f};
            objectArray.push_back(std::make_unique<Particle>(vertex, radius, 15));
        }
        
        // Shader Setup
        view = glm::mat4(1.0f);
        projection = glm::ortho(0.0f, (float) screenSizeX, 0.0f, (float) screenSizeY, -1.0f, 1.0f);

        speedLoc = glGetUniformLocation(shaderProgram, "speedSq");
        viewLoc = glGetUniformLocation(shaderProgram, "view");
        projLoc = glGetUniformLocation(shaderProgram, "projection");
        modelLocation = glGetUniformLocation(shaderProgram, "model");

        glUseProgram(shaderProgram);
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
        
        // Shader Compilation Debug

        int success;
        char infoLog[512];
        glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
            std::cout << "Vertex Shader Error:\n" << infoLog << std::endl;
        }

        glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
            std::cout << "Fragment Shader Error:\n" << infoLog << std::endl;
        }

        glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
        if (!success)
        {
            glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
            std::cout << "Shader Program Link Error:\n" << infoLog << std::endl;
        }

        // Delta Time
        float prevFrame = 0.0f;
        float currentFrame;
        float deltaTime;
        const float minTime = 1.0f / 250.0f;

        //object grid (for velocity mapping)
        cellsX = screenSizeX / gridSize; //100
        cellsY = screenSizeY / gridSize; //75
        std::vector<std::vector<Particle*>> grid(cellsX * cellsY, std::vector<Particle*>());
        std::vector<glm::vec3> impulses(particles, glm::vec3(0.0f));

        // Main Loop
        while(!glfwWindowShouldClose(window))
        {
            // Calculate the frame difference
            currentFrame = static_cast<float>(glfwGetTime());
            deltaTime = std::min(currentFrame - prevFrame, minTime);
            prevFrame = currentFrame;
            //std::cout << std::to_string(1 / deltaTime) + "\n"; //fps

            // Check for window changes
            processInput(window);

            // Rendering
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            // Update Velocities + solver
            // Reset objects
            for (auto& cell : grid) cell.clear();
            generateGrid(objectArray, grid);

            for (int iter = 0; iter < solverIterations; iter++) {
                std::fill(impulses.begin(), impulses.end(), glm::vec3(0.0f));
                for (int p = 0; p < objectArray.size(); p++)
                {
                    auto& object = objectArray[p];
                    auto& A = *object;
                    glm::vec3 objectPos = A.position;
                    int gridX = static_cast<int>(objectPos.x / gridSize); //indices
                    int gridY = static_cast<int>(objectPos.y / gridSize);
                    int checks = 0;
                    for (int i = -1; i < 2; i++) { //row
                        for (int j = -1; j < 2; j++) { //column
                            if (gridX + i >= 0 && gridX + i < cellsX && gridY + j >= 0 && gridY + j < cellsY) {
                                int key = (gridX + i) + (gridY + j) * cellsX;
                                // collision checker
                                for (auto& collider : grid[key]){
                                    if (object.get() != collider) {
                                        auto& B = *collider;
                                        glm::vec3 normal = A.position - B.position;
                                        float dist2 = glm::dot(normal, normal);
                                        const float eps = 1e-6f; // prevent division by zero
                                        float maxDist = 2.0f * radius;
                                        float maxDist2 = maxDist * maxDist;
                                        // check collision with small-distance guard and tighter radius
                                        if (dist2 > eps && dist2 <= maxDist2)
                                        {
                                            // normalize
                                            normal *= glm::inversesqrt(dist2);
                                            glm::vec3 relativeVelocity = A.velocity - B.velocity;
                                            float velAlongNormal = glm::dot(relativeVelocity, normal);
                                            // check direciton
                                            if (velAlongNormal < 0.0f)
                                            {
                                                glm::vec3 impulse = 0.5f * (1.0f + restitution) * velAlongNormal * normal;
                                                impulses[p] += impulse;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            for (int p = 0; p < objectArray.size(); p++)
            {
                (*objectArray[p]).velocity -= impulses[p];
            }
        }

            // Draw and Update Velocities
            for (int p = 0; p < objectArray.size(); p++)
            {
                auto& object = *objectArray[p];
                object.updatePositions(deltaTime);
                glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(object.modelMatrix));
                object.draw();
            }

            // Update
            glfwSwapBuffers(window);
            glfwPollEvents();    
        }
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);  
    glfwTerminate();
    return 0;
}