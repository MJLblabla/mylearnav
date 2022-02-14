#ifndef OPENGLRBEAUTY_BASEBEAUTYRENDER
#define OPENGLRBEAUTY_BASEBEAUTYRENDER

#include <GLES3/gl3.h>
#include "FBOTools.h"
#include "FBOTools.h"
#include "EGLTools.h"

class BaseBeautyRender {


private:
    bool isCreate = false;

    void GetRenderFrameFromFBO(uint8_t *pBuffer, int widthsrc, int heightsrc);

    FBOTools *mFBOTools = nullptr;
    EGLTools *mEGLTools = nullptr;
    int w = -1;
    int h = -1;
protected:
    GLuint m_ProgramObj = GL_NONE;
    GLuint m_SamplerLoc = GL_NONE;
    GLuint m_VertexShader;
    GLuint m_FragmentShader;
    GLuint m_VaoId = GL_NONE;
    GLuint m_VboIds[3];
    GLuint m_TextureId;


public:

    BaseBeautyRender();

    ~BaseBeautyRender();

    void rendRGBAFrame(uint8_t *rgbaBuffer, int widthsrc, int heightsrc);

    virtual char *getFragShaderStr() = 0;

    virtual void drawFragShader(int widthsrc, int heightsrc) = 0;
};

#endif