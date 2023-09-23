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

// Helper to convert HSV to RGB
glm::vec3 hsv2rgb(glm::vec3 c) {

    glm::vec4 K = glm::vec4(1.0f, 2.0f/3.0f, 1.0f/3.0f, 3.0f);

    glm::vec3 p;
    p.x = abs(static_cast<float>(glm::fract(c.x + K.x) * 6.0 - K.w));
    p.y = abs(static_cast<float>(glm::fract(c.x + K.y) * 6.0 - K.w));
    p.z = abs(static_cast<float>(glm::fract(c.x + K.z) * 6.0 - K.w));

    return c.z * glm::mix(glm::vec3(K.x), glm::clamp(p - glm::vec3(K.x), 0.0f, 1.0f), c.y);

}

// Shader fragment function
Fragment sol(Fragment& fragment) {

    // Get UV coordinates
    glm::vec2 uv = glm::vec2(fragment.originalPos.x, fragment.originalPos.y / fragment.originalPos.z + 0.5f);
    // glm::vec2 uv = glm::vec2(fragment.originalPos.x, fragment.originalPos.y);
    // uv = uv * fragment.originalPos.x;

    // Generate Perlin noise
    FastNoiseLite noiseGenerator;
    noiseGenerator.SetNoiseType(FastNoiseLite::NoiseType_Perlin);

    float offsetX = 10000.0f;
    float offsetY = 10000.0f;
    float scale = 9000.0f;

    float noiseValue = noiseGenerator.GetNoise((uv.x + offsetX) * scale,
                                               (uv.y + offsetY) * scale);

    // Map noise range [-1, 1] to hue shift [0, 0.1]
    float hueShift = noiseValue * 0.1f;

    // Create HSV color with shifted hue
    glm::vec3 hsv = glm::vec3(hueShift, 1.0f, 1.0f);

    // Convert HSV to RGB
    glm::vec3 rgb = hsv2rgb(hsv);

    // Set final fragment color
    fragment.color = Color(rgb.r, rgb.g, rgb.b);

    return fragment;
}

Fragment solAmarillo(Fragment& fragment) {
    glm::vec3 sunColor1 = glm::vec3(252.0f / 255.0f, 211.0f / 255.0f, 0.0f / 255.0f);
    glm::vec3 sunColor2 = glm::vec3(252.0f / 255.0f, 163.0f / 255.0f, 0.0f / 255.0f);

    // Sample the Perlin noise map at the fragment's position
    glm::vec2 uv = glm::vec2(fragment.originalPos.x, fragment.originalPos.y * 5.0f);

    // Set up the noise generator
    FastNoiseLite noiseGenerator;
    noiseGenerator.SetNoiseType(FastNoiseLite::NoiseType_Perlin);

    float offsetX = 10000.0f;
    float offsetY = 10000.0f;
    float scale = 9000.0f;

    // Generate the noise value
    float noiseValue = noiseGenerator.GetNoise((uv.x + offsetX) * scale, (uv.y + offsetY) * scale);

    // Map the noise value to a smooth gradient between sunColor1 and sunColor2
    float t = glm::smoothstep(-1.0f, 1.0f, noiseValue); // Map [-1, 1] to [0, 1]
    glm::vec3 finalColor = glm::mix(sunColor1, sunColor2, t);

    // Convert glm::vec3 color to your Color class
    fragment.color = Color(finalColor.r, finalColor.g, finalColor.b);

    return fragment;
}

Fragment tierra(Fragment& fragment) {
    Color color;

    glm::vec3 groundColor = glm::vec3(0.44f, 0.51f, 0.33f);
    glm::vec3 groudColor2 = glm::vec3(0.97f, 0.53f, 0.18f);
    glm::vec3 oceanColor = glm::vec3(0.12f, 0.38f, 0.57f);
    glm::vec3 cloudColor = glm::vec3(1.0f, 1.0f, 1.0f);

    glm::vec3 uv = glm::vec3(fragment.originalPos.x, fragment.originalPos.y, fragment.originalPos.z);

    FastNoiseLite noiseGenerator;
    noiseGenerator.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);

    FastNoiseLite noiseGenerator2;
    noiseGenerator2.SetNoiseType(FastNoiseLite::NoiseType_Perlin);

    float ox = 1200.0f;
    float oy = 3000.0f;
    float oz = 3000.0f;
    float zoom = 400.0f;

    float noiseValue = noiseGenerator.GetNoise((uv.x + ox) * zoom, (uv.y + oy) * zoom, (uv.z + oz) * zoom);

    float oxg = 5500.0f;
    float oyg = 6900.0f;
    float ozg = 3000.0f;
    float zoomg = 900.0f;

    float noiseValueG = noiseGenerator2.GetNoise((uv.x + oxg) * zoomg, (uv.y + oyg) * zoomg, (uv.z + ozg) * zoom);

    glm::vec3 tmpColor;
    if (noiseValue < 0.5f) {
        tmpColor = oceanColor;
    } else {
        tmpColor = groundColor;
        if (noiseValueG < 0.1f) {
            float t = (noiseValueG + 1.0f) * 0.5f; // Map [-1, 1] to [0, 1]
            tmpColor = glm::mix(groundColor, groudColor2, t);
        }
    }

    float oxc = 5500.0f;
    float oyc = 6900.0f;
    float oyz = 3000.0f;
    float zoomc = 300.0f;

    float noiseValueC = noiseGenerator.GetNoise((uv.x + oxc) * zoomc, (uv.y + oyc) * zoomc, (uv.z + oyz) * zoom);

    if (noiseValueC > 0.5f) {
        float t = (noiseValueC - 0.5f) * 2.0f; // Map [-1, 1] to [0, 1]
        tmpColor = glm::mix(tmpColor, cloudColor, t);
    }

    color = Color(tmpColor.x, tmpColor.y, tmpColor.z);

    if (fragment.intensity < 0.5f){
        fragment.intensity = glm::mix(fragment.intensity, 0.5f, 0.2f);
    }

    fragment.color = color * fragment.intensity;

    return fragment;
}

Fragment gaseoso(Fragment& fragment) {
    Color color;

    glm::vec3 mainColor = glm::vec3(163.0f/255.0f, 135.0f/255.0f, 115.0f/255.0f);
    glm::vec3 secondColor = glm::vec3(248.0f/255.0f, 213.0f/255.0f, 183.0f/255.0f);

    glm::vec2 uv = glm::vec2(fragment.originalPos.x * 2.0 - 1.0 , fragment.originalPos.y * 2.0 - 1.0);

    // Frecuencia y amplitud de las ondas en el planeta
    float frequency = 15.0; // Ajusta la frecuencia de las líneas
    float amplitude = 0.1; // Ajusta la amplitud de las líneas

    FastNoiseLite noiseGenerator;
    noiseGenerator.SetNoiseType(FastNoiseLite::NoiseType_Cellular);

    float offsetX = 10000.0f;
    float offsetY = 10000.0f;
    float scale = 900.0f;

    // Genera el valor de ruido
    float noiseValue = noiseGenerator.GetNoise((uv.x + offsetX) * scale, (uv.y + offsetY) * scale);
    noiseValue = (noiseValue + 1.0f) * 0.5f; // Map [-1, 1] to [0, 1]

    // Calcula el valor sinusoide para crear líneas
    float sinValue = glm::sin(uv.y * frequency) * amplitude;

    // Combina el color base con las líneas sinusoide
    secondColor = mainColor + glm::vec3 (sinValue);

    color = Color(secondColor.x, secondColor.y, secondColor.z);

    if (fragment.intensity < 0.5f){
        fragment.intensity = glm::mix(fragment.intensity, 0.5f, 0.2f);
    }

    fragment.color = color * fragment.intensity;

    return fragment;
}

Fragment luna(Fragment& fragment) {
    Color color;

    glm::vec3 moonColor = glm::vec3(0.8f, 0.8f, 0.8f); // Color de la luna

    glm::vec3 uv = glm::vec3(fragment.originalPos.x * 2.0 - 1.0,
                             fragment.originalPos.y * 2.0 - 1.0,
                             fragment.originalPos.z);

    // Frecuencia y amplitud de las texturas para simular la superficie rugosa
    float amplitude = 0.1; // Ajusta la amplitud de las texturas

    FastNoiseLite noiseGenerator;
    noiseGenerator.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);

    float offsetX = 5000.0f;
    float offsetY = 8000.0f;
    float scale = 500.0f;

    // Genera el valor de ruido para la superficie rugosa
    float noiseValue = noiseGenerator.GetNoise((uv.x + offsetX) * scale, (uv.y + offsetY) * scale, uv.z);
    noiseValue = (noiseValue + 1.0f) * 0.5f; // Mapea [-1, 1] a [0, 1]

    // Combina el color de la luna con las texturas rugosas
    moonColor = glm::mix(moonColor, glm::vec3(0.6f, 0.6f, 0.6f), noiseValue * amplitude * 5.0f);

    color = Color(moonColor.x, moonColor.y, moonColor.z);

    if (fragment.intensity < 0.5f){
        fragment.intensity = glm::mix(fragment.intensity, 0.5f, 0.2f);
    }

    fragment.color = color * fragment.intensity;

    return fragment;
}

Fragment anillos(Fragment& fragment) {
    Color color;
    glm::vec3 mainColor = glm::vec3 (0.0f, 188.0f/ 255.0f, 159.0f/255.0f);
    glm::vec3 secondColor = glm::vec3(51.0f, 108.0f/ 255.0f, 99.0f/255.0f);
    FastNoiseLite noiseGenerator;
    noiseGenerator.SetNoiseType(FastNoiseLite::NoiseType_Perlin);

    glm::vec3 uv = glm::vec3(fragment.originalPos.x * 2.0 - 1.0,
                             fragment.originalPos.y * 2.0 - 1.0,
                             fragment.originalPos.z);

    // Frecuencia y amplitud de las texturas para simular la superficie rugosa
    float amplitude = 0.1; // Ajusta la amplitud de las texturas

    float offsetX = 5000.0f;
    float offsetY = 8000.0f;
    float offsetZ = 300.0f;
    float scale = 500.0f;

    float noiseValue = noiseGenerator.GetNoise((uv.x + offsetX) * scale, (uv.y + offsetY) * scale, (uv.z + offsetZ) * scale);
    noiseValue = (noiseValue + 1.0f) * 0.5f; // Mapea [-1, 1] a [0, 1]

    mainColor = glm::mix(mainColor, secondColor, noiseValue * amplitude * 5.0f);


    color = Color(mainColor.x, mainColor.y, mainColor.z);

    if (fragment.intensity < 0.5f){
        fragment.intensity = glm::mix(fragment.intensity, 0.5f, 0.2f);
    }

    fragment.color = color * fragment.intensity;

    return fragment;
}

Fragment platenaAnillos(Fragment& fragment) {
    Color color;

    glm::vec3 mainColor = glm::vec3 (0.0f, 188.0f/ 255.0f, 159.0f/255.0f);
    glm::vec3 secondColor = glm::vec3(51.0f, 108.0f/ 255.0f, 99.0f/255.0f);

    glm::vec2 uv = glm::vec2(fragment.originalPos.x * 2.0 - 1.0 , fragment.originalPos.y * 2.0 - 1.0);

    // Frecuencia y amplitud de las ondas en el planeta
    float frequency = 15.0; // Ajusta la frecuencia de las líneas
    float amplitude = 0.1; // Ajusta la amplitud de las líneas

    FastNoiseLite noiseGenerator;
    noiseGenerator.SetNoiseType(FastNoiseLite::NoiseType_Cellular);

    float offsetX = 10000.0f;
    float offsetY = 10000.0f;
    float scale = 900.0f;

    // Genera el valor de ruido
    float noiseValue = noiseGenerator.GetNoise((uv.x + offsetX) * scale, (uv.y + offsetY) * scale);
    noiseValue = (noiseValue + 1.0f) * 0.5f; // Map [-1, 1] to [0, 1]

    // Calcula el valor sinusoide para crear líneas
    float sinValue = glm::sin(uv.y * frequency) * amplitude;

    // Combina el color base con las líneas sinusoide
    secondColor = mainColor + glm::vec3 (sinValue);

    color = Color(secondColor.x, secondColor.y, secondColor.z);

    if (fragment.intensity < 0.5f){
        fragment.intensity = glm::mix(fragment.intensity, 0.5f, 0.2f);
    }

    fragment.color = color * fragment.intensity;

    return fragment;
}


