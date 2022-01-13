/*** COPYRIGHT FUJITSU LIMITED 2017-2018 ***/
#ifndef MFTLHOPENGL_H
#define MFTLHOPENGL_H

typedef enum {
    GLResult_Success,
    GLResult_WAYError,
    GLResult_EGLError,
    GLResult_GLError
} mf_TLH_OpenGLResult;

#define Touch_Begin 1
#define Touch_Update 2
#define Touch_End 3

typedef void (*mf_TLH_TouchListener)(int type, int x1, int y1, int x2, int y2);


#ifdef __cplusplus
extern "C" {
#endif

void *allocateOpenGLInfo();
void freeOpenGLInfo( void *openglinfo );
mf_TLH_OpenGLResult openglinit(void *openglinfo , unsigned int ilmSurfaceId,
                               int windowWidth, int windowHeight);
void openglSetTouchListener( void *openglinfo, mf_TLH_TouchListener listener);
int openglGetEventFd( void *openglinfo);
unsigned long  getSurfaceID( void *openglinfo );
mf_TLH_OpenGLResult getOpenGLResult( void *openglinfo );
unsigned short getDetailFunc( void *openglinfo );
unsigned short getDetailResult( void *openglinfo );
void handleEvent(void *param);
mf_TLH_OpenGLResult checkOpenGLAsyncError( void *openglinfo );
void openglDestroySurface(void *param);
mf_TLH_OpenGLResult createEGLSurface(void *openglinfo, void *config);
void *openglGetEglDisplay(void *openglinfo);
void *openglGetEglSurface(void *openglinfo);
void openglSetMinInterval(void *openglinfo);
void openglFlush(void *openglinfo);

#ifdef __cplusplus
}
#endif


#endif // MFTLHOPENGL_H
