/*** COPYRIGHT FUJITSU LIMITED 2018 ***/
#include "fhilhscreen.h"
#include "mftlhopengl.h"
#include <QtPlatformSupport/private/qeglplatformcontext_p.h>
#include <QtPlatformSupport/private/qeglconvenience_p.h>

#include "unistd.h"
#include "sys/types.h"
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

static void *g_fhilhContext;

QT_BEGIN_NAMESPACE

class FhiLhEGLContext : public QEGLPlatformContext
{
public:
	FhiLhEGLContext(const QSurfaceFormat &format, QPlatformOpenGLContext *share, EGLDisplay display)
		: QEGLPlatformContext(format, share, display)
	{
	}
	
	EGLSurface eglSurfaceForPlatformSurface(QPlatformSurface *surface)
	{
		FhiLhScreen *pltScreen = (FhiLhScreen *)((QPlatformWindow *)surface)->screen();
		return pltScreen->getPlatformEglSurface();
	}
};

FhiLhScreen::~FhiLhScreen()
{
	openglDestroySurface(g_fhilhContext);
    freeOpenGLInfo(g_fhilhContext);
    g_fhilhContext = NULL;
}

unsigned int FhiLhScreen::getIlmSurfaceId() const
{
	// ブラウザ本体が用意した共有メモリからILMサーフェースIDを取得する。
    unsigned int ilmSurfaceId = 0;
	unsigned int *pSurfaceId = NULL;
	struct stat a_stat;
	int retval;
	int shm_fd = shm_open("/FHILH_SurfaceId", 
					O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
	fstat(shm_fd, &a_stat);
	if ( 0 == a_stat.st_size ) {
		retval = ftruncate(shm_fd, sizeof(unsigned int));
		if ( 0 == retval )
		{
			fstat(shm_fd, &a_stat);
		}
	}
	pSurfaceId = (unsigned int *)mmap(NULL, a_stat.st_size, 
						PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
	if (pSurfaceId == NULL)
	{
		close(shm_fd);
		return 0;
	}
	ilmSurfaceId = *pSurfaceId;
	close(shm_fd);
	
	return ilmSurfaceId;
}	

QPlatformOpenGLContext *FhiLhScreen::createAndSetPlatformContext() const
{
	unsigned int ilmSurfaceId = getIlmSurfaceId();
	if (ilmSurfaceId == 0)
	{
		return NULL;
	}

	// OpenGLテーブルを確保する。
	g_fhilhContext = allocateOpenGLInfo();
	if (g_fhilhContext == NULL)
	{
		return NULL;
	}

	// OpenGLを初期化する。
	mf_TLH_OpenGLResult result = openglinit( g_fhilhContext, ilmSurfaceId,
									m_geometry.width(), m_geometry.height());
    if (result != GLResult_Success)
    {
		return NULL;
    }

	// QtGuiからEGLConfigを取得する。
    QSurfaceFormat platformFormat;
	platformFormat.setDepthBufferSize(24);
	platformFormat.setStencilBufferSize(8);
	platformFormat.setRedBufferSize(8);
	platformFormat.setGreenBufferSize(8);
	platformFormat.setBlueBufferSize(8);
	platformFormat.setAlphaBufferSize(8);
	platformFormat.setSwapInterval(0);
    platformFormat.setSamples(4);

	EGLDisplay display = openglGetEglDisplay(g_fhilhContext);
    EGLConfig config = q_configFromGLFormat(display, platformFormat);
    
    // EGLサーフェースを生成する。
    result = createEGLSurface(g_fhilhContext, config);
    if (result != GLResult_Success)
    {
		return NULL;
    }
    
	// QtのEGLコンテキストを生成する。
    QEGLPlatformContext *platformContext = new FhiLhEGLContext(platformFormat, 0, display);
    return platformContext;
}

/* EGLSurfaceを参照する */
void *FhiLhScreen::getPlatformEglSurface()
{
	return openglGetEglSurface(g_fhilhContext);
}

/* SwapIntervalに最小値を設定する */
void FhiLhScreen::setMinInterval()
{
	openglSetMinInterval(g_fhilhContext);
}

/* ウィンドウシステムのイベント処理するファイルディスクリプタを参照する */
int FhiLhScreen::getEvnetFd()
{
	return openglGetEventFd(g_fhilhContext);
}

/* タッチリスナー関数を登録する */
void FhiLhScreen::setTouchListener(mf_TLH_TouchListener touchListener)
{
	openglSetTouchListener(g_fhilhContext, touchListener);
}

/* OpenGLテーブルのアドレスを参照する */
void *FhiLhScreen::getPlatformHandle()
{
	return g_fhilhContext;
}

/* 描画結果をウィンドウシステムに送る */
void FhiLhScreen::displayFlush()
{
    openglFlush(g_fhilhContext);
}

QT_END_NAMESPACE
