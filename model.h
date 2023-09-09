#include <vector>
#include "uniforms.h"
#include "fragment.h"
#include "functional"

class Model {
    public:
        glm::mat4 modelMatrix;
        std::vector<glm::vec3> VBO;
        Uniforms uniforms;
        std::function<Fragment(Fragment&)> fshader;
};
