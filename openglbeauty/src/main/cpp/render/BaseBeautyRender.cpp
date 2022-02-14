#include "BaseBeautyRender.h"
#include "../util/GLUtils.h"


char vShaderStr[] =
        "#version 300 es                            \n"
        "layout(location = 0) in vec4 a_position;   \n"
        "layout(location = 1) in vec2 a_texCoord;   \n"
        "out vec2 v_texCoord;                       \n"
        "void main()                                \n"
        "{                                          \n"
        "   gl_Position = a_position;               \n"
        "   v_texCoord = a_texCoord;                \n"
        "}                                          \n";

char fShaderStr[] =
        "#version 300 es                                     \n"
        "precision mediump float;                            \n"
        "in vec2 v_texCoord;                                 \n"
        "layout(location = 0) out vec4 outColor;             \n"
        "uniform sampler2D s_TextureMap;                     \n"
        "void main()                                         \n"
        "{                                                   \n"
        "  outColor = texture(s_TextureMap, v_texCoord);     \n"
        "  //outColor = texelFetch(s_TextureMap,  ivec2(int(v_texCoord.x * 404.0), int(v_texCoord.y * 336.0)), 0);\n"
        "}                                                   \n";

static GLfloat verticesCoords[] = {
        -1.0f, 1.0f, 0.0f,  // Position 0
        -1.0f, -1.0f, 0.0f,  // Position 1
        1.0f, -1.0f, 0.0f,  // Position 2
        1.0f, 1.0f, 0.0f,  // Position 3
};


static GLfloat textureCoords[] = {
        0.0f, 0.0f,        // TexCoord 0
        0.0f, 1.0f,        // TexCoord 1
        1.0f, 1.0f,        // TexCoord 2
        1.0f, 0.0f         // TexCoord 3
};

static GLushort indices[] = {0, 1, 2, 0, 2, 3};


BaseBeautyRender::BaseBeautyRender() {
    mEGLTools = new EGLTools();
    mFBOTools = new FBOTools();
}

BaseBeautyRender::~BaseBeautyRender() {
    mFBOTools->deleteFBO();
    mEGLTools->DestroyGlesEnv();
}

void BaseBeautyRender::rendRGBAFrame(uint8_t *rgbaBuffer, int widthsrc, int heightsrc) {
    if (!isCreate) {
        isCreate = true;
        mEGLTools->CreateGlesEnv();
        m_ProgramObj = GLUtils::CreateProgram(vShaderStr, getFragShaderStr(), m_VertexShader,
                                              m_FragmentShader);
        if (m_ProgramObj) {
            m_SamplerLoc = glGetUniformLocation(m_ProgramObj, "s_TextureMap");
        }

        //create RGBA texture
        glGenTextures(1, &m_TextureId);
        glBindTexture(GL_TEXTURE_2D, m_TextureId);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glBindTexture(GL_TEXTURE_2D, GL_NONE);

        //   三个顶点
        glGenBuffers(3, m_VboIds);
        //顶点坐标
        glBindBuffer(GL_ARRAY_BUFFER, m_VboIds[0]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(verticesCoords), verticesCoords, GL_STATIC_DRAW);
        //纹理坐标
        glBindBuffer(GL_ARRAY_BUFFER, m_VboIds[1]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(textureCoords), textureCoords, GL_STATIC_DRAW);
        //绘制顺序
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_VboIds[2]);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

        // Generate VAO Id
        glGenVertexArrays(1, &m_VaoId);
        glBindVertexArray(m_VaoId);

        glBindBuffer(GL_ARRAY_BUFFER, m_VboIds[0]);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (const void *) 0);
        glBindBuffer(GL_ARRAY_BUFFER, GL_NONE);

        glBindBuffer(GL_ARRAY_BUFFER, m_VboIds[1]);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (const void *) 0);
        glBindBuffer(GL_ARRAY_BUFFER, GL_NONE);

        //什么时候用到 glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, indices);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_VboIds[2]);

        glBindVertexArray(GL_NONE);

        //fbo
        mFBOTools->createFBOTexture(widthsrc, heightsrc);
        mFBOTools->createFrameBuffer();
    }

    if (w != widthsrc) {
        glViewport(0, 0, widthsrc, heightsrc);
        glClear(GL_COLOR_BUFFER_BIT);
        w = widthsrc;
        h = heightsrc;
    }

    //upload RGBA image data
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_TextureId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, widthsrc, heightsrc, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                 rgbaBuffer);
    glBindTexture(GL_TEXTURE_2D, GL_NONE);

    mFBOTools->bindFBO();
    //开始绘制
    glUseProgram(m_ProgramObj);
    //设置vao的顶点
    glBindVertexArray(m_VaoId);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_TextureId);
    // Set the RGBA map sampler to texture unit to 0
    glUniform1i(m_SamplerLoc, 0);
    drawFragShader(widthsrc, heightsrc);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, (const void *) 0);
    // uint8_t *pBuffer = static_cast<uint8_t *>(malloc(widthsrc * heightsrc * 4));
      GetRenderFrameFromFBO(rgbaBuffer,widthsrc,heightsrc);
    //int resut =0;
    mFBOTools->unbindFBO();
}

void
BaseBeautyRender::GetRenderFrameFromFBO(uint8_t *pBuffer, int widthsrc, int heightsrc) {
    glReadPixels(0, 0, widthsrc, heightsrc, GL_RGBA, GL_UNSIGNED_BYTE, pBuffer);
}