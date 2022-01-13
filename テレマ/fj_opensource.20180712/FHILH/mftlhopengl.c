/*** COPYRIGHT FUJITSU LIMITED 2016-2018 ***/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <sys/time.h>
#include <signal.h>
#ifdef MFTLH_PLATFORM_UBUNTU
#include <linux/input.h>
#endif

#include <EGL/egl.h>
#include <GLES2/gl2.h>

#include <wayland-client.h>
#include <wayland-egl.h>

#ifdef MFTLH_PLATFORM_TARGET
#include "ivi-application-client-protocol.h"
#endif

#ifdef MFTLHDEBUGTRACE
#include <time.h>
#endif

#include "mftlhopengl.h"

///
//  Macros
//
#define ESUTIL_API
#define ESCALLBACK

typedef enum {
    WAYFunc_NoFunc                   = 0x00,
    WAYFunc_displayConnect           = 0x01,
    WAYFunc_displayGetRegistry       = 0x02,
    WAYFunc_registryAddListener      = 0x03,
    WAYFunc_displayDispatch          = 0x04,
    WAYFunc_registryBind             = 0x05,
    WAYFunc_seatAddListener          = 0x06,
    WAYFunc_compositorCreateSurface  = 0x07,
    WAYFunc_seatGetTouch             = 0x08,
    WAYFunc_touchAddListener         = 0x09,
    IVIFunc_surfaceWarning           = 0x0a,
    WAYFunc_flush                    = 0x0b
} mf_TLH_WAYFunc;

typedef enum {
    EGLFunc_NoFunc                   = 0x00,
    EGLFunc_eglChooseConfig          = 0x01,
    EGLFunc_eglCreateContext         = 0x02,
    EGLFunc_eglCreatePixmapSurface   = 0x03,
    EGLFunc_eglGetConfigs            = 0x04,
    EGLFunc_eglGetDisplay            = 0x05,
    EGLFunc_eglInitialize            = 0x06,
    EGLFunc_eglMakeCurrent           = 0x07,
    EGLFunc_eglSwapBuffers           = 0x08,
    EGLFunc_eglGetConfigAttrib       = 0x09,
    EGLFunc_eglSwapInterval          = 0x0a,
    EGLFunc_eglBindAPI               = 0x0b,
    EGLFunc_eglCreateWindowSurface   = 0x0c
} mf_TLH_EGLFunc;

typedef enum {
    GLFunc_NoFunc                    = 0x00,
    GLFunc_glActiveTexture           = 0x01,
    GLFunc_glAttachShader            = 0x02,
    GLFunc_glBindTexture             = 0x03,
    GLFunc_glCompileShader           = 0x04,
    GLFunc_glCreateProgram           = 0x05,
    GLFunc_glCreateShader            = 0x06,
    GLFunc_glDrawElements            = 0x07,
    GLFunc_glEnableVertexAttribArray = 0x08,
    GLFunc_glFlush                   = 0x09,
    GLFunc_glGenTextures             = 0x0a,
    GLFunc_glGetAttribLocation       = 0x0b,
    GLFunc_glGetProgramiv            = 0x0c,
    GLFunc_glGetShaderiv             = 0x0d,
    GLFunc_glGetUniformLocation      = 0x0e,
    GLFunc_glLinkProgram             = 0x0f,
    GLFunc_glPixelStorei             = 0x10,
    GLFunc_glTexImage2D              = 0x11,
    GLFunc_glTexSubImage2D           = 0x12,
    GLFunc_glTexParameteri           = 0x13,
    GLFunc_glShaderSource            = 0x14,
    GLFunc_glUniform1i               = 0x15,
    GLFunc_glUseProgram              = 0x16,
    GLFunc_glVertexAttribPointer     = 0x17,
    GLFunc_glViewport                = 0x18,
    GLFunc_glUniformMatrix4fv        = 0x19
} mf_TLH_GLFunc;


typedef struct {

	int m_windowWidth;
	int m_windowHeight;

    EGLNativeWindowType m_graSurface;
    unsigned long m_surfaceId;
    EGLDisplay m_eglDisplay;
    EGLSurface m_eglSurface;
    EGLConfig m_eglConfig;

    mf_TLH_OpenGLResult m_tlhResult;
    unsigned short m_detailFunc;
    unsigned short m_detailResult;

    uint32_t iviSurfaceId;
    struct wl_display *wlDisplay;
    struct wl_registry *wlRegistry;
    struct wl_compositor *wlCompositor;
    struct wl_surface *wlSurface;
    struct wl_seat *wlSeat;
    struct wl_touch *wlTouch;
#ifdef MFTLH_PLATFORM_TARGET
    struct ivi_application *iviApplication;
    struct ivi_surface *iviSurface;
    int32_t ivi_warningCode;
#endif
    struct wl_egl_window *eglWindowNative;
    int32_t touchId[2];
    int touchX[2];
    int touchY[2];
    mf_TLH_TouchListener touchListener;
    struct wl_shell_surface *wlShellSurface;
    struct wl_shell *wlShell;
#ifdef MFTLH_PLATFORM_UBUNTU
    /* ubuntuデバッグ用 */
    struct wl_pointer *wlPointer;
#endif
} mf_TLH_OpenGLInfo;


#define TOUCHNO_NONE -1
#define TOUCHNO_1ST 0
#define TOUCHNO_2ND 1
#define TOUCHID_BLANK -1
#define TOUCHID_DUMMY 1
#define TOUCHPOS_BLANK -1

#ifdef MFTLH_PLATFORM_UBUNTU
wl_fixed_t g_pointerX;
wl_fixed_t g_pointerY;
#endif

#ifdef MFTLHDEBUGTRACE
void PrintLog(const char *aLog);
#endif

mf_TLH_OpenGLResult getOpenGLResult( void *openglinfo )
{
    return ((mf_TLH_OpenGLInfo *)openglinfo)->m_tlhResult;
}

unsigned short getDetailFunc( void *openglinfo )
{
    return ((mf_TLH_OpenGLInfo *)openglinfo)->m_detailFunc;
}

unsigned short getDetailResult( void *openglinfo )
{
    return ((mf_TLH_OpenGLInfo *)openglinfo)->m_detailResult;
}

mf_TLH_OpenGLResult createEGLSurface(void *openglinfo, void *config)
{
    mf_TLH_OpenGLInfo *info = (mf_TLH_OpenGLInfo *)openglinfo;
    EGLSurface eglSurface = eglCreateWindowSurface(info->m_eglDisplay, (EGLConfig)config, 
												(EGLNativeWindowType)(info->eglWindowNative), NULL);
    if (eglSurface == EGL_NO_SURFACE)
    {
        eglTerminate(info->m_eglDisplay);
        info->m_tlhResult = GLResult_EGLError;
        info->m_detailFunc = EGLFunc_eglCreateWindowSurface;
        info->m_detailResult = 0;
        return info->m_tlhResult;
    }
    info->m_eglConfig = (EGLConfig)config;
    info->m_eglSurface = eglSurface;

    return GLResult_Success;
}


/****************************************************************************
 *  説明   : Waylandサーフェイスハンドラ
 ****************************************************************************/
static void handlePing(void *data, struct wl_shell_surface *shell_surface, uint32_t serial)
{
    wl_shell_surface_pong(shell_surface, serial);
}

static void handleConfigureDummy(void *data, struct wl_shell_surface *shell_surface,
                 uint32_t edges, int32_t width, int32_t height)
{
    /* DO NOTHING */
}

static void handlePopupDoneDummy(void *data, struct wl_shell_surface *shell_surface)
{
    /* DO NOTHING */
}

static struct wl_shell_surface_listener wlShellSurfaceListener = {
    handlePing,
    handleConfigureDummy,
    handlePopupDoneDummy
};

void touchProcess(mf_TLH_OpenGLInfo *info, int type, int32_t id, wl_fixed_t sx, wl_fixed_t sy)
{
    int sendType = Touch_End;
    int idNo = TOUCHNO_NONE;
    int lastX[2] = {TOUCHPOS_BLANK, TOUCHPOS_BLANK};
    int lastY[2] = {TOUCHPOS_BLANK, TOUCHPOS_BLANK};

    /* タッチ終了イベント */
    if (Touch_End == type)
    {
        /* どこかの指が離れた */
        if (info->touchId[TOUCHNO_1ST] == id)
        {
            /* 1ST指 */
            idNo = TOUCHNO_1ST;
            if (TOUCHID_BLANK == info->touchId[TOUCHNO_2ND])
            {
                /* 指が全部離れた */
                sendType = Touch_End;
                /* 離した座標を残す */
                lastX[TOUCHNO_1ST] = info->touchX[TOUCHNO_1ST];
                lastY[TOUCHNO_1ST] = info->touchY[TOUCHNO_1ST];
                lastX[TOUCHNO_2ND] = info->touchX[TOUCHNO_2ND];
                lastY[TOUCHNO_2ND] = info->touchY[TOUCHNO_2ND];
            }
            else
            {
                /* まだ指が残っているのでUpdate扱い */
                sendType = Touch_Update;
            }
        }
        else if (info->touchId[TOUCHNO_2ND] == id)
        {
            /* 2ND指 */
            idNo = TOUCHNO_2ND;
            if (TOUCHID_BLANK == info->touchId[TOUCHNO_1ST])
            {
                /* 指が全部離れた */
                sendType = Touch_End;
                /* 離した座標を残す */
                lastX[TOUCHNO_1ST] = info->touchX[TOUCHNO_1ST];
                lastY[TOUCHNO_1ST] = info->touchY[TOUCHNO_1ST];
                lastX[TOUCHNO_2ND] = info->touchX[TOUCHNO_2ND];
                lastY[TOUCHNO_2ND] = info->touchY[TOUCHNO_2ND];
            }
            else
            {
                /* まだ指が残っているのでUpdate扱い */
                sendType = Touch_Update;
            }
        }
        else
        {
            /* 2ND以降の指の操作は無視 */
            return;
        }
        if ((Touch_Update == sendType) && (TOUCHNO_1ST == idNo))
        {
            /* 1STが空いたので2NDを繰り上げて空ける */
            info->touchId[TOUCHNO_1ST] = info->touchId[TOUCHNO_2ND];
            info->touchX[TOUCHNO_1ST] = info->touchX[TOUCHNO_2ND];
            info->touchY[TOUCHNO_1ST] = info->touchY[TOUCHNO_2ND];
            idNo = TOUCHNO_2ND;
        }
        info->touchId[idNo] = TOUCHID_BLANK;
        info->touchX[idNo] = TOUCHPOS_BLANK;
        info->touchY[idNo] = TOUCHPOS_BLANK;
    }
    /* タッチ開始または更新イベント */
    else
    {
        /* タッチ開始 */
        if (Touch_Begin == type)
        {
            /* 既存のIDの場合(原則ありえないがフェイルセーフのため)*/
            if (info->touchId[TOUCHNO_1ST] == id)
            {
                /* 1ST指の場合はUpdate扱い */
                idNo = TOUCHNO_1ST;
                sendType = Touch_Update;
            }
            else if (info->touchId[TOUCHNO_2ND] == id)
            {
                /* 2ND指の場合はUpdate扱い */
                idNo = TOUCHNO_2ND;
                sendType = Touch_Update;
            }
            else
            {
                /* 新規のID */

                /* 座標変換 */
                int x = wl_fixed_to_int(sx);
                int y = wl_fixed_to_int(sy);

                /* 座標が自サーフェイス外なら無視 */
                if (((x < 0) || (info->m_windowWidth  <= x)) ||
                    ((y < 0) || (info->m_windowHeight <= y))   )
                {
                    return;
                }

                if (TOUCHID_BLANK == info->touchId[TOUCHNO_1ST])
                {
                    /* 1STまたは2NDでなく1STが空いている場合はBegin扱い */
                    idNo = TOUCHNO_1ST;
                    info->touchId[idNo] = id;
                    sendType = Touch_Begin;
                }
                else if (TOUCHID_BLANK == info->touchId[TOUCHNO_2ND])
                {
                    /* 1STまたは2NDでなく2NDが空いている場合はBegin扱い */
                    idNo = TOUCHNO_2ND;
                    info->touchId[idNo] = id;
                    sendType = Touch_Begin;
                }
                else
                {
                    /* 2ND以降の指の操作は無視 */
                    return;
                }
                info->touchX[idNo] = x;
                info->touchY[idNo] = y;
            }
        }
        /* タッチ更新 */
        else if (Touch_Update == type)
        {
            /* どこかの指が動いた */
            if (info->touchId[TOUCHNO_1ST] == id)
            {
                /* 1ST指 */
                idNo = TOUCHNO_1ST;
                sendType = Touch_Update;
            }
            else if (info->touchId[TOUCHNO_2ND] == id)
            {
                /* 2ND指 */
                idNo = TOUCHNO_2ND;
                sendType = Touch_Update;
            }
            else
            {
                /* 2ND以降の指の操作は無視 */
                return;
            }
        }
        else
        {
            /* 未定義のタッチ操作は処理しない */
            return;
        }

        /* タッチ更新扱いの場合はサーフェイス外の座標を補正する */
        if (Touch_Update == sendType)
        {
            /* 座標変換 */
            int x = wl_fixed_to_int(sx);
            int y = wl_fixed_to_int(sy);

            /* X座標補正 */
            if (x < 0)
            {
                info->touchX[idNo] = 0;
            }
            else if (info->m_windowWidth <= x)
            {
                info->touchX[idNo] = info->m_windowWidth-1;
            }
            else
            {
                info->touchX[idNo] = x;
            }

            /* Y座標補正 */
            if (y < 0)
            {
                info->touchY[idNo] = 0;
            }
            else if (info->m_windowHeight <= y)
            {
                info->touchY[idNo] = info->m_windowHeight-1;
            }
            else
            {
                info->touchY[idNo] = y;
            }
        }
    }

    if (NULL != info->touchListener)
    {
        /* タッチリスナーを呼び出す */
        if (Touch_End == sendType)
        {
            /* 指がすべて離れた場合は最後の座標を通知 */
            (info->touchListener)(sendType,
                                  lastX[TOUCHNO_1ST], lastY[TOUCHNO_1ST],
                                  lastX[TOUCHNO_2ND], lastY[TOUCHNO_2ND]);
        }
        else
        {
            (info->touchListener)(sendType,
                                  info->touchX[TOUCHNO_1ST], info->touchY[TOUCHNO_1ST],
                                  info->touchX[TOUCHNO_2ND], info->touchY[TOUCHNO_2ND]);
        }
    }
}

#ifdef MFTLH_PLATFORM_UBUNTU
static void handlePointerEnterDummy(void *data, struct wl_pointer *pointer,
                                 uint32_t serial, struct wl_surface *surface,
                                 wl_fixed_t sx, wl_fixed_t sy)
{
    /* DO NOTHING */
}

static void handlePointerLeaveDummy(void *data, struct wl_pointer *pointer,
                                 uint32_t serial, struct wl_surface *surface)
{
    /* DO NOTHING */
}

static void handlePointerMotion(void *data, struct wl_pointer *pointer,
                                uint32_t time, wl_fixed_t sx, wl_fixed_t sy)
{
    mf_TLH_OpenGLInfo *info = (mf_TLH_OpenGLInfo *)data;
    g_pointerX = sx;
    g_pointerY = sy;
    /* マウスボタンが押されている場合はドラッグ動作を通知する。*/
    if ((TOUCHID_DUMMY == info->touchId[TOUCHNO_1ST]) ||
            (TOUCHID_DUMMY == info->touchId[TOUCHNO_2ND]))
    {
        touchProcess(info, Touch_Update, TOUCHID_DUMMY, g_pointerX, g_pointerY);
    }
}

static void handlePointerButton(void *data, struct wl_pointer *wl_pointer,
                                uint32_t serial, uint32_t time, uint32_t button,
                                uint32_t state)
{
    /* Pointer(マウス)の操作で一点タッチをシミュレートする */
    /* IDはダミーを与える。*/
    mf_TLH_OpenGLInfo *info = (mf_TLH_OpenGLInfo *)data;
    if (button == BTN_LEFT && state == WL_POINTER_BUTTON_STATE_PRESSED)
    {
        /* 左マウスボタンを押した（タッチ開始） */
        touchProcess(info, Touch_Begin, TOUCHID_DUMMY, g_pointerX, g_pointerY);
    }
    else
    {
        /* 左マウスボタンを離した（タッチ終了） */
        touchProcess(info, Touch_End, TOUCHID_DUMMY, g_pointerX, g_pointerY);
    }
}

static void handlePointerAxisDummy(void *data, struct wl_pointer *wl_pointer,
                              uint32_t time, uint32_t axis, wl_fixed_t value)
{
    /* DO NOTHING */
}

static const struct wl_pointer_listener pointerListener = {
    handlePointerEnterDummy,
    handlePointerLeaveDummy,
    handlePointerMotion,
    handlePointerButton,
    handlePointerAxisDummy,
    NULL,
    NULL,
    NULL,
    NULL
};
#endif

/****************************************************************************
 *
 *  説明   : Wayland対応タッチハンドラ
 *
 ****************************************************************************/
static void handleTouchPress(void *data, struct wl_touch *wl_touch,
                             uint32_t serial, uint32_t time,
                             struct wl_surface *surface, int32_t id,
                             wl_fixed_t x, wl_fixed_t y)
{
    mf_TLH_OpenGLInfo *info = (mf_TLH_OpenGLInfo *)data;
    touchProcess(info, Touch_Begin, id, x, y);
}

static void handleTouchRelease(void *data, struct wl_touch *wl_touch,
                               uint32_t serial, uint32_t time, int32_t id)
{
    mf_TLH_OpenGLInfo *info = (mf_TLH_OpenGLInfo *)data;
    touchProcess(info, Touch_End, id, TOUCHPOS_BLANK, TOUCHPOS_BLANK);
}

static void handleTouchMotion(void *data, struct wl_touch *wl_touch,
                                   uint32_t time, int32_t id,
                                   wl_fixed_t x, wl_fixed_t y)
{
    mf_TLH_OpenGLInfo *info = (mf_TLH_OpenGLInfo *)data;
    touchProcess(info, Touch_Update, id, x, y);
}

static void handleTouchFrameDummy(void *data, struct wl_touch *wl_touch)
{
    /* DO NOTHING */
}

static void handleTouchCancelDummy(void *data, struct wl_touch *wl_touch)
{
    /* DO NOTHING */
}

static const struct wl_touch_listener touchListener = {
    handleTouchPress,
    handleTouchRelease,
    handleTouchMotion,
    handleTouchFrameDummy,
    handleTouchCancelDummy,
};

void openglSetTouchListener(void *openglinfo, mf_TLH_TouchListener touchListener)
{
    mf_TLH_OpenGLInfo *info = (mf_TLH_OpenGLInfo *)openglinfo;
    info->touchListener = touchListener;
}

int openglGetEventFd(void *openglinfo)
{
    mf_TLH_OpenGLInfo *info = (mf_TLH_OpenGLInfo *)openglinfo;
	return wl_display_get_fd(info->wlDisplay);
}

unsigned long  getSurfaceID( void *openglinfo )
{
    return ((mf_TLH_OpenGLInfo *)openglinfo)->m_surfaceId;
}

void *openglGetEglDisplay(void *openglinfo)
{
	return ((mf_TLH_OpenGLInfo *)openglinfo)->m_eglDisplay;
}

void *openglGetEglSurface(void *openglinfo)
{
	return ((mf_TLH_OpenGLInfo *)openglinfo)->m_eglSurface;
}

static void handleSeatCapabilities(void *data, struct wl_seat *seat, uint32_t caps)
{
    mf_TLH_OpenGLInfo *info = (mf_TLH_OpenGLInfo *)data;
    if ((caps & WL_SEAT_CAPABILITY_TOUCH) && !info->wlTouch) {
        info->wlTouch = wl_seat_get_touch(seat);
        wl_touch_set_user_data(info->wlTouch, NULL);
#ifdef MFTLHDEBUGTRACE
        PrintLog("fhilh: wayland touch listener setup");
#endif
        wl_touch_add_listener(info->wlTouch, &touchListener, info);
    } else if (!(caps & WL_SEAT_CAPABILITY_TOUCH) && info->wlTouch) {
        wl_touch_destroy(info->wlTouch);
        info->wlTouch = NULL;
    }
#ifdef MFTLH_PLATFORM_UBUNTU
    if ((caps & WL_SEAT_CAPABILITY_POINTER) && !info->wlPointer) {
        info->wlPointer = wl_seat_get_pointer(seat);
#ifdef MFTLHDEBUGTRACE
        PrintLog("fhilh: wayland pointer listener setup");
#endif
        wl_pointer_add_listener(info->wlPointer, &pointerListener, info);
    } else if (!(caps & WL_SEAT_CAPABILITY_POINTER) && info->wlPointer) {
        wl_pointer_destroy(info->wlPointer);
        info->wlPointer = NULL;
    }
#endif
}

static const struct wl_seat_listener seatListener = {
    handleSeatCapabilities,
    NULL
};

/****************************************************************************
 *  説明   : Waylandレジストリハンドラ
 ****************************************************************************/
static void handleRegistry(void *data, struct wl_registry *registry,
                           uint32_t name, const char *interface, uint32_t aaa)
{
    mf_TLH_OpenGLInfo *info = (mf_TLH_OpenGLInfo *)data;
    if (strcmp(interface, "wl_compositor") == 0)
    {
#ifdef MFTLHDEBUGTRACE
        PrintLog("fhilh: wl_compositor");
#endif
        /* コンポジターを取得（waylandサーフェイス生成に必要）*/
        info->wlCompositor = (struct wl_compositor *) wl_registry_bind(
                (struct wl_registry *) registry,
                (uint32_t) name,
                (const struct wl_interface *) &wl_compositor_interface,
                (uint32_t) 1);
    }
    else if (strcmp(interface, "wl_shell") == 0)
    {
#ifdef MFTLHDEBUGTRACE
        PrintLog("fhilh: wl_shell");
#endif
        info->wlShell = wl_registry_bind(registry, name, &wl_shell_interface, 1);
    }
#ifdef MFTLH_PLATFORM_TARGET
    else if (strcmp(interface, "ivi_application") == 0)
    {
#ifdef MFTLHDEBUGTRACE
        PrintLog("fhilh: ivi_application");
#endif
        /* IVIアプリケーションを取得（iviサーフェイス生成に必要）*/
        info->iviApplication = (struct ivi_application *) wl_registry_bind(
                (struct wl_registry *) registry,
                (uint32_t) name,
                (const struct wl_interface *) &ivi_application_interface,
                (uint32_t) 1);
    }
#endif
    else if (strcmp(interface, "wl_seat") == 0)
    {
#ifdef MFTLHDEBUGTRACE
        PrintLog("fhilh: wl_seat");
#endif
        /* 入力デバイス対応（入力イベントのリスナー登録に必要）*/
        info->wlSeat = (struct wl_seat *) wl_registry_bind(
                (struct wl_registry *) registry,
                (uint32_t) name,
                (const struct wl_interface *) &wl_seat_interface,
                (uint32_t) 1);
        wl_seat_add_listener(info->wlSeat, &seatListener, info);
    }
	else
	{
#ifdef MFTLHDEBUGTRACE
		char logBuf[256];
		snprintf(logBuf,sizeof(logBuf),"fhilh: other registry %s", interface);
		PrintLog(logBuf);
#endif
	}
}

static const struct wl_registry_listener registry_listener = {
    handleRegistry,
    NULL
};

/****************************************************************************
 *  説明   : Waylandイベント処理
 ****************************************************************************/
void handleEvent(void *param)
{
	int wlResult;
    mf_TLH_OpenGLInfo *info = (mf_TLH_OpenGLInfo *)param;
    wlResult = wl_display_dispatch(info->wlDisplay);
    if (wlResult == -1)
    {
#ifdef MFTLHDEBUGTRACE
		char logBuf[256];
		snprintf(logBuf,sizeof(logBuf),"fhilh: wl_display_dispatch Error %d", wlResult);
		PrintLog(logBuf);
#endif
		return;
	}
#ifdef MFTLHDEBUGTRACE
	{
		char logBuf[256];
		snprintf(logBuf,sizeof(logBuf),"fhilh: wl_display_dispatch OK(%devents)", wlResult);
		PrintLog(logBuf);
	}
#endif
}

#ifdef MFTLH_PLATFORM_TARGET
/****************************************************************************
 *  説明   : IVIサーフェイスハンドラ
 ****************************************************************************/
static void iviSurfaceVisibility(void *data, struct ivi_surface *ivi_surface,
                                    int32_t visibility)
{
    /* ウィンドウシステムによってサーフェイスの表示、非表示が切り替わった */
#ifdef MFTLHDEBUGTRACE
	{
		char logBuf[256];
		snprintf(logBuf,sizeof(logBuf),"fhilh: ivi_surface_visibility %d", visibility);
		PrintLog(logBuf);
	}
#endif
}

static void iviSurfaceConfigure(void *data, struct ivi_surface* ivi_surface,
                                int32_t width,int32_t height)
{
    /* DO NOTHING */
}

static const struct ivi_surface_listener iviSurfaceEventListener = {
    iviSurfaceVisibility,
    iviSurfaceConfigure
};
#endif

mf_TLH_OpenGLResult createWindow(mf_TLH_OpenGLInfo *esContext,
                                    int *returnDetailResult, mf_TLH_WAYFunc *returnDetailFunc)
{
    int wlResult = 0;
    esContext->wlDisplay = wl_display_connect(NULL);
    if ( NULL == esContext->wlDisplay )
    {
#ifdef MFTLHDEBUGTRACE
        PrintLog("fhilh: wayland compositor not connected");
#endif
        *returnDetailFunc = WAYFunc_displayConnect;
        *returnDetailResult = 0;
        return GLResult_WAYError;
    }

#ifdef MFTLHDEBUGTRACE
	{
		char logBuf[256];
		snprintf(logBuf,sizeof(logBuf),"fhilh: wayland compositor connected p_display:%p",esContext->wlDisplay);
		PrintLog(logBuf);
	}
#endif

    esContext->wlRegistry = wl_display_get_registry(esContext->wlDisplay);
    if (NULL == esContext->wlRegistry)
    {
        *returnDetailFunc = WAYFunc_displayGetRegistry;
        *returnDetailResult = 0;
        return GLResult_WAYError;
    }
    wlResult = wl_registry_add_listener(esContext->wlRegistry, &registry_listener, esContext);
    if (0 != wlResult)
    {
        *returnDetailFunc = WAYFunc_registryAddListener;
        *returnDetailResult = wlResult;
        return GLResult_WAYError;
    }
    wlResult = wl_display_roundtrip(esContext->wlDisplay);
    if (wlResult < 0)
    {
        *returnDetailFunc = WAYFunc_displayDispatch;
        *returnDetailResult = wlResult;
        return GLResult_WAYError;
    }
    wlResult = wl_display_roundtrip(esContext->wlDisplay);
    if (wlResult < 0)
    {
        *returnDetailFunc = WAYFunc_displayDispatch;
        *returnDetailResult = wlResult;
        return GLResult_WAYError;
    }
    esContext->wlSurface = wl_compositor_create_surface(esContext->wlCompositor);

    if ( NULL != esContext->wlShell )
    {
#ifdef MFTLHDEBUGTRACE
        PrintLog("fhilh: wl_shell_get_shell_surface");
#endif
        esContext->wlShellSurface = wl_shell_get_shell_surface(
                    esContext->wlShell, esContext->wlSurface);
        wl_shell_surface_add_listener(
                    esContext->wlShellSurface, &wlShellSurfaceListener, esContext);
        wl_display_roundtrip(esContext->wlDisplay);
    }
#ifdef MFTLH_PLATFORM_TARGET
    if (NULL != esContext->iviApplication)
    {
#ifdef MFTLHDEBUGTRACE
        PrintLog("fhilh: ivi_application_surface_create");
#endif
        esContext->iviSurface =
                ivi_application_surface_create(esContext->iviApplication,
                                                esContext->iviSurfaceId,
                                                esContext->wlSurface);
        ivi_surface_add_listener(esContext->iviSurface,
                                    &iviSurfaceEventListener, esContext);
    }
#endif
    esContext->eglWindowNative = wl_egl_window_create(
										esContext->wlSurface,
										esContext->m_windowWidth,
										esContext->m_windowHeight);
#ifdef MFTLH_PLATFORM_UBUNTU
    if( NULL != esContext->wlShellSurface )
    {
#ifdef MFTLHDEBUGTRACE
        PrintLog("fhilh: wl_shell_surface_set_toplevel");
#endif
        wl_shell_surface_set_toplevel(esContext->wlShellSurface);
    }
#endif
    // Get Display
    EGLDisplay display = eglGetDisplay((EGLNativeDisplayType)(esContext->wlDisplay));
    if ( display == EGL_NO_DISPLAY )
    {
#ifdef MFTLHDEBUGTRACE
		{
			char logBuf[256];
			snprintf(logBuf,sizeof(logBuf),"fhilh: eglGetDisplay Error %p", display);
			PrintLog(logBuf);
		}
#endif
        *returnDetailFunc = EGLFunc_eglGetDisplay;
        *returnDetailResult = 0;
        return GLResult_EGLError;
    }

    // Initialize EGL
    EGLint majorVersion;
    EGLint minorVersion;
    if ( EGL_TRUE != eglInitialize(display, &majorVersion, &minorVersion) )
    {
		EGLint ret_egl = eglGetError();
#ifdef MFTLHDEBUGTRACE
		{
			char logBuf[256];
			snprintf(logBuf,sizeof(logBuf),"fhilh: eglInitialize Error %d",ret_egl);
			PrintLog(logBuf);
		}
#endif
        *returnDetailFunc = EGLFunc_eglInitialize;
        *returnDetailResult = ret_egl;
        return GLResult_EGLError;
    }
#ifdef MFTLHDEBUGTRACE
	{
		char logBuf[256];
		snprintf(logBuf,sizeof(logBuf),"fhilh: eglInitialize %d,%d",majorVersion,minorVersion);
		PrintLog(logBuf);
	}
#endif

    esContext->m_eglDisplay = display;
    *returnDetailFunc = EGLFunc_NoFunc;
	*returnDetailResult = 0;
    return GLResult_Success;
}


void *allocateOpenGLInfo()
{
    mf_TLH_OpenGLInfo *openglinfo;
    openglinfo = malloc(sizeof(mf_TLH_OpenGLInfo));
    if ( NULL != openglinfo )
    {
        memset(openglinfo,0x00,sizeof(mf_TLH_OpenGLInfo));
        return openglinfo;
    }
    else
    {
        return NULL;
    }
}

void freeOpenGLInfo(void *info )
{
    mf_TLH_OpenGLInfo *openglinfo = (mf_TLH_OpenGLInfo *)info;
    free(info);
}

mf_TLH_OpenGLResult openglinit( void *openglinfo,
                                unsigned int ilmSurfaceId,
                                int windowWidth, int windowHeight)
{
    EGLBoolean eglBool;
    mf_TLH_WAYFunc returnDetailFunc;
    int returnDetailResult;
    mf_TLH_OpenGLResult tlhResult;

    mf_TLH_OpenGLInfo *info = (mf_TLH_OpenGLInfo *)openglinfo;
    memset(openglinfo, 0x00, sizeof(mf_TLH_OpenGLInfo));
    info->touchId[0] = TOUCHID_BLANK;
    info->touchX[0] = TOUCHPOS_BLANK;
    info->touchY[0] = TOUCHPOS_BLANK;
    info->touchId[1] = TOUCHID_BLANK;
    info->touchX[1] = TOUCHPOS_BLANK;
    info->touchY[1] = TOUCHPOS_BLANK;

    info->iviSurfaceId = (uint32_t)ilmSurfaceId;

    info->m_windowWidth = windowWidth;
    info->m_windowHeight = windowHeight;

    /* APIをバインドする */
    eglBool = eglBindAPI(EGL_OPENGL_ES_API);
    if (eglBool != EGL_TRUE)
    {
        info->m_tlhResult = GLResult_EGLError;
        info->m_detailFunc = EGLFunc_eglBindAPI;
        info->m_detailResult = eglBool;
        return info->m_tlhResult;
    }

    /* OpenGLウィンドウ生成 */
    tlhResult = createWindow( info, &returnDetailResult, &returnDetailFunc );
    if ( GLResult_Success != tlhResult )
    {
        info->m_tlhResult = tlhResult;
        info->m_detailFunc = returnDetailFunc;
        info->m_detailResult = returnDetailResult;
        return info->m_tlhResult;
    }

    return tlhResult;
}

void openglSetMinInterval(void *openglinfo)
{
#ifdef MFTLH_PLATFORM_TARGET
    EGLint eglResult;
    mf_TLH_OpenGLInfo *info = (mf_TLH_OpenGLInfo *)openglinfo;

    /* EGLからSwapBufferの最小待ち時間を取得する(ゼロは非同期描画対応を表す) */
    EGLint eglMinSwapInterval = 0;
    if (EGL_TRUE != eglGetConfigAttrib(info->m_eglDisplay, info->m_eglConfig,
                                EGL_MIN_SWAP_INTERVAL, &eglMinSwapInterval))
    {
        eglResult = eglGetError();
#ifdef MFTLHDEBUGTRACE
		{
			char logBuf[256];
			snprintf(logBuf,sizeof(logBuf),"fhilh: eglGetConfigAttrib Error %d", eglResult);
			PrintLog(logBuf);
		}
#endif
        info->m_tlhResult = GLResult_EGLError;
        info->m_detailFunc = EGLFunc_eglGetConfigAttrib;
        info->m_detailResult = eglResult;
        return;
    }

    /* SwapBufferの間隔を最小値に変更する(ゼロ値は非同期描画となる) */
#ifdef MFTLHDEBUGTRACE
	{
		char logBuf[256];
		snprintf(logBuf,sizeof(logBuf),"fhilh: eglSwapInterval(%d)", eglMinSwapInterval);
		PrintLog(logBuf);
	}
#endif
    if (EGL_TRUE != eglSwapInterval(info->m_eglDisplay, eglMinSwapInterval))
    {
        eglResult = eglGetError();
#ifdef MFTLHDEBUGTRACE
		{
			char logBuf[256];
			snprintf(logBuf,sizeof(logBuf),"fhilh: eglSwapInterval Error %d", eglResult);
			PrintLog(logBuf);
		}
#endif
        info->m_tlhResult = GLResult_EGLError;
        info->m_detailFunc = EGLFunc_eglSwapInterval;
        info->m_detailResult = eglResult;
    }
#endif
}

void openglFlush(void *param)
{
    int wlResult = 0;
    mf_TLH_OpenGLInfo *info = (mf_TLH_OpenGLInfo *)param;
    if (NULL == info)
    {
        return;
    }

    wlResult = wl_display_flush(info->wlDisplay);
#ifdef MFTLHDEBUGTRACE
    if (wlResult >= 0)
    {
		char logBuf[256];
		snprintf(logBuf,sizeof(logBuf),"fhilh: wl_display_flush processed %d", wlResult);
		PrintLog(logBuf);
    }
    else
    {
		PrintLog("fhilh: wl_display_flush error");
    }
#endif
}

mf_TLH_OpenGLResult checkOpenGLAsyncError(void *openglinfo )
{
    mf_TLH_OpenGLInfo *info = (mf_TLH_OpenGLInfo *)openglinfo;
#ifdef MFTLH_PLATFORM_TARGET
    if (0 != info->ivi_warningCode)
    {
#ifdef MFTLHDEBUGTRACE
		{
			char logBuf[256];
			snprintf(logBuf,sizeof(logBuf),"fhilh: opengldraw error(ivi_surface_warning code:%d)",
                info->ivi_warningCode);
			PrintLog(logBuf);
		}
#endif
        info->m_tlhResult = GLResult_WAYError;
        info->m_detailFunc = IVIFunc_surfaceWarning;
        info->m_detailResult = (unsigned short)info->ivi_warningCode;
        info->ivi_warningCode = 0;
        return info->m_tlhResult;
    }
#endif
    return GLResult_Success;
}

void openglDestroySurface(void *param)
{
    int wlResult = 0;
    mf_TLH_OpenGLInfo *info = (mf_TLH_OpenGLInfo *)param;
    if (NULL == info)
    {
        return;
    }

#ifdef MFTLH_PLATFORM_TARGET
    /* IVIサーフェイス破棄 */
    if (NULL != info->iviSurface)
    {
        ivi_surface_destroy(info->iviSurface);
    }

    /* IVIアプリケーション破棄 */
    if (NULL != info->iviApplication)
    {
        ivi_application_destroy(info->iviApplication);
    }
#endif

    eglMakeCurrent(
            info->m_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

    /* EGLサーフェイス破棄 */
    eglDestroySurface(info->m_eglDisplay, info->m_eglSurface);

    /* EGLウインドウ破棄 */
    if (0 != info->eglWindowNative)
    {
        wl_egl_window_destroy(info->eglWindowNative);
    }

    /* Waylandシェルサーフェイス破棄 */
    if (NULL != info->wlShellSurface)
    {
        wl_shell_surface_destroy(info->wlShellSurface);
    }

    /* Waylandサーフェイス破棄 */
    if (NULL != info->wlSurface)
    {
        wl_surface_destroy(info->wlSurface);
    }

    /* EGL終了 */
    eglTerminate(info->m_eglDisplay);
    eglReleaseThread();

    /* Waylandシェル破棄 */
    if (NULL != info->wlShell)
    {
        wl_shell_destroy(info->wlShell);
    }

    if (NULL != info->wlCompositor)
    {
        wl_compositor_destroy(info->wlCompositor);
    }

    wlResult = wl_display_flush(info->wlDisplay);
    if (0 != wlResult)
    {
    }

    if (NULL != info->wlDisplay)
    {
        wl_display_disconnect(info->wlDisplay);
    }

    /* wayland情報をクリア */
    memset(info, 0x00, sizeof(mf_TLH_OpenGLInfo));

}

#ifdef MFTLHDEBUGTRACE
void PrintLog(const char *aLog)
{
	time_t now;
	struct tm* tm_now;
	struct timeval tv;
	char logStr[256];

	if (aLog == NULL)
	{
		return;
	}

	gettimeofday(&tv, NULL);
	now = time(NULL);
	tm_now = localtime(&now);

	snprintf(logStr,sizeof(logStr),"%02d:%02d:%02d.%03d %s",
				tm_now->tm_hour,
				tm_now->tm_min,
				tm_now->tm_sec,
				tv.tv_usec/1000,
				aLog);
#ifdef MFTLH_PLATFORM_TARGET
	// ディレクトリがない場合は、作成する.
	FILE* file;
	file = fopen("/data/customapp0browser/out/fhilhlog.txt", "a");	// 追加書き込み.
	if (file == NULL)
	{
		return;
	}
	fprintf(file, "%s\n", logStr);
	fclose(file);
#else
	fprintf(stderr,"%s\n", logStr);
#endif
}
#endif

