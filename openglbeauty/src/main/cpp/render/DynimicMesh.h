#ifndef OPENGLRBEAUTY_DynimicMesh
#define OPENGLRBEAUTY_DynimicMesh

#include <GLUtils.h>
#include "BaseBeautyRender.h"
#include <glm.hpp>

#define MATH_PI 3.1415926535897932384626433832802

class DynimicMesh : public BaseBeautyRender {

private:
    char *fragShape = "#version 300 es\n"
                      "precision highp float;\n"
                      "in vec2 v_texCoord;\n"
                      "layout(location = 0) out vec4 outColor;\n"
                      "\n"
                      "uniform sampler2D s_TextureMap;\n"
                      "\n"
                      "uniform float u_Offset;\n"
                      "uniform vec2 u_TexSize;\n"
                      "\n"
                      "\n"
                      "\n"
                      "void main()\n"
                      "{\n"
                      "    vec2 imgTexCoord = v_texCoord * u_TexSize;\n"
                      "    float sideLength = u_TexSize.y / 6.0;\n"
                      "    float maxOffset = 0.08 * sideLength;\n"
                      "    float x = mod(imgTexCoord.x, floor(sideLength));\n"
                      "    float y = mod(imgTexCoord.y, floor(sideLength));\n"
                      "\n"
                      "    float offset = u_Offset * maxOffset;\n"
                      "\n"
                      "    if(offset <= x\n"
                      "    && x <= sideLength - offset\n"
                      "    && offset <= y\n"
                      "    && y <= sideLength - offset)\n"
                      "    {\n"
                      "        outColor = texture(s_TextureMap, v_texCoord);\n"
                      "    }\n"
                      "    else\n"
                      "    {\n"
                      "        outColor = vec4(1.0, 1.0, 1.0, 1.0);\n"
                      "    }\n"
                      "}";

    int m_FrameIndex = 0;

public:

    char *getFragShaderStr() {
        return fragShape;
    }

    void drawFragShader(int widthsrc, int heightsrc) {
        m_FrameIndex++;
        float offset = (sin(m_FrameIndex * MATH_PI / 40) + 1.0f) / 2.0f;
        GLUtils::setFloat(m_ProgramObj, "u_Offset", offset);
        GLUtils::setVec2(m_ProgramObj, "u_TexSize", glm::vec2(widthsrc, heightsrc));
    }
    
};

#endif