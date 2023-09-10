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

void render() {

    for (auto& model: models){
        // 1. Vertex Shader
        std::vector<Vertex> transformedVertices(model.VBO.size() / 3);
        for (size_t i = 0; i < model.VBO.size() / 3; ++i) {
            Vertex vertex = { model.VBO[i * 3], model.VBO[i * 3 + 1], model.VBO[i * 3 + 2] };
            transformedVertices[i] = vertexShader(vertex, model.uniforms);
        }

        // 2. Primitive Assembly
        std::vector<std::vector<Vertex>> assembledVertices(transformedVertices.size() / 3);
        for (size_t i = 0; i < transformedVertices.size() / 3; ++i) {
            Vertex edge1 = transformedVertices[3 * i];
            Vertex edge2 = transformedVertices[3 * i + 1];
            Vertex edge3 = transformedVertices[3 * i + 2];
            assembledVertices[i] = { edge1, edge2, edge3 };
        }

        // 3. Rasterization
        std::vector<Fragment> fragments;
        for (size_t i = 0; i < assembledVertices.size(); ++i) {
            std::vector<Fragment> rasterizedTriangle = triangle(
                    assembledVertices[i][0],
                    assembledVertices[i][1],
                    assembledVertices[i][2]
            );
            fragments.insert(fragments.end(), rasterizedTriangle.begin(), rasterizedTriangle.end());
        }

        // 4. Fragment Shader
        for (size_t i = 0; i < fragments.size(); ++i) {
            Fragment& fragment = fragments[i];
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
    camera.cameraPosition = glm::vec3(0.0f, 0.0f, 3.0f);
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
    sol.modelMatrix = glm::mat4(1.0f);

    // models.push_back(sol);

    Model tierra;
    tierra.VBO = vertexBufferObject;
    tierra.currentShader = TIERRA;
    tierra.uniforms = uniforms;
    tierra.modelMatrix = glm::mat4(1.0f);

    models.push_back(tierra);

    Model luna;
    luna.VBO = vertexBufferObject;
    luna.currentShader = LUNA;
    luna.uniforms = uniforms;
    luna.modelMatrix = glm::mat4(1.0f);

    models.push_back(luna);

    // Posicion de los astros
    glm::vec3 newTranslationVector(0.0f, 0.0f, 0.0f);

    // Rotacion de los astros
    float rotaSol = 45.0f;
    float rotaTierra = 45.0f;
    float rotaLuna = 45.0f;
    glm::vec3 rotationAxis(0.0f, 1.0f, 0.0f);

    // Tamaño de los astros
    glm::vec3 scaleFactor(1.0f, 1.0f, 1.0f);

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
                        camera.cameraPosition.x += -speed;
                        break;
                    case SDLK_RIGHT:
                        camera.cameraPosition.x += speed;
                        break;
                    case SDLK_UP:
                        camera.cameraPosition.y += -speed;
                        break;
                    case SDLK_DOWN:
                        camera.cameraPosition.y += speed;
                        break;
                }
            }
        }

        uniforms.view = glm::lookAt(
                camera.cameraPosition,
                camera.targetPosition,
                camera.upVector
        );

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        clearFramebuffer();
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
                    newTranslationVector = glm::vec3(0.0f, 0.0f, 0.0f);
                    // newTranslationVector = glm::vec3(1.5f, 0.0f, 0.0f);
                    scaleFactor = glm::vec3(0.5f, 0.5f, 0.5f);
                    rotation = glm::rotate(glm::mat4(1.0f), glm::radians(rotaTierra), rotationAxis);
                    break;
                case LUNA:
                    rotaLuna += 1.5f;
                    newTranslationVector = glm::vec3(0.5f, 0.3f, 0.0f);
                    // newTranslationVector = glm::vec3(2.0f, 0.3f, 0.0f);
                    scaleFactor = glm::vec3(0.25f, 0.25f, 0.25f);
                    rotation = glm::rotate(glm::mat4(1.0f), glm::radians(rotaLuna), rotationAxis);
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
