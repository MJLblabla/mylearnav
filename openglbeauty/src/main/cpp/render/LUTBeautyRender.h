#ifndef OPENGLRBEAUTY_LUTBEAUTYRENDFER
#define OPENGLRBEAUTY_LUTBEAUTYRENDFER

#include <GLUtils.h>
#include "BaseBeautyRender.h"
#include <glm.hpp>

#define MATH_PI 3.1415926535897932384626433832802


class LUTBeautyRender : public BaseBeautyRender {


    char *fragShape =
           "#version 300 es\n"
           "precision highp float;\n"
           "in vec2 v_texCoord;\n"
           "layout(location = 0) out vec4 outColor;\n"
           "uniform sampler2D s_TextureMap;\n"
           "uniform sampler2D s_LutTexture;\n"
           "\n"
           "\n"
           "\n"
           "vec4 LutFilter(vec2 texCoord)\n"
           "{\n"
           "    //原始采样像素的 RGBA 值\n"
           "    vec4 textureColor =  texture(s_TextureMap, texCoord);\n"
           "\n"
           "    //获取 B 分量值，确定 LUT 小方格的 index, 取值范围转为 0～63\n"
           "    float blueColor = textureColor.b * 63.0;\n"
           "\n"
           "    //取与 B 分量值最接近的 2 个小方格的坐标\n"
           "    vec2 quad1;\n"
           "    quad1.x = floor(floor(blueColor) / 8.0);\n"
           "    quad1.y = floor(blueColor) - (quad1.x * 8.0);\n"
           "\n"
           "    vec2 quad2;\n"
           "    quad2.x = floor(ceil(blueColor) / 7.9999);\n"
           "    quad2.y = ceil(blueColor) - (quad2.x * 8.0);\n"
           "\n"
           "    //通过 R 和 G 分量的值确定小方格内目标映射的 RGB 组合的坐标，然后归一化，转化为纹理坐标。\n"
           " vec2 texPos1;\n"
           "    texPos1.x = (quad1.x * 0.125) + 0.5/512.0 + ((0.125 - 1.0/512.0) * textureColor.r);\n"
           "    texPos1.y = (quad1.y * 0.125) + 0.5/512.0 + ((0.125 - 1.0/512.0) * textureColor.g);\n"
           "\n"
           "    vec2 texPos2;\n"
           "    texPos2.x = (quad2.x * 0.125) + 0.5/512.0 + ((0.125 - 1.0/512.0) * textureColor.r);\n"
           "    texPos2.y = (quad2.y * 0.125) + 0.5/512.0 + ((0.125 - 1.0/512.0) * textureColor.g);"
           "    //取目标映射对应的像素值\n"
           "    vec4 newColor1 = texture(s_LutTexture, texPos1);\n"
           "    vec4 newColor2 = texture(s_LutTexture, texPos2);\n"
           "\n"
           "    //使用 Mix 方法对 2 个边界像素值进行混合\n"
           "    vec4 newColor = mix(newColor1, newColor2, fract(blueColor));\n"
           "    return mix(textureColor, vec4(newColor.rgb, textureColor.w), 1.0);\n"
           "}\n"
           "\n"
           "\n"
           "void main()\n"
           "{\n"
           "    if(v_texCoord.x > 0.5)\n"
           "    {\n"
           "        outColor = LutFilter(v_texCoord);\n"
           "    }\n"
           "    else\n"
           "    {\n"
           "        outColor =  texture(s_TextureMap, v_texCoord);\n"
           "    }\n"
           "\n"
           "}\n"
           "";


    GLint mLUTTextureId = 0;
public:

    char *getFragShaderStr() {
        return fragShape;
    }


    void setLUTTextureId(int lutTextureId) {
        mLUTTextureId = lutTextureId;
    }

    void drawFragShader(int widthsrc, int heightsrc) {
        int locLutTexture = glGetUniformLocation(m_ProgramObj, "s_LutTexture");

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, m_TextureId);
        glBindTexture(GL_TEXTURE_2D, GL_NONE);

        glUniform1i(locLutTexture, 0);
    }
};

#endif