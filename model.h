#include <vector>
#include "uniforms.h"
#include "fragment.h"
#include "functional"
enum shaderType {
    SOL,
    TIERRA,
    GASEOSO,
    LUNA,
    ANILLOS,
    PLANETA_ANILLOS,
    SOL_AMARILLO,
};

class Model {
    public:
        glm::mat4 modelMatrix;
        std::vector<glm::vec3> VBO;
        Uniforms uniforms;
        shaderType currentShader;
};
