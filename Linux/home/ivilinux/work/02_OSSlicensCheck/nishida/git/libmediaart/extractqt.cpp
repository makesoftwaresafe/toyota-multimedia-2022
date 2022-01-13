/*
 * Copyright (C) 2010, Nokia
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 *
 * Authors:
 * Philip Van Hoof <philip@codeminded.be>
 */

#include "config.h"

#include <QFile>
#include <QBuffer>
#include <QImageReader>
#include <QImageWriter>
#ifdef HAVE_QT5
#include <QCoreApplication>
#else
#include <QApplication>
#endif
#include <QColor>
#include <QPainter>

#include <glib.h>

#ifdef HAVE_NEMO
#include <stdlib.h>
#endif

G_BEGIN_DECLS

#ifdef HAVE_QT5
static QCoreApplication *app = NULL;
#else  /* HAVE_QT4 (we fallback to Qt4) */
static QApplication *app = NULL;
#endif /* HAVE_QT5 */

static gint max_width_in_bytes = 0;

void
media_art_plugin_init (gint max_width)
{
	static int argc = 1;
	static char *argv[2] = { (char*) "libmediaart", NULL };

	g_return_if_fail (max_width >= 0);

	max_width_in_bytes = max_width;

#ifdef HAVE_QT5

#ifdef HAVE_NEMO
	setenv("QT_QPA_PLATFORM", "minimal", 1);
#endif /* HAVE_NEMO */

	app = new QCoreApplication (argc, argv);
#else  /* HAVE_QT4 (we fallback to Qt4) */
 	app = new QApplication (argc, argv, QApplication::Tty);
#endif /* HAVE_QT5 */
}

void
media_art_plugin_shutdown (void)
{
	// Apparently isn't destructing a QApplication something you should do, as
	// QApplication is designed to work on stack of the main() function.

	// delete app;
}

gboolean
media_art_file_to_jpeg (const gchar  *filename,
                        const gchar  *target,
                        GError      **error)
{
	if (max_width_in_bytes < 0) {
		g_debug ("Not saving album art from file, disabled in config");
		return TRUE;
	}

	/* TODO: Add resizing support */
	/* TODO: Add error reporting */

	QFile file (filename);

	if (!file.open (QIODevice::ReadOnly)) {
		g_message ("Could not get QFile from file: '%s'", filename);
		return FALSE;
	}

	QByteArray array = file.readAll ();
	QBuffer buffer (&array);

	buffer.open (QIODevice::ReadOnly);

	QImageReader reader (&buffer);

	if (!reader.canRead ()) {
		g_message ("Could not get QImageReader from file: '%s', reader.canRead was FALSE",
		           filename);
		return FALSE;
	}

	QImage image1;
	image1 = reader.read ();

	if (image1.hasAlphaChannel ()) {
		QImage image2 (image1.size(), QImage::Format_RGB32);
		image2.fill (QColor(Qt::black).rgb());
		QPainter painter (&image2);
		painter.drawImage (0, 0, image1);
		image2.save (QString (target), "jpeg");
	} else {
		image1.save (QString (target), "jpeg");
	}

	return TRUE;
}

gboolean
media_art_buffer_to_jpeg (const unsigned char  *buffer,
                          size_t                len,
                          const gchar          *buffer_mime,
                          const gchar          *target,
                          GError              **error)
{
	if (max_width_in_bytes < 0) {
		g_debug ("Not saving album art from buffer, disabled in config");
		return TRUE;
	}

	/* FF D8 FF are the three first bytes of JPeg images */
	if (max_width_in_bytes == 0 &&
	    (g_strcmp0 (buffer_mime, "image/jpeg") == 0 ||
	     g_strcmp0 (buffer_mime, "JPG") == 0) &&
	    (buffer && len > 2 && buffer[0] == 0xff && buffer[1] == 0xd8 && buffer[2] == 0xff)) {
		g_debug ("Saving album art using raw data as uri:'%s'", target);
		if (!g_file_set_contents (target, (const gchar*) buffer, (gssize) len, error)) {
			return FALSE;
		}
	} else {
		QImageReader *reader = NULL;
		QByteArray array;

		/* TODO: Add resizing support */
		/* TODO: Add error reporting */

		array = QByteArray ((const char *) buffer, (int) len);

		QBuffer qbuffer (&array);
		qbuffer.open (QIODevice::ReadOnly);

		if (buffer_mime != NULL) {
			reader = new QImageReader (&qbuffer, QByteArray (buffer_mime));
		} else {
			QByteArray format = QImageReader::imageFormat (&qbuffer);

			if (!format.isEmpty ()) {
				reader = new QImageReader (&qbuffer, format);
			}
		}

		if (!reader) {
			g_message ("Could not get QImageReader from buffer");
			return FALSE;
		}

		QImage image1;
		image1 = reader->read ();

		if (image1.hasAlphaChannel ()) {
			QImage image2 (image1.size(), QImage::Format_RGB32);
			image2.fill (QColor(Qt::black).rgb());
			QPainter painter (&image2);
			painter.drawImage (0, 0, image1);
			image2.save (QString (target), "jpeg");
		} else {
			image1.save (QString (target), "jpeg");
		}

		delete reader;
	}

	return TRUE;
}

G_END_DECLS
