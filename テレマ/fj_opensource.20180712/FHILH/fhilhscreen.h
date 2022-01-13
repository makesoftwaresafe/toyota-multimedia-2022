/*** COPYRIGHT FUJITSU LIMITED 2018 ***/
#ifndef FHILHSCREEN_H
#define FHILHSCREEN_H

#include <qpa/qplatformscreen.h>

QT_BEGIN_NAMESPACE

typedef void (*mf_TLH_TouchListener)(int type, int x1, int y1, int x2, int y2);

class FhiLhScreen : public QPlatformScreen
{
public:
    FhiLhScreen()
			: m_geometry( QRect(0, 0, 800, 480))
			, m_depth(32)
			, m_format(QImage::Format_ARGB32_Premultiplied)
	{
		;
	}
	~FhiLhScreen();

	QRect geometry() const { return m_geometry; }
	int depth() const { return m_depth; }
	QImage::Format format() const { return m_format; }

protected:
	QPlatformOpenGLContext *createAndSetPlatformContext() const;
	void setGeometry(QRect newGeometry) { m_geometry = newGeometry; }
	void *getPlatformHandle();
	void *getPlatformEglSurface();
	int getEvnetFd();
	void setMinInterval();
	void setTouchListener(mf_TLH_TouchListener touchListener);
	void displayFlush();

private:
	unsigned int getIlmSurfaceId() const;
	QRect m_geometry;
	int m_depth;
	QImage::Format m_format;

	friend class FhiLhIntegration;
    friend class FhiLhEGLContext;
    friend class FhiLhBackingStore;
    friend class FhiLhWindow;
};

QT_END_NAMESPACE

#endif // FHILHSCREEN_H
