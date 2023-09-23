#include <vector>
#include "uniforms.h"
#include "fragment.h"
#include "functional"
enum shaderType {
    SOL,
    TIERRA,
    GASEOSO,
    LUNA,
    NAVE,
    ORBIT,
    ANILLOS,
    PLANETAANILLOS,
};

class Model {
    public:
        std::vector<glm::vec3> VBO;
        Uniforms uniforms;
        shaderType currentShader;
        float degreesRotation = 45.0f;
        float radius;
        float speedRotation;
};
