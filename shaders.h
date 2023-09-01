#pragma once
#include "glm/geometric.hpp"
#include "glm/glm.hpp"
#include "uniforms.h"
#include "fragment.h"
#include "noise.h"
#include "print.h"

Vertex vertexShader(const Vertex& vertex, const Uniforms& uniforms) {
    // Apply transformations to the input vertex using the matrices from the uniforms
    glm::vec4 clipSpaceVertex = uniforms.projection * uniforms.view * uniforms.model * glm::vec4(vertex.position, 1.0f);

    // Perspective divide
    glm::vec3 ndcVertex = glm::vec3(clipSpaceVertex) / clipSpaceVertex.w;

    // Apply the viewport transform
    glm::vec4 screenVertex = uniforms.viewport * glm::vec4(ndcVertex, 1.0f);
    
    // Transform the normal
    glm::vec3 transformedNormal = glm::mat3(uniforms.model) * vertex.normal;
    transformedNormal = glm::normalize(transformedNormal);

    glm::vec3 transformedWorldPosition = glm::vec3(uniforms.model * glm::vec4(vertex.position, 1.0f));

    // Return the transformed vertex as a vec3
    return Vertex{
        glm::vec3(screenVertex),
        transformedNormal,
        vertex.tex,
        transformedWorldPosition,
        vertex.position
    };
}

Fragment fragmentShader(Fragment& fragment) {
    glm::vec3 sunColor1 = glm::vec3(252.0f / 255.0f, 211.0f / 255.0f, 0.0f / 255.0f);
    glm::vec3 sunColor2 = glm::vec3(252.0f / 255.0f, 163.0f / 255.0f, 0.0f / 255.0f);

    // Sample the Perlin noise map at the fragment's position
    glm::vec2 uv = glm::vec2(fragment.originalPos.x, fragment.originalPos.z);
    uv = glm::clamp(uv, 0.0f, 1.0f);  // Ensure UV coordinates are in [0, 1] range

    // Set up the noise generator
    FastNoiseLite noiseGenerator;
    noiseGenerator.SetNoiseType(FastNoiseLite::NoiseType_Perlin);

    float offsetX = 10000.0f;
    float offsetY = 10000.0f;
    float scale = 900.0f;

    // Generate the noise value
    float noiseValue = noiseGenerator.GetNoise((uv.x + offsetX) * scale, (uv.y + offsetY) * scale);

    // Map the noise value to a smooth gradient between sunColor1 and sunColor2
    float t = (noiseValue + 1.0f) * 0.5f; // Map [-1, 1] to [0, 1]
    glm::vec3 finalColor = glm::mix(sunColor1, sunColor2, t);

    // Convert glm::vec3 color to your Color class
    fragment.color = Color(finalColor.r, finalColor.g, finalColor.b);

    return fragment;
}

