#ifndef OPENGLRBEAUTY_EGLTOOLS
#define OPENGLRBEAUTY_EGLTOOLS

#include "stdint.h"
#include <GLES3/gl3.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include "../util/LogUtil.h"
class EGLTools {

private:
    EGLConfig  m_eglConf;
    EGLSurface m_eglSurface;
    EGLContext m_eglCtx;
    EGLDisplay m_eglDisplay;

public:

    int CreateGlesEnv() {
// EGL config attributes
        const EGLint confAttr[] =
                {
                        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT_KHR,
                        EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,//EGL_WINDOW_BIT EGL_PBUFFER_BIT we will create a pixelbuffer surface
                        EGL_RED_SIZE,   8,
                        EGL_GREEN_SIZE, 8,
                        EGL_BLUE_SIZE,  8,
                        EGL_ALPHA_SIZE, 8,// if you need the alpha channel
                        EGL_DEPTH_SIZE, 16,// if you need the depth buffer
                        EGL_STENCIL_SIZE,8,
                        EGL_NONE
                };

        // EGL context attributes
        const EGLint ctxAttr[] = {
                EGL_CONTEXT_CLIENT_VERSION, 2,
                EGL_NONE
        };

        // surface attributes
        // the surface size is set to the input frame size
        const EGLint surfaceAttr[] = {
                EGL_WIDTH, 1,
                EGL_HEIGHT,1,
                EGL_NONE
        };
        EGLint eglMajVers, eglMinVers;
        EGLint numConfigs;

        int resultCode = 0;
        do
        {
            //1. 获取 EGLDisplay 对象，建立与本地窗口系统的连接
            m_eglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
            if(m_eglDisplay == EGL_NO_DISPLAY)
            {
                //Unable to open connection to local windowing system
                LOGCATE("EGLRender::CreateGlesEnv Unable to open connection to local windowing system");
                resultCode = -1;
                break;
            }

            //2. 初始化 EGL 方法
            if(!eglInitialize(m_eglDisplay, &eglMajVers, &eglMinVers))
            {
                // Unable to initialize EGL. Handle and recover
                LOGCATE("EGLRender::CreateGlesEnv Unable to initialize EGL");
                resultCode = -1;
                break;
            }

            LOGCATE("EGLRender::CreateGlesEnv EGL init with version %d.%d", eglMajVers, eglMinVers);

            //3. 获取 EGLConfig 对象，确定渲染表面的配置信息
            if(!eglChooseConfig(m_eglDisplay, confAttr, &m_eglConf, 1, &numConfigs))
            {
                LOGCATE("EGLRender::CreateGlesEnv some config is wrong");
                resultCode = -1;
                break;
            }

            //4. 创建渲染表面 EGLSurface, 使用 eglCreatePbufferSurface 创建屏幕外渲染区域
            m_eglSurface = eglCreatePbufferSurface(m_eglDisplay, m_eglConf, surfaceAttr);
            if(m_eglSurface == EGL_NO_SURFACE)
            {
                switch(eglGetError())
                {
                    case EGL_BAD_ALLOC:
                        // Not enough resources available. Handle and recover
                        LOGCATE("EGLRender::CreateGlesEnv Not enough resources available");
                        break;
                    case EGL_BAD_CONFIG:
                        // Verify that provided EGLConfig is valid
                        LOGCATE("EGLRender::CreateGlesEnv provided EGLConfig is invalid");
                        break;
                    case EGL_BAD_PARAMETER:
                        // Verify that the EGL_WIDTH and EGL_HEIGHT are
                        // non-negative values
                        LOGCATE("EGLRender::CreateGlesEnv provided EGL_WIDTH and EGL_HEIGHT is invalid");
                        break;
                    case EGL_BAD_MATCH:
                        // Check window and EGLConfig attributes to determine
                        // compatibility and pbuffer-texture parameters
                        LOGCATE("EGLRender::CreateGlesEnv Check window and EGLConfig attributes");
                        break;
                }
            }

            //5. 创建渲染上下文 EGLContext
            m_eglCtx = eglCreateContext(m_eglDisplay, m_eglConf, EGL_NO_CONTEXT, ctxAttr);
            if(m_eglCtx == EGL_NO_CONTEXT)
            {
                EGLint error = eglGetError();
                if(error == EGL_BAD_CONFIG)
                {
                    // Handle error and recover
                    LOGCATE("EGLRender::CreateGlesEnv EGL_BAD_CONFIG");
                    resultCode = -1;
                    break;
                }
            }

            //6. 绑定上下文
            if(!eglMakeCurrent(m_eglDisplay, m_eglSurface, m_eglSurface, m_eglCtx))
            {
                LOGCATE("EGLRender::CreateGlesEnv MakeCurrent failed");
                resultCode = -1;
                break;
            }
            LOGCATE("EGLRender::CreateGlesEnv initialize success!");
        }
        while (false);

        if (resultCode != 0)
        {
            LOGCATE("EGLRender::CreateGlesEnv fail");
        }

        return resultCode;
    }

    void DestroyGlesEnv() {
//8. 释放 EGL 环境
        if (m_eglDisplay != EGL_NO_DISPLAY) {
            eglMakeCurrent(m_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
            eglDestroyContext(m_eglDisplay, m_eglCtx);
            eglDestroySurface(m_eglDisplay, m_eglSurface);
            eglReleaseThread();
            eglTerminate(m_eglDisplay);
        }

        m_eglDisplay = EGL_NO_DISPLAY;
        m_eglSurface = EGL_NO_SURFACE;
        m_eglCtx = EGL_NO_CONTEXT;
    }

};

#endif