#include <SDL.h>
#include <iostream>
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include <vector>
#include <sstream>
#include <cassert>
#include "color.h"
#include "print.h"
#include "framebuffer.h"
#include "uniforms.h"
#include "shaders.h"
#include "fragment.h"
#include "triangle.h"
#include "camera.h"
#include "ObjLoader.h"
#include "noise.h"
#include "model.h"
#include <thread>


SDL_Window* window = nullptr;
SDL_Renderer* renderer = nullptr;
Color currentColor;

std::vector<Model> models;

bool init() {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "Error: Failed to initialize SDL: " << SDL_GetError() << std::endl;
        return false;
    }

    window = SDL_CreateWindow("Software Renderer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (!window) {
        std::cerr << "Error: Failed to create SDL window: " << SDL_GetError() << std::endl;
        return false;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        std::cerr << "Error: Failed to create SDL renderer: " << SDL_GetError() << std::endl;
        return false;
    }

    setupNoise();

    return true;
}

void setColor(const Color& color) {
    currentColor = color;
}

// Función de renderización paralela para cada hilo
void renderParallel(int threadID, int numThreads) {
    for (int i = threadID; i < models.size(); i += numThreads) {
        // Obtener el modelo que este hilo debe procesar
        Model& model = models[i];

        // Resto del código de renderización aquí...
        // 1. Vertex Shader
        std::vector<Vertex> transformedVertices(model.VBO.size() / 3);
        for (size_t j = 0; j < model.VBO.size() / 3; ++j) {
            Vertex vertex = { model.VBO[j * 3], model.VBO[j * 3 + 1], model.VBO[j * 3 + 2] };
            transformedVertices[j] = vertexShader(vertex, model.uniforms);
        }

        // 2. Primitive Assembly
        std::vector<std::vector<Vertex>> assembledVertices(transformedVertices.size() / 3);
        for (size_t j = 0; j < transformedVertices.size() / 3; ++j) {
            Vertex edge1 = transformedVertices[3 * j];
            Vertex edge2 = transformedVertices[3 * j + 1];
            Vertex edge3 = transformedVertices[3 * j + 2];
            assembledVertices[j] = { edge1, edge2, edge3 };
        }

        // 3. Rasterization
        std::vector<Fragment> fragments;
        for (size_t j = 0; j < assembledVertices.size(); ++j) {
            std::vector<Fragment> rasterizedTriangle = triangle(
                    assembledVertices[j][0],
                    assembledVertices[j][1],
                    assembledVertices[j][2]
            );
            fragments.insert(fragments.end(), rasterizedTriangle.begin(), rasterizedTriangle.end());
        }

        // 4. Fragment Shader
        for (size_t j = 0; j < fragments.size(); ++j) {
            Fragment& fragment = fragments[j];
            shaderType currentShader = model.currentShader;

            switch (currentShader) {
                case SOL:
                    fragment = sol(fragment);
                    break;
                case TIERRA:
                    fragment = tierra(fragment);
                    break;
                case GASEOSO:
                    fragment = gaseoso(fragment);
                    break;
                case LUNA:
                    fragment = luna(fragment);
                    break;
                    // Añade más casos para otros shaders
            }
            point(fragment); // Be aware of potential race conditions here
        }
    }
}

// Función principal de renderización
void render() {
    // Especifica el número de hilos que deseas utilizar (por ejemplo, 4 hilos)
    int numThreads = 8;
    std::vector<std::thread> threads;

    // Iniciar los hilos
    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back(renderParallel, i, numThreads);
    }

    // Esperar a que todos los hilos terminen
    for (auto& thread : threads) {
        thread.join();
    }
}


glm::mat4 createViewportMatrix(size_t screenWidth, size_t screenHeight) {
    glm::mat4 viewport = glm::mat4(1.0f);
    viewport = glm::scale(viewport, glm::vec3(screenWidth / 2.0f, screenHeight / 2.0f, 0.5f));
    viewport = glm::translate(viewport, glm::vec3(1.0f, 1.0f, 0.5f));
    return viewport;
}

int main(int argc, char* argv[]) {
    if (!init()) {
        return 1;
    }

    std::vector<glm::vec3> vertices;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec3> texCoords;
    std::vector<Face> faces;
    std::vector<glm::vec3> vertexBufferObject;

    loadOBJ("C:\\Users\\caste\\OneDrive\\Documentos\\Universidad\\semestre6\\graficosxcomputador\\lab4\\sphere.obj", vertices, normals, texCoords, faces);

    for (const auto& face : faces) {
        for (int i = 0; i < 3; ++i) {
            glm::vec3 vertexPosition = vertices[face.vertexIndices[i]];
            glm::vec3 vertexNormal = normals[face.normalIndices[i]];
            glm::vec3 vertexTexture = texCoords[face.texIndices[i]];
            vertexBufferObject.push_back(vertexPosition);
            vertexBufferObject.push_back(vertexNormal);
            vertexBufferObject.push_back(vertexTexture);
        }
    }

    Uniforms uniforms;
    glm::mat4 model = glm::mat4(1);
    glm::mat4 view = glm::mat4(1);
    glm::mat4 projection = glm::mat4(1);

    Camera camera;
    camera.cameraPosition = glm::vec3(0.0f, 0.0f, 10.0f);
    camera.targetPosition = glm::vec3(0.0f, 0.0f, 0.0f);
    camera.upVector = glm::vec3(0.0f, 1.0f, 0.0f);

    float fovInDegrees = 45.0f;
    float aspectRatio = static_cast<float>(SCREEN_WIDTH) / static_cast<float>(SCREEN_HEIGHT);
    float nearClip = 0.1f;
    float farClip = 100.0f;
    uniforms.projection = glm::perspective(glm::radians(fovInDegrees), aspectRatio, nearClip, farClip);

    uniforms.viewport = createViewportMatrix(SCREEN_WIDTH, SCREEN_HEIGHT);
    Uint32 frameStart, frameTime;
    std::string title = "FPS: ";
    int speed = 10;

    Model sol;
    sol.VBO = vertexBufferObject;
    sol.currentShader = SOL;
    sol.uniforms = uniforms;

    models.push_back(sol);

    Model tierra;
    tierra.VBO = vertexBufferObject;
    tierra.currentShader = TIERRA;
    tierra.uniforms = uniforms;

    models.push_back(tierra);

    Model luna;
    luna.VBO = vertexBufferObject;
    luna.currentShader = LUNA;
    luna.uniforms = uniforms;

    models.push_back(luna);

    Model gaseoso;
    gaseoso.VBO = vertexBufferObject;
    gaseoso.currentShader = GASEOSO;
    gaseoso.uniforms = uniforms;

    models.push_back(gaseoso);

    // Posicion de los astros
    glm::vec3 newTranslationVector(0.0f, 0.0f, 0.0f);

    // Rotacion de los astros
    float rotaSol = 45.0f;
    float rotaTierra = 45.0f;
    float rotaLuna = 45.0f;
    float rotaGaseoso = 45.0f;
    glm::vec3 rotationAxis(0.0f, 1.0f, 0.0f);

    // Tamaño de los astros
    glm::vec3 scaleFactor(1.0f, 1.0f, 1.0f);

    // Movimiento de la camara
    glm::vec3 cameraPosition = glm::vec3(0.0f, 0.0f, 10.0f); // Posición inicial de la cámara
    glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);    // Dirección hacia la que mira la cámara
    glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);        // Vector "arriba" de la cámara
    float yaw = -90.0f;  // Ángulo de rotación en el plano horizontal (eje Y)
    float pitch = 0.0f;  // Ángulo de rotación en el plano vertical (eje X)
    float rotationSpeed = 1.0f;  // Velocidad de rotación de la cámara
    float pitchy = 0.0f;          // Ángulo de inclinación vertical
    float pitchSpeed = 1.0f;     // Velocidad de rotación vertical

    // Rotacion automatica
    bool isAutoRotation = true; // Inicialmente, la rotación no es automática


    bool running = true;
    while (running) {
        frameStart = SDL_GetTicks();

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }
            if (event.type == SDL_KEYDOWN) {
                switch (event.key.keysym.sym) {
                    case SDLK_LEFT:
                        cameraPosition -= glm::normalize(glm::cross(cameraFront, cameraUp));
                        break;
                    case SDLK_RIGHT:
                        cameraPosition += glm::normalize(glm::cross(cameraFront, cameraUp));
                        break;
                    case SDLK_UP:
                        cameraPosition += cameraFront;
                        break;
                    case SDLK_DOWN:
                        cameraPosition -= cameraFront;
                        break;
                    case SDLK_a:  // Rotar hacia la izquierda (sentido anti-horario)
                        isAutoRotation = false;
                        yaw -= rotationSpeed;
                        break;
                    case SDLK_d:  // Rotar hacia la derecha (sentido horario)
                        isAutoRotation = false;
                        yaw += rotationSpeed;
                        break;
                    case SDLK_l: // Cambiar entre rotación automática y manual
                        if (isAutoRotation) {
                            isAutoRotation = false; // Cambiar a rotación manual
                        } else {
                            isAutoRotation = true; // Cambiar a rotación automática
                        }
                        break;
                    case SDLK_w: // Rotar hacia arriba
                        pitch += pitchSpeed;
                        break;
                    case SDLK_s: // Rotar hacia abajo
                        pitch -= pitchSpeed;
                        break;
                }
            }
        }

        if (isAutoRotation) {
            rotationSpeed = 0.2f; // Establecer la velocidad de rotación automática
            yaw += rotationSpeed; // Rotación automática
        } else {
            rotationSpeed = 1.0f; // Establecer la velocidad de rotación manual
        }

        glm::mat4 view = glm::lookAt(cameraPosition, cameraPosition + cameraFront, cameraUp);
        view = glm::rotate(view, glm::radians(pitch), glm::vec3(1.0f, 0.0f, 0.0f)); // Rotación vertical
        view = glm::rotate(view, glm::radians(yaw), glm::vec3(0.0f, 1.0f, 0.0f)); // Rotación horizontal
        uniforms.view = view;



        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        int ox = 2;
        int oy = 2;
        clearFramebuffer(ox, oy);

        glm::mat4 rotation = glm::mat4(1.0f);
        for (auto& model: models){
            switch (model.currentShader) {
                case SOL:
                    rotaSol += 0.3f;
                    newTranslationVector = glm::vec3(0.0f, 0.0f, 0.0f);
                    scaleFactor = glm::vec3(1.0f, 1.0f, 1.0f);
                    rotation = glm::rotate(glm::mat4(1.0f), glm::radians(rotaSol), rotationAxis);
                    break;
                case TIERRA:
                    rotaTierra += 0.8f;
                    newTranslationVector = glm::vec3(1.5f, 0.0f, 0.0f);
                    scaleFactor = glm::vec3(0.5f, 0.5f, 0.5f);
                    rotation = glm::rotate(glm::mat4(1.0f), glm::radians(rotaTierra), rotationAxis);
                    break;
                case LUNA:
                    rotaLuna += 1.5f;
                    newTranslationVector = glm::vec3(2.0f, 0.3f, 0.0f);
                    scaleFactor = glm::vec3(0.25f, 0.25f, 0.25f);
                    rotation = glm::rotate(glm::mat4(1.0f), glm::radians(rotaLuna), rotationAxis);
                    break;
                case GASEOSO:
                    rotaGaseoso += 3.0f;
                    newTranslationVector = glm::vec3(4.0f, 0.0f, 0.0f);
                    scaleFactor = glm::vec3(1.75f, 1.75f, 1.75f);
                    rotation = glm::rotate(glm::mat4(1.0f), glm::radians(rotaGaseoso), rotationAxis);
                    break;
            }
            glm::mat4 translation = glm::translate(glm::mat4(1.0f), newTranslationVector);
            glm::mat4 scale = glm::scale(glm::mat4(1.0f), scaleFactor);
            uniforms.model = translation * rotation * scale;
            model.uniforms = uniforms;
        }

        render();

        renderBuffer(renderer);

        frameTime = SDL_GetTicks() - frameStart;

        if (frameTime > 0) {
            std::ostringstream titleStream;
            titleStream << "FPS: " << 1000.0 / frameTime;
            SDL_SetWindowTitle(window, titleStream.str().c_str());
        }
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
