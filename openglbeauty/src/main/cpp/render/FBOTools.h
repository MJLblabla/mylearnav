#ifndef OPENGLRBEAUTY_FBOTOOLS
#define OPENGLRBEAUTY_FBOTOOLS

#include <GLES3/gl3.h>

class FBOTools {

public:

    GLuint textures[1];
    GLuint fbs[1];

    void createFBOTexture(int with, int height) {
        glGenTextures(1, textures);
        glBindTexture(GL_TEXTURE_2D, textures[0]);
        // 根据颜色参数，宽高等信息，为上面的纹理ID，生成一个2D纹理
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, with, height, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                     nullptr);

        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        // 解绑
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    void createFrameBuffer() {
        glGenFramebuffers(1, fbs);
        bindFBO();
        glBindTexture(GL_TEXTURE_2D, textures[0]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textures[0],
                               0);
        // 解绑纹理ID
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    void bindFBO() {
        glBindFramebuffer(GL_FRAMEBUFFER, fbs[0]);
    }

    void unbindFBO() {
        glBindFramebuffer(GL_FRAMEBUFFER, GL_NONE);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    void deleteFBO() {
        //删除Frame Buffer
        glBindFramebuffer(GL_FRAMEBUFFER, GL_NONE);
        glDeleteFramebuffers(1, fbs);
        //删除纹理
        glBindTexture(GL_TEXTURE_2D, 0);
        glDeleteTextures(1, textures);
    }
};

#endif