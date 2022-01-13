/*
    Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies)
    Copyright (C) 2013 Apple Inc. All rights reserved.

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "config.h"
#include "qwebsettings.h"

#include "qwebplugindatabase_p.h"

#include "ApplicationCacheStorage.h"
#include "CrossOriginPreflightResultCache.h"
#include "DatabaseManager.h"
#include "FileSystem.h"
#include "FontCache.h"
#if USE(JSC)
#include "GCController.h"
#else
#include "V8GCController.h"
#include "ScriptController.h"
#endif
#include "IconDatabase.h"
#include "Image.h"
#if ENABLE(ICONDATABASE)
#include "IconDatabaseClientQt.h"
#endif
#include "InitWebCoreQt.h"
#include "IntSize.h"
#include "URL.h"
#include "MemoryCache.h"
#include "NetworkStateNotifier.h"
#include "Page.h"
#include "PageCache.h"
#include "PluginDatabase.h"
#include "RuntimeEnabledFeatures.h"
#include "Settings.h"
#include "StorageThread.h"
#include "WorkerThread.h"
#include <QDir>
#include <QFileInfo>
#include <QFont>
/**/
#if HAVE(QT5)
#include <QGuiApplication>
#else
#include <QApplication>
#endif
/**/
#include <QHash>
#include <QSharedData>
/**/
#if HAVE(QT5)
#include <QStandardPaths>
#else
#include <QDesktopServices>
#endif
/**/
#include <QUrl>
#include <wtf/FastMalloc.h>
#include <wtf/text/WTFString.h>
/**/
#include <QNetworkAccessManager>
#include <QNetworkCookieJar>

#include "Modules/websockets/WebSocket.h"
/**/

/**/
using namespace WebCore;
/**/

QWEBKIT_EXPORT void qt_networkAccessAllowed(bool isAllowed)
{
#ifndef QT_NO_BEARERMANAGEMENT
    WebCore::networkStateNotifier().setNetworkAccessAllowed(isAllowed);
#endif
}

class QWebSettingsPrivate {
public:
    QWebSettingsPrivate(WebCore::Settings* wcSettings = 0)
        : settings(wcSettings)
    {
    }

    QHash<int, QString> fontFamilies;
    QHash<int, int> fontSizes;
    QHash<int, bool> attributes;
    QUrl userStyleSheetLocation;
    QString defaultTextEncoding;
    QString localStoragePath;
    QString offlineWebApplicationCachePath;
    QString mediaType;
    qint64 offlineStorageDefaultQuota;
    QWebSettings::ThirdPartyCookiePolicy thirdPartyCookiePolicy;
    void apply();
    WebCore::Settings* settings;

/**/
#if ENABLE_NETSCAPE_PLUGIN_API
    QVector<QNPPValue>	m_nppValue;
#endif

    quint32 aboutBlankBackGroundColor;
/**/
};

Q_GLOBAL_STATIC(QList<QWebSettingsPrivate*>, allSettings);

void QWebSettingsPrivate::apply()
{
    if (settings) {
        settings->setTextAreasAreResizable(true);

        QWebSettingsPrivate* global = QWebSettings::globalSettings()->d;

        QString family = fontFamilies.value(QWebSettings::StandardFont,
                                            global->fontFamilies.value(QWebSettings::StandardFont));
        settings->setStandardFontFamily(family);

        family = fontFamilies.value(QWebSettings::FixedFont,
                                    global->fontFamilies.value(QWebSettings::FixedFont));
        settings->setFixedFontFamily(family);

        family = fontFamilies.value(QWebSettings::SerifFont,
                                    global->fontFamilies.value(QWebSettings::SerifFont));
        settings->setSerifFontFamily(family);

        family = fontFamilies.value(QWebSettings::SansSerifFont,
                                    global->fontFamilies.value(QWebSettings::SansSerifFont));
        settings->setSansSerifFontFamily(family);

        family = fontFamilies.value(QWebSettings::CursiveFont,
                                    global->fontFamilies.value(QWebSettings::CursiveFont));
        settings->setCursiveFontFamily(family);

        family = fontFamilies.value(QWebSettings::FantasyFont,
                                    global->fontFamilies.value(QWebSettings::FantasyFont));
        settings->setFantasyFontFamily(family);

        int size = fontSizes.value(QWebSettings::MinimumFontSize,
                                   global->fontSizes.value(QWebSettings::MinimumFontSize));
        settings->setMinimumFontSize(size);

        size = fontSizes.value(QWebSettings::MinimumLogicalFontSize,
                                   global->fontSizes.value(QWebSettings::MinimumLogicalFontSize));
        settings->setMinimumLogicalFontSize(size);

        size = fontSizes.value(QWebSettings::DefaultFontSize,
                                   global->fontSizes.value(QWebSettings::DefaultFontSize));
        settings->setDefaultFontSize(size);

        size = fontSizes.value(QWebSettings::DefaultFixedFontSize,
                                   global->fontSizes.value(QWebSettings::DefaultFixedFontSize));
        settings->setDefaultFixedFontSize(size);

        bool value = attributes.value(QWebSettings::AutoLoadImages,
                                      global->attributes.value(QWebSettings::AutoLoadImages));
        settings->setLoadsImagesAutomatically(value);

        value = attributes.value(QWebSettings::DnsPrefetchEnabled,
                                 global->attributes.value(QWebSettings::DnsPrefetchEnabled));
        settings->setDNSPrefetchingEnabled(value);

        value = attributes.value(QWebSettings::JavascriptEnabled,
                                      global->attributes.value(QWebSettings::JavascriptEnabled));
        settings->setScriptEnabled(value);

/**/
        settings->setAcceleratedCompositingEnabled(false);
/**/

        bool showDebugVisuals = qgetenv("WEBKIT_SHOW_COMPOSITING_DEBUG_VISUALS") == "1";
        settings->setShowDebugBorders(showDebugVisuals);
        settings->setShowRepaintCounter(showDebugVisuals);

#if ENABLE(WEBGL)
        value = attributes.value(QWebSettings::WebGLEnabled,
                                 global->attributes.value(QWebSettings::WebGLEnabled));

        settings->setWebGLEnabled(value);
//#if ENABLE(CSS_SHADERS)
        // For now, enable CSS shaders when WebGL is enabled.
//        settings->setCSSCustomFilterEnabled(value);
//#endif
#endif
#if ENABLE(WEB_AUDIO)
        value = attributes.value(QWebSettings::WebAudioEnabled, global->attributes.value(QWebSettings::WebAudioEnabled));
        settings->setWebAudioEnabled(value);
#endif

        value = attributes.value(QWebSettings::CSSRegionsEnabled,
                                 global->attributes.value(QWebSettings::CSSRegionsEnabled));
        WebCore::RuntimeEnabledFeatures::sharedFeatures().setCSSRegionsEnabled(value);
        value = attributes.value(QWebSettings::CSSCompositingEnabled,
                                 global->attributes.value(QWebSettings::CSSCompositingEnabled));
        WebCore::RuntimeEnabledFeatures::sharedFeatures().setCSSCompositingEnabled(value);
        value = attributes.value(QWebSettings::CSSGridLayoutEnabled,
                                 global->attributes.value(QWebSettings::CSSGridLayoutEnabled));
        settings->setCSSGridLayoutEnabled(value);

        value = attributes.value(QWebSettings::HyperlinkAuditingEnabled,
                                 global->attributes.value(QWebSettings::HyperlinkAuditingEnabled));

        settings->setHyperlinkAuditingEnabled(value);
 
        value = attributes.value(QWebSettings::JavascriptCanOpenWindows,
                                      global->attributes.value(QWebSettings::JavascriptCanOpenWindows));
        settings->setJavaScriptCanOpenWindowsAutomatically(value);

        value = attributes.value(QWebSettings::JavascriptCanCloseWindows,
                                      global->attributes.value(QWebSettings::JavascriptCanCloseWindows));
        settings->setAllowScriptsToCloseWindows(value);

        value = attributes.value(QWebSettings::JavaEnabled,
                                      global->attributes.value(QWebSettings::JavaEnabled));
        settings->setJavaEnabled(value);

        value = attributes.value(QWebSettings::PluginsEnabled,
                                      global->attributes.value(QWebSettings::PluginsEnabled));
        settings->setPluginsEnabled(value);

        value = attributes.value(QWebSettings::PrivateBrowsingEnabled,
                                      global->attributes.value(QWebSettings::PrivateBrowsingEnabled));
        settings->setPrivateBrowsingEnabled(value);

        value = attributes.value(QWebSettings::SpatialNavigationEnabled,
                                      global->attributes.value(QWebSettings::SpatialNavigationEnabled));
        settings->setSpatialNavigationEnabled(value);

        value = attributes.value(QWebSettings::JavascriptCanAccessClipboard,
                                      global->attributes.value(QWebSettings::JavascriptCanAccessClipboard));
        settings->setDOMPasteAllowed(value);
        settings->setJavaScriptCanAccessClipboard(value);

        value = attributes.value(QWebSettings::DeveloperExtrasEnabled,
                                      global->attributes.value(QWebSettings::DeveloperExtrasEnabled));
        settings->setDeveloperExtrasEnabled(value);

        value = attributes.value(QWebSettings::FrameFlatteningEnabled,
                                      global->attributes.value(QWebSettings::FrameFlatteningEnabled));
        settings->setFrameFlatteningEnabled(value);

        QUrl location = !userStyleSheetLocation.isEmpty() ? userStyleSheetLocation : global->userStyleSheetLocation;
        settings->setUserStyleSheetLocation(WebCore::URL(location));

        QString encoding = !defaultTextEncoding.isEmpty() ? defaultTextEncoding: global->defaultTextEncoding;
        settings->setDefaultTextEncodingName(encoding);

        QString storagePath = !localStoragePath.isEmpty() ? localStoragePath : global->localStoragePath;
        settings->setLocalStorageDatabasePath(storagePath);

        if (mediaType.isEmpty())
            mediaType = global->mediaType;

        value = attributes.value(QWebSettings::PrintElementBackgrounds,
                                      global->attributes.value(QWebSettings::PrintElementBackgrounds));
        settings->setShouldPrintBackgrounds(value);

#if ENABLE(SQL_DATABASE)
        value = attributes.value(QWebSettings::OfflineStorageDatabaseEnabled,
                                      global->attributes.value(QWebSettings::OfflineStorageDatabaseEnabled));
        WebCore::DatabaseManager::manager().setIsAvailable(value);
#endif

        value = attributes.value(QWebSettings::OfflineWebApplicationCacheEnabled,
                                      global->attributes.value(QWebSettings::OfflineWebApplicationCacheEnabled));
        settings->setOfflineWebApplicationCacheEnabled(value);

        value = attributes.value(QWebSettings::LocalStorageEnabled,
                                      global->attributes.value(QWebSettings::LocalStorageEnabled));
        settings->setLocalStorageEnabled(value);

        value = attributes.value(QWebSettings::LocalContentCanAccessRemoteUrls,
                                      global->attributes.value(QWebSettings::LocalContentCanAccessRemoteUrls));
        settings->setAllowUniversalAccessFromFileURLs(value);

        value = attributes.value(QWebSettings::LocalContentCanAccessFileUrls,
                                      global->attributes.value(QWebSettings::LocalContentCanAccessFileUrls));
        settings->setAllowFileAccessFromFileURLs(value);

        value = attributes.value(QWebSettings::XSSAuditingEnabled,
                                      global->attributes.value(QWebSettings::XSSAuditingEnabled));
        settings->setXSSAuditorEnabled(value);

#if USE(TILED_BACKING_STORE)
        value = attributes.value(QWebSettings::TiledBackingStoreEnabled,
                                      global->attributes.value(QWebSettings::TiledBackingStoreEnabled));
        settings->setTiledBackingStoreEnabled(value);
#endif

#if ENABLE(SMOOTH_SCROLLING)
        value = attributes.value(QWebSettings::ScrollAnimatorEnabled,
                                      global->attributes.value(QWebSettings::ScrollAnimatorEnabled));
        settings->setScrollAnimatorEnabled(value);
#endif

        value = attributes.value(QWebSettings::CaretBrowsingEnabled,
                                      global->attributes.value(QWebSettings::CaretBrowsingEnabled));
        settings->setCaretBrowsingEnabled(value);

        value = attributes.value(QWebSettings::NotificationsEnabled,
                                      global->attributes.value(QWebSettings::NotificationsEnabled));
        settings->setNotificationsEnabled(value);

        value = attributes.value(QWebSettings::SiteSpecificQuirksEnabled,
                                      global->attributes.value(QWebSettings::SiteSpecificQuirksEnabled));
        settings->setNeedsSiteSpecificQuirks(value);

        settings->setUsesPageCache(WebCore::pageCache()->capacity());

/**/        
        quint32 blankColor = (aboutBlankBackGroundColor != 0xffffffff) ? aboutBlankBackGroundColor : global->aboutBlankBackGroundColor;
        unsigned char red = blankColor >> 16;
        unsigned char green = blankColor >> 8;
        unsigned char blue = blankColor;
        settings->setAboutBlankBackGroundColor(red, green, blue);
        
        settings->setInteractiveFormValidationEnabled(true);
/**/        
    } else {
        QList<QWebSettingsPrivate*> settings = *::allSettings();
        for (int i = 0; i < settings.count(); ++i)
            settings[i]->apply();
    }
}

/*!
    Returns the global settings object.

    Any setting changed on the default object is automatically applied to all
    QWebPage instances where the particular setting is not overriden already.
*/
QWebSettings* QWebSettings::globalSettings()
{
    static QWebSettings* global = 0;
    if (!global) {
        WebCore::initializeWebCoreQt();
        global = new QWebSettings;
    }
    return global;
}

/*!
    \class QWebSettings
    \since 4.4
    \brief The QWebSettings class provides an object to store the settings used
    by QWebPage and QWebFrame.

    \inmodule QtWebKit

    Each QWebPage object has its own QWebSettings object, which configures the
    settings for that page. If a setting is not configured, then it is looked
    up in the global settings object, which can be accessed using
    globalSettings().

    QWebSettings allows configuration of browser properties, such as font sizes and
    families, the location of a custom style sheet, and generic attributes like
    JavaScript and plugins. Individual attributes are set using the setAttribute()
    function. The \l{QWebSettings::WebAttribute}{WebAttribute} enum further describes
    each attribute.

    QWebSettings also configures global properties such as the web page memory
    cache, icon database, local database storage and offline
    applications storage.

    \section1 Enabling Plugins

    Support for browser plugins can enabled by setting the
    \l{QWebSettings::PluginsEnabled}{PluginsEnabled} attribute. For many applications,
    this attribute is enabled for all pages by setting it on the
    \l{globalSettings()}{global settings object}. Qt WebKit will always ignore this setting
    when processing Qt plugins. The decision to allow a Qt plugin is made by the client
    in its reimplementation of QWebPage::createPlugin().

    \section1 Web Application Support

    WebKit provides support for features specified in \l{HTML 5} that improve the
    performance and capabilities of Web applications. These include client-side
    (offline) storage and the use of a Web application cache.

    Client-side (offline) storage is an improvement over the use of cookies to
    store persistent data in Web applications. Applications can configure and
    enable the use of an offline storage database by calling the
    setOfflineStoragePath() with an appropriate file path, and can limit the quota
    for each application by calling setOfflineStorageDefaultQuota().

    \sa QWebPage::settings(), QWebView::settings(), {Web Browser}
*/

/*!
    \enum QWebSettings::FontFamily

    This enum describes the generic font families defined by CSS 2.
    For more information see the
    \l{http://www.w3.org/TR/REC-CSS2/fonts.html#generic-font-families}{CSS standard}.

    \value StandardFont
    \value FixedFont
    \value SerifFont
    \value SansSerifFont
    \value CursiveFont
    \value FantasyFont
*/

/*!
    \enum QWebSettings::FontSize

    This enum describes the font sizes configurable through QWebSettings.

    \value MinimumFontSize The hard minimum font size.
    \value MinimumLogicalFontSize The minimum logical font size that is applied
        when zooming out with QWebFrame::setTextSizeMultiplier().
    \value DefaultFontSize The default font size for regular text.
    \value DefaultFixedFontSize The default font size for fixed-pitch text.
*/

/*!
    \enum QWebSettings::ThirdPartyCookiePolicy

    This enum describes the policies configurable for accepting and sending
    third-party cookies. These are cookies that are set or retrieved when fetching
    a resource that is stored for a different registry-controlled domain from the page containing it.

    \value AlwaysAllowThirdPartyCookies Allow third-party resources to set and retrieve cookies.
    \value AlwaysBlockThirdPartyCookies Never allow third-party resources to set and retrieve cookies.
    \value AllowThirdPartyWithExistingCookies If the cookie jar already contains cookies
        from a third-party, allow it to set and retrieve new and existing cookies.

    \since QtWebKit 2,3
*/

/*!
    \enum QWebSettings::WebGraphic

    This enums describes the standard graphical elements used in webpages.

    \value MissingImageGraphic The replacement graphic shown when an image could not be loaded.
    \value MissingPluginGraphic The replacement graphic shown when a plugin could not be loaded.
    \value DefaultFrameIconGraphic The default icon for QWebFrame::icon().
    \value TextAreaSizeGripCornerGraphic The graphic shown for the size grip of text areas.
    \value DeleteButtonGraphic The graphic shown for the WebKit-Editing-Delete-Button in Deletion UI.
    \value InputSpeechButtonGraphic The graphic shown in input fields that support speech recognition.
    \value SearchCancelButtonGraphic The graphic shown for clearing the text in a search field.
    \value SearchCancelButtonPressedGraphic The graphic shown when SearchCancelButtonGraphic is pressed.
*/

/*!
    \enum QWebSettings::WebAttribute

    This enum describes various attributes that are configurable through QWebSettings.

    \value AutoLoadImages Specifies whether images are automatically loaded in
        web pages. This is enabled by default.
    \value DnsPrefetchEnabled Specifies whether Qt WebKit will try to pre-fetch DNS entries to
        speed up browsing. This only works as a global attribute. Only for Qt 4.6 and later. This is disabled by default.
    \value JavascriptEnabled Enables or disables the running of JavaScript
        programs. This is enabled by default
    \value JavaEnabled Enables or disables Java applets.
        Currently Java applets are not supported.
    \value PluginsEnabled Enables or disables plugins in Web pages (e.g. using NPAPI). Qt plugins
        with a mimetype such as "application/x-qt-plugin" are not affected by this setting. This is disabled by default.
    \value PrivateBrowsingEnabled Private browsing prevents WebKit from
        recording visited pages in the history and storing web page icons. This is disabled by default.
    \value JavascriptCanOpenWindows Specifies whether JavaScript programs
        can open new windows. This is disabled by default.
    \value JavascriptCanCloseWindows Specifies whether JavaScript programs
        can close windows. This is disabled by default.
    \value JavascriptCanAccessClipboard Specifies whether JavaScript programs
        can read or write to the clipboard. This is disabled by default.
    \value DeveloperExtrasEnabled Enables extra tools for Web developers.
        Currently this enables the "Inspect" element in the context menu as
        well as the use of QWebInspector which controls the web inspector
        for web site debugging. This is disabled by default.
    \value SpatialNavigationEnabled Enables or disables the Spatial Navigation
        feature, which consists in the ability to navigate between focusable
        elements in a Web page, such as hyperlinks and form controls, by using
        Left, Right, Up and Down arrow keys. For example, if a user presses the
        Right key, heuristics determine whether there is an element he might be
        trying to reach towards the right and which element he probably wants.
        This is disabled by default.
    \value LinksIncludedInFocusChain Specifies whether hyperlinks should be
        included in the keyboard focus chain. This is enabled by default.
    \value ZoomTextOnly Specifies whether the zoom factor on a frame applies
        only to the text or to all content. This is disabled by default.
    \value PrintElementBackgrounds Specifies whether the background color and images
        are also drawn when the page is printed. This is enabled by default.
    \value OfflineStorageDatabaseEnabled Specifies whether support for the HTML 5
        offline storage feature is enabled or not. This is disabled by default.
    \value OfflineWebApplicationCacheEnabled Specifies whether support for the HTML 5
        web application cache feature is enabled or not. This is disabled by default.
    \value LocalStorageEnabled Specifies whether support for the HTML 5
        local storage feature is enabled or not. This is disabled by default.
        (This value was introduced in 4.6.)
    \value LocalStorageDatabaseEnabled \e{This enum value is deprecated.} Use
        QWebSettings::LocalStorageEnabled instead.
    \value LocalContentCanAccessRemoteUrls Specifies whether locally loaded documents are
        allowed to access remote urls. This is disabled by default. For more information
        about security origins and local vs. remote content see QWebSecurityOrigin.
        (This value was introduced in 4.6.)
    \value LocalContentCanAccessFileUrls Specifies whether locally loaded documents are
        allowed to access other local urls. This is enabled by default. For more information
        about security origins and local vs. remote content see QWebSecurityOrigin.
    \value XSSAuditingEnabled Specifies whether load requests should be monitored for cross-site
        scripting attempts. Suspicious scripts will be blocked and reported in the inspector's
        JavaScript console. Enabling this feature might have an impact on performance
        and it is disabled by default.
    \value AcceleratedCompositingEnabled This feature, when used in conjunction with
        QGraphicsWebView, accelerates animations of web content. CSS animations of the transform and
        opacity properties will be rendered by composing the cached content of the animated elements.
        This is enabled by default.
    \value TiledBackingStoreEnabled This setting enables the tiled backing store feature
        for a QGraphicsWebView. With the tiled backing store enabled, the web page contents in and around
        the current visible area is speculatively cached to bitmap tiles. The tiles are automatically kept
        in sync with the web page as it changes. Enabling tiling can significantly speed up painting heavy 
        operations like scrolling. Enabling the feature increases memory consumption. It does not work well 
        with contents using CSS fixed positioning (see also \l{QGraphicsWebView::}{resizesToContents} property).
        \l{QGraphicsWebView::}{tiledBackingStoreFrozen} property allows application to temporarily
        freeze the contents of the backing store. This is disabled by default.
    \value FrameFlatteningEnabled With this setting each subframe is expanded to its contents.
        On touch devices, it is desired to not have any scrollable sub parts of the page
        as it results in a confusing user experience, with scrolling sometimes scrolling sub parts
        and at other times scrolling the page itself. For this reason iframes and framesets are
        barely usable on touch devices. This will flatten all the frames to become one scrollable page.
        This is disabled by default.
    \value SiteSpecificQuirksEnabled This setting enables WebKit's workaround for broken sites. It is
        enabled by default.
    \value ScrollAnimatorEnabled This setting enables animated scrolling. It is disabled by default.
    \value CaretBrowsingEnabled This setting enables caret browsing. It is disabled by default.
    \value NotificationsEnabled Specifies whether support for the HTML 5 web notifications is enabled
        or not. This is enabled by default.
*/

/*!
    \internal
*/
QWebSettings::QWebSettings()
/**/
    //: d(new QWebSettingsPrivate)
      : d(new QWebSettingsPrivate(WebCore::Settings::create(0).leakRef()))
/**/
{
    // Initialize our global defaults
    d->fontSizes.insert(QWebSettings::MinimumFontSize, 0);
    d->fontSizes.insert(QWebSettings::MinimumLogicalFontSize, 0);
    d->fontSizes.insert(QWebSettings::DefaultFontSize, 16);
    d->fontSizes.insert(QWebSettings::DefaultFixedFontSize, 13);

    QFont defaultFont;
    defaultFont.setStyleHint(QFont::Serif);
    d->fontFamilies.insert(QWebSettings::StandardFont, defaultFont.defaultFamily());
    d->fontFamilies.insert(QWebSettings::SerifFont, defaultFont.defaultFamily());

    defaultFont.setStyleHint(QFont::Fantasy);
    d->fontFamilies.insert(QWebSettings::FantasyFont, defaultFont.defaultFamily());

    defaultFont.setStyleHint(QFont::Cursive);
    d->fontFamilies.insert(QWebSettings::CursiveFont, defaultFont.defaultFamily());

    defaultFont.setStyleHint(QFont::SansSerif);
    d->fontFamilies.insert(QWebSettings::SansSerifFont, defaultFont.defaultFamily());

    defaultFont.setStyleHint(QFont::Monospace);
    d->fontFamilies.insert(QWebSettings::FixedFont, defaultFont.defaultFamily());

    d->attributes.insert(QWebSettings::AutoLoadImages, true);
    d->attributes.insert(QWebSettings::DnsPrefetchEnabled, false);
    d->attributes.insert(QWebSettings::JavascriptEnabled, true);
    d->attributes.insert(QWebSettings::SpatialNavigationEnabled, false);
    d->attributes.insert(QWebSettings::LinksIncludedInFocusChain, true);
    d->attributes.insert(QWebSettings::ZoomTextOnly, false);
    d->attributes.insert(QWebSettings::PrintElementBackgrounds, true);
    d->attributes.insert(QWebSettings::OfflineStorageDatabaseEnabled, false);
    d->attributes.insert(QWebSettings::OfflineWebApplicationCacheEnabled, false);
    d->attributes.insert(QWebSettings::LocalStorageEnabled, false);
    d->attributes.insert(QWebSettings::LocalContentCanAccessRemoteUrls, false);
    d->attributes.insert(QWebSettings::LocalContentCanAccessFileUrls, true);
/**/
#if 0
    d->attributes.insert(QWebSettings::AcceleratedCompositingEnabled, true);
    d->attributes.insert(QWebSettings::WebGLEnabled, true);
#else
    d->attributes.insert(QWebSettings::AcceleratedCompositingEnabled, false);
    d->attributes.insert(QWebSettings::WebGLEnabled, false);
#endif
/**/
    d->attributes.insert(QWebSettings::WebAudioEnabled, false);
    d->attributes.insert(QWebSettings::CSSRegionsEnabled, true);
    d->attributes.insert(QWebSettings::CSSCompositingEnabled, true);
    d->attributes.insert(QWebSettings::CSSGridLayoutEnabled, false);
    d->attributes.insert(QWebSettings::HyperlinkAuditingEnabled, false);
    d->attributes.insert(QWebSettings::TiledBackingStoreEnabled, false);
    d->attributes.insert(QWebSettings::FrameFlatteningEnabled, false);
    d->attributes.insert(QWebSettings::SiteSpecificQuirksEnabled, true);
    d->attributes.insert(QWebSettings::ScrollAnimatorEnabled, false);
    d->attributes.insert(QWebSettings::CaretBrowsingEnabled, false);
    d->attributes.insert(QWebSettings::NotificationsEnabled, true);
    d->offlineStorageDefaultQuota = 5 * 1024 * 1024;
    d->defaultTextEncoding = QLatin1String("iso-8859-1");
    d->thirdPartyCookiePolicy = AlwaysAllowThirdPartyCookies;
/**/    
    d->aboutBlankBackGroundColor = 0xffffffff;
/**/
}

/*!
    \internal
*/
QWebSettings::QWebSettings(WebCore::Settings* settings)
    : d(new QWebSettingsPrivate(settings))
{
    d->settings = settings;
/**/
    d->aboutBlankBackGroundColor = 0xffffffff;
/**/
    d->apply();
    allSettings()->append(d);
}

/*!
    \internal
*/
QWebSettings::~QWebSettings()
{
    if (d->settings)
        allSettings()->removeAll(d);

    delete d;
}

/*!
    Sets the font size for \a type to \a size.
*/
void QWebSettings::setFontSize(FontSize type, int size)
{
    d->fontSizes.insert(type, size);
    d->apply();
}

/*!
    Returns the default font size for \a type.
*/
int QWebSettings::fontSize(FontSize type) const
{
    int defaultValue = 0;
    if (d->settings) {
        QWebSettingsPrivate* global = QWebSettings::globalSettings()->d;
        defaultValue = global->fontSizes.value(type);
    }
    return d->fontSizes.value(type, defaultValue);
}

/*!
    Resets the font size for \a type to the size specified in the global
    settings object.

    This function has no effect on the global QWebSettings instance.
*/
void QWebSettings::resetFontSize(FontSize type)
{
    if (d->settings) {
        d->fontSizes.remove(type);
        d->apply();
    }
}

/*!
    Specifies the location of a user stylesheet to load with every web page.

    The \a location must be either a path on the local filesystem, or a data URL
    with UTF-8 and Base64 encoded data, such as:

    "data:text/css;charset=utf-8;base64,cCB7IGJhY2tncm91bmQtY29sb3I6IHJlZCB9Ow=="

    \note If the base64 data is not valid, the style will not be applied.

    \sa userStyleSheetUrl()
*/
void QWebSettings::setUserStyleSheetUrl(const QUrl& location)
{
    d->userStyleSheetLocation = location;
    d->apply();
}

/*!
    Returns the location of the user stylesheet.

    \sa setUserStyleSheetUrl()
*/
QUrl QWebSettings::userStyleSheetUrl() const
{
    return d->userStyleSheetLocation;
}

/*!
    \since 4.6
    Specifies the default text encoding system.

    The \a encoding, must be a string describing an encoding such as "utf-8",
    "iso-8859-1", etc. If left empty a default value will be used. For a more
    extensive list of encoding names see \l{QTextCodec}

    \sa defaultTextEncoding()
*/
void QWebSettings::setDefaultTextEncoding(const QString& encoding)
{
    d->defaultTextEncoding = encoding;
    d->apply();
}

/*!
    \since 4.6
    Returns the default text encoding.

    \sa setDefaultTextEncoding()
*/
QString QWebSettings::defaultTextEncoding() const
{
    return d->defaultTextEncoding;
}

/*!
    Sets the path of the icon database to \a path. The icon database is used
    to store "favicons" associated with web sites.

    \a path must point to an existing directory.

    Setting an empty path disables the icon database.

    \sa iconDatabasePath(), clearIconDatabase()
*/
void QWebSettings::setIconDatabasePath(const QString& path)
{
    WebCore::initializeWebCoreQt();
#if ENABLE(ICONDATABASE)
    // Make sure that IconDatabaseClientQt is instantiated.
    WebCore::IconDatabaseClientQt::instance();
#endif

    WebCore::IconDatabase::delayDatabaseCleanup();

    WebCore::IconDatabaseBase& db = WebCore::iconDatabase();

    if (!path.isEmpty()) {
        db.setEnabled(true);
        if (db.isOpen())
            db.close();
        QFileInfo info(path);
        if (info.isDir() && info.isWritable())
            db.open(path, WebCore::IconDatabase::defaultDatabaseFilename());
    } else {
        db.setEnabled(false);
        db.close();
    }
}

/*!
    Returns the path of the icon database or an empty string if the icon
    database is disabled.

    \sa setIconDatabasePath(), clearIconDatabase()
*/
QString QWebSettings::iconDatabasePath()
{
    WebCore::initializeWebCoreQt();
    if (WebCore::iconDatabase().isEnabled() && WebCore::iconDatabase().isOpen())
        return WebCore::iconDatabase().databasePath();
    else
        return QString();
}

/*!
    Clears the icon database.
*/
void QWebSettings::clearIconDatabase()
{
    WebCore::initializeWebCoreQt();
    if (WebCore::iconDatabase().isEnabled() && WebCore::iconDatabase().isOpen())
        WebCore::iconDatabase().removeAllIcons();
}

/*!
    Returns the web site's icon for \a url.

    If the web site does not specify an icon \b OR if the icon is not in the
    database, a null QIcon is returned.

    \note The returned icon's size is arbitrary.

    \sa setIconDatabasePath()
*/
QIcon QWebSettings::iconForUrl(const QUrl& url)
{
    WebCore::initializeWebCoreQt();
    QPixmap* icon = WebCore::iconDatabase().synchronousNativeIconForPageURL(WebCore::URL(url).string(),
                                WebCore::IntSize(16, 16));
    if (!icon)
        return QIcon();

    return* icon;
}

/*
    Returns the plugin database object.

QWebPluginDatabase *QWebSettings::pluginDatabase()
{
    WebCore::initializeWebCoreQt();
    static QWebPluginDatabase* database = 0;
    if (!database)
        database = new QWebPluginDatabase();
    return database;
}
*/

static const char* resourceNameForWebGraphic(QWebSettings::WebGraphic type)
{
    switch (type) {
    case QWebSettings::MissingImageGraphic: return "missingImage";
    case QWebSettings::MissingPluginGraphic: return "nullPlugin";
    case QWebSettings::DefaultFrameIconGraphic: return "urlIcon";
    case QWebSettings::TextAreaSizeGripCornerGraphic: return "textAreaResizeCorner";
    case QWebSettings::DeleteButtonGraphic: return "deleteButton";
    case QWebSettings::InputSpeechButtonGraphic: return "inputSpeech";
    case QWebSettings::SearchCancelButtonGraphic: return "searchCancelButton";
    case QWebSettings::SearchCancelButtonPressedGraphic: return "searchCancelButtonPressed";
    }
    return 0;
}

/*!
    Sets \a graphic to be drawn when Qt WebKit needs to draw an image of the
    given \a type.

    For example, when an image cannot be loaded, the pixmap specified by
    \l{QWebSettings::WebGraphic}{MissingImageGraphic} is drawn instead.

    \sa webGraphic()
*/
void QWebSettings::setWebGraphic(WebGraphic type, const QPixmap& graphic)
{
    WebCore::initializeWebCoreQt();
    WebCore::Image::setPlatformResource(resourceNameForWebGraphic(type), graphic);
}

/*!
    Returns a previously set pixmap used to draw replacement graphics of the
    specified \a type.

    \sa setWebGraphic()
*/
QPixmap QWebSettings::webGraphic(WebGraphic type)
{
    WebCore::initializeWebCoreQt();
    RefPtr<WebCore::Image> img = WebCore::Image::loadPlatformResource(resourceNameForWebGraphic(type));
    if (!img)
        return QPixmap();
    QPixmap* pixmap = img->nativeImageForCurrentFrame();
    if (!pixmap)
        return QPixmap();
    return *pixmap;
}

/*!
    Frees up as much memory as possible by calling the JavaScript garbage collector and cleaning all memory caches such
    as page, object and font cache.

    \since 4.6
 */
void QWebSettings::clearMemoryCaches()
{
    WebCore::initializeWebCoreQt();
    // Turn the cache on and off.  Disabling the object cache will remove all
    // resources from the cache.  They may still live on if they are referenced
    // by some Web page though.
    if (!WebCore::memoryCache()->disabled()) {
        WebCore::memoryCache()->setDisabled(true);
        WebCore::memoryCache()->setDisabled(false);
    }

    int pageCapacity = WebCore::pageCache()->capacity();
    // Setting size to 0, makes all pages be released.
    WebCore::pageCache()->setCapacity(0);
    WebCore::pageCache()->setCapacity(pageCapacity);

    // Invalidating the font cache and freeing all inactive font data.
    WebCore::fontCache()->invalidate();

    // Empty the Cross-Origin Preflight cache
    WebCore::CrossOriginPreflightResultCache::shared().empty();

#if USE(JSC)
    // Drop JIT compiled code from ExecutableAllocator.
    WebCore::gcController().discardAllCompiledCode();
    // Garbage Collect to release the references of CachedResource from dead objects.
    WebCore::gcController().garbageCollectNow();
#else
    WebCore::V8GCController::collectGarbage();
#endif

    // FastMalloc has lock-free thread specific caches that can only be cleared from the thread itself.
    WebCore::StorageThread::releaseFastMallocFreeMemoryInAllThreads();

    WebCore::WorkerThread::releaseFastMallocFreeMemoryInAllThreads();

    WTF::releaseFastMallocFreeMemory();        
}

/*!
    Sets the maximum number of pages to hold in the memory page cache to \a pages.

    The Page Cache allows for a nicer user experience when navigating forth or back
    to pages in the forward/back history, by pausing and resuming up to \a pages.

    For more information about the feature, please refer to:

    http://webkit.org/blog/427/webkit-page-cache-i-the-basics/
*/
void QWebSettings::setMaximumPagesInCache(int pages)
{
    QWebSettingsPrivate* global = QWebSettings::globalSettings()->d;
    WebCore::pageCache()->setCapacity(qMax(0, pages));
    global->apply();
}

/*!
    Returns the maximum number of web pages that are kept in the memory cache.
*/
int QWebSettings::maximumPagesInCache()
{
    WebCore::initializeWebCoreQt();
    return WebCore::pageCache()->capacity();
}

/*!
   Specifies the capacities for the memory cache for dead objects such as
   stylesheets or scripts.

   The \a cacheMinDeadCapacity specifies the \e minimum number of bytes that
   dead objects should consume when the cache is under pressure.

   \a cacheMaxDead is the \e maximum number of bytes that dead objects should
   consume when the cache is \b not under pressure.

   \a totalCapacity specifies the \e maximum number of bytes that the cache
   should consume \b overall.

   The cache is enabled by default. Calling setObjectCacheCapacities(0, 0, 0)
   will disable the cache. Calling it with one non-zero enables it again.
*/
void QWebSettings::setObjectCacheCapacities(int cacheMinDeadCapacity, int cacheMaxDead, int totalCapacity)
{
    WebCore::initializeWebCoreQt();
    bool disableCache = !cacheMinDeadCapacity && !cacheMaxDead && !totalCapacity;
    WebCore::memoryCache()->setDisabled(disableCache);

    WebCore::memoryCache()->setCapacities(qMax(0, cacheMinDeadCapacity),
                                    qMax(0, cacheMaxDead),
                                    qMax(0, totalCapacity));
    WebCore::memoryCache()->setDeadDecodedDataDeletionInterval(disableCache ? 0 : 60);
}

/*!
    Sets the third-party cookie policy, the default is AlwaysAllowThirdPartyCookies.
*/
void QWebSettings::setThirdPartyCookiePolicy(ThirdPartyCookiePolicy policy)
{
    d->thirdPartyCookiePolicy = policy;
}

/*!
    Returns the third-party cookie policy.
*/
QWebSettings::ThirdPartyCookiePolicy QWebSettings::thirdPartyCookiePolicy() const
{
    return d->thirdPartyCookiePolicy;
}

/*!
    Sets the CSS media type to \a type.
    
    Setting this will override the normal value of the CSS media property.
    
    \note Setting the value to null QString will restore the default value.
*/
void QWebSettings::setCSSMediaType(const QString& type)
{
    d->mediaType = type;
    d->apply();
}

/*!
    Returns the current CSS media type.
    
    \note It will only return the value set through setCSSMediaType and not the one used internally.
*/
QString QWebSettings::cssMediaType() const
{
    return d->mediaType;
}

/*!
    Sets the actual font family to \a family for the specified generic family,
    \a which.
*/
void QWebSettings::setFontFamily(FontFamily which, const QString& family)
{
    d->fontFamilies.insert(which, family);
    d->apply();
}

/*!
    Returns the actual font family for the specified generic font family,
    \a which.
*/
QString QWebSettings::fontFamily(FontFamily which) const
{
    QString defaultValue;
    if (d->settings) {
        QWebSettingsPrivate* global = QWebSettings::globalSettings()->d;
        defaultValue = global->fontFamilies.value(which);
    }
    return d->fontFamilies.value(which, defaultValue);
}

/*!
    Resets the actual font family specified by \a which to the one set
    in the global QWebSettings instance.

    This function has no effect on the global QWebSettings instance.
*/
void QWebSettings::resetFontFamily(FontFamily which)
{
    if (d->settings) {
        d->fontFamilies.remove(which);
        d->apply();
    }
}

/*!
    \fn void QWebSettings::setAttribute(WebAttribute attribute, bool on)

    Enables or disables the specified \a attribute feature depending on the
    value of \a on.
*/
void QWebSettings::setAttribute(WebAttribute attr, bool on)
{
    d->attributes.insert(attr, on);
    d->apply();
}

/*!
    \fn bool QWebSettings::testAttribute(WebAttribute attribute) const

    Returns true if \a attribute is enabled; otherwise returns false.
*/
bool QWebSettings::testAttribute(WebAttribute attr) const
{
    bool defaultValue = false;
    if (d->settings) {
        QWebSettingsPrivate* global = QWebSettings::globalSettings()->d;
        defaultValue = global->attributes.value(attr);
    }
    return d->attributes.value(attr, defaultValue);
}

/*!
    \fn void QWebSettings::resetAttribute(WebAttribute attribute)

    Resets the setting of \a attribute to the value specified in the
    global QWebSettings instance.

    This function has no effect on the global QWebSettings instance.

    \sa globalSettings()
*/
void QWebSettings::resetAttribute(WebAttribute attr)
{
    if (d->settings) {
        d->attributes.remove(attr);
        d->apply();
    }
}

/*!
    \since 4.5

    Sets \a path as the save location for HTML5 client-side database storage data.

    \a path must point to an existing directory.

    Setting an empty path disables the feature.

    Support for client-side databases can enabled by setting the
    \l{QWebSettings::OfflineStorageDatabaseEnabled}{OfflineStorageDatabaseEnabled} attribute.

    \sa offlineStoragePath()
*/
void QWebSettings::setOfflineStoragePath(const QString& path)
{
    WebCore::initializeWebCoreQt();
#if ENABLE(SQL_DATABASE)
    WebCore::DatabaseManager::manager().setDatabaseDirectoryPath(path);
#endif
}

/*!
    \since 4.5

    Returns the path of the HTML5 client-side database storage or an empty string if the
    feature is disabled.

    \sa setOfflineStoragePath()
*/
QString QWebSettings::offlineStoragePath()
{
    WebCore::initializeWebCoreQt();
#if ENABLE(SQL_DATABASE)
    return WebCore::DatabaseManager::manager().databaseDirectoryPath();
#else
    return QString();
#endif
}

/*!
    \since 4.5

    Sets the value of the default quota for new offline storage databases
    to \a maximumSize.
*/
void QWebSettings::setOfflineStorageDefaultQuota(qint64 maximumSize)
{
    QWebSettings::globalSettings()->d->offlineStorageDefaultQuota = maximumSize;
}

/*!
    \since 4.5

    Returns the value of the default quota for new offline storage databases.
*/
qint64 QWebSettings::offlineStorageDefaultQuota()
{
    return QWebSettings::globalSettings()->d->offlineStorageDefaultQuota;
}

/*!
    \since 4.6

    Sets the path for HTML5 offline web application cache storage to \a path.

    An application cache acts like an HTTP cache in some sense. For documents
    that use the application cache via JavaScript, the loader engine will
    first ask the application cache for the contents, before hitting the
    network.

    The feature is described in details at:
    http://dev.w3.org/html5/spec/Overview.html#appcache

    \a path must point to an existing directory.

    Setting an empty path disables the feature.

    Support for offline web application cache storage can enabled by setting the
    \l{QWebSettings::OfflineWebApplicationCacheEnabled}{OfflineWebApplicationCacheEnabled} attribute.

    \sa offlineWebApplicationCachePath()
*/
void QWebSettings::setOfflineWebApplicationCachePath(const QString& path)
{
    WebCore::initializeWebCoreQt();
    WebCore::cacheStorage().setCacheDirectory(path);
}

/*!
    \since 4.6

    Returns the path of the HTML5 offline web application cache storage
    or an empty string if the feature is disabled.

    \sa setOfflineWebApplicationCachePath()
*/
QString QWebSettings::offlineWebApplicationCachePath()
{
    WebCore::initializeWebCoreQt();
    return WebCore::cacheStorage().cacheDirectory();
}

/*!
    \since 4.6

    Sets the value of the quota for the offline web application cache
    to \a maximumSize.
*/
void QWebSettings::setOfflineWebApplicationCacheQuota(qint64 maximumSize)
{
    WebCore::initializeWebCoreQt();
    WebCore::cacheStorage().setMaximumSize(maximumSize);
/**/
    int64_t overFlowSpace = 0 ;
    if(!WebCore::cacheStorage().getOverFlowSpace(0 , overFlowSpace))
    {
        return;
    }
    if(overFlowSpace > 0)
    {
        WebCore::cacheStorage().screwDataBaseSize(overFlowSpace);
    }
/**/
}

/*!
    \since 4.6

    Returns the value of the quota for the offline web application cache.
*/
qint64 QWebSettings::offlineWebApplicationCacheQuota()
{
    WebCore::initializeWebCoreQt();
    return WebCore::cacheStorage().maximumSize();
}

/*!
    \since 4.6

    Sets the path for HTML5 local storage to \a path.
    
    For more information on HTML5 local storage see the
    \l{http://www.w3.org/TR/webstorage/#the-localstorage-attribute}{Web Storage standard}.
    
    Support for local storage can enabled by setting the
    \l{QWebSettings::LocalStorageEnabled}{LocalStorageEnabled} attribute.     

    \sa localStoragePath()
*/
void QWebSettings::setLocalStoragePath(const QString& path)
{
    d->localStoragePath = path;
    d->apply();
}

/*!
    \since 4.6

    Returns the path for HTML5 local storage.
    
    \sa setLocalStoragePath()
*/
QString QWebSettings::localStoragePath() const
{
    return d->localStoragePath;
}

/*!
    \since 4.6

    Enables WebKit data persistence and sets the path to \a path.
    If \a path is empty, the user-specific data location specified by
    \l{QDesktopServices::DataLocation}{DataLocation} will be used instead.

    This method will simultaneously set and enable the iconDatabasePath(),
    localStoragePath(), offlineStoragePath() and offlineWebApplicationCachePath().

    \sa localStoragePath()
*/
void QWebSettings::enablePersistentStorage(const QString& path)
{
    WebCore::initializeWebCoreQt();
#ifndef QT_NO_DESKTOPSERVICES
    QString storagePath;

    if (path.isEmpty()) {

/**/
#if HAVE(QT5)
        storagePath = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
#else
        storagePath = QDesktopServices::storageLocation(QDesktopServices::DataLocation);
#endif
/**/
        if (storagePath.isEmpty())
            storagePath = WebCore::pathByAppendingComponent(QDir::homePath(), QCoreApplication::applicationName());
    } else
        storagePath = path;

    WebCore::makeAllDirectories(storagePath);

    QWebSettings::setIconDatabasePath(storagePath);
    QWebSettings::setOfflineWebApplicationCachePath(storagePath);
    QWebSettings::setOfflineStoragePath(WebCore::pathByAppendingComponent(storagePath, "Databases"));
    QWebSettings::globalSettings()->setLocalStoragePath(WebCore::pathByAppendingComponent(storagePath, "LocalStorage"));
    QWebSettings::globalSettings()->setAttribute(QWebSettings::LocalStorageEnabled, true);
    QWebSettings::globalSettings()->setAttribute(QWebSettings::OfflineStorageDatabaseEnabled, true);
    QWebSettings::globalSettings()->setAttribute(QWebSettings::OfflineWebApplicationCacheEnabled, true);

#if ENABLE(NETSCAPE_PLUGIN_METADATA_CACHE)
    // All applications can share the common QtWebkit cache file(s).
    // Path is not configurable and uses QDesktopServices::CacheLocation by default.
/**/
#if HAVE(QT5)
    QString cachePath = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
#else
    QString cachePath = QDesktopServices::storageLocation(QDesktopServices::CacheLocation); 
#endif
/**/
    WebCore::makeAllDirectories(cachePath);

    QFileInfo info(cachePath);
    if (info.isDir() && info.isWritable()) {
        WebCore::PluginDatabase::setPersistentMetadataCacheEnabled(true);
        WebCore::PluginDatabase::setPersistentMetadataCachePath(cachePath);
    }
#endif
#endif
}

/**/
void QWebSettings::setSendReferer(bool referer)
{
    if (d->settings)
    {
        d->settings->setClearReferer(!referer);
    }
}

bool QWebSettings::sendReferer()
{
    bool referer = false;
    if (d->settings)
    {
        referer = d->settings->clearReferer();
    }
    return !referer;
}

#define FJIB_NOT_IMPLEMENTED() do{ qWarning() << "notImplemented:" << __func__; }while(0)

void QWebSettings::setTimeOut(const unsigned long timeout)
{
#ifdef USE_FJ_QT
    QNetworkAccessManager::setTimeOut(timeout);
#else
	FJIB_NOT_IMPLEMENTED();
#endif
}

unsigned long QWebSettings::timeOut()
{
#ifdef USE_FJ_QT
    return QNetworkAccessManager::timeOut();
#else
	FJIB_NOT_IMPLEMENTED();
	return 0L;
#endif
}

void QWebSettings::setSynchronousTimeout(unsigned long timeout)
{
#ifdef USE_FJ_QT
    QNetworkAccessManager::setSynchronousTimeout(timeout);
#else
	FJIB_NOT_IMPLEMENTED();
#endif
}

unsigned long QWebSettings::synchronousTimeout()
{
#ifdef USE_FJ_QT
    return QNetworkAccessManager::synchronousTimeout();
#else
	FJIB_NOT_IMPLEMENTED();
	return 0L;
#endif
}

void QWebSettings::setCookiePath(const QString& path)
{
#ifdef USE_FJ_QT
    QNetworkCookieJar::setCookiePath(path);
#else
	FJIB_NOT_IMPLEMENTED();
#endif
}

const QString& QWebSettings::cookiePath()
{
#ifdef USE_FJ_QT
    return QNetworkCookieJar::cookiePath();
#else
	FJIB_NOT_IMPLEMENTED();
	static QString s_tmp = QString::fromLatin1("/tmp");
	return s_tmp;
#endif
}

void QWebSettings::setCookieSize(const unsigned long cookieSize)
{
#ifdef USE_FJ_QT
    QNetworkCookieJar::setCookieSize(cookieSize);
#else
	FJIB_NOT_IMPLEMENTED();
#endif
}

unsigned long QWebSettings::cookieSize()
{
#ifdef USE_FJ_QT
    return QNetworkCookieJar::cookieSize();
#else
	FJIB_NOT_IMPLEMENTED();
	return 0L;
#endif
}

void QWebSettings::setApplicationCacheMaxSize(int64_t size)
{
    if (d->settings)
    {
        d->settings->setApplicationCacheMaxSize(size);
    }
}

int64_t QWebSettings::applicationCacheMaxSize()
{
    int64_t size = 0;
    if (d->settings)
    {
        size = d->settings->applicationCacheMaxSize();
    }
	return size;
}

void QWebSettings::clearApplicationCache()
{
    if (d->settings)
    {
        d->settings->clearApplicationCache();
    }
}

void QWebSettings::deleteOfflineWebApplicationCacheFiles()
{
    WebCore::Settings::deleteOfflineWebApplicationCacheFiles();
}

void QWebSettings::setWebSocketEnabled(bool enabled)
{
#if ENABLE(WEB_SOCKETS)
	WebSocket::setIsAvailable(enabled);
#endif
}

bool QWebSettings::webSocketEnabled()
{
#if ENABLE(WEB_SOCKETS)
	return WebSocket::isAvailable();
#else
	return false;
#endif
}

void QWebSettings::setUsePlatformNppValues(bool use)
{
	if(d->settings){
		d->settings->setUsePlatformNppValues(use);
	}
}

bool QWebSettings::usePlatformNppValues()
{
	bool retval = false;
	if(d->settings){
		retval = d->settings->usePlatformNppValues();
	}
	return retval;
}

void QWebSettings::setPlatformNPPValues(QVector<QNPPValue> values)
{
	if(d->settings && d->settings->usePlatformNppValues()){
		Vector<NPPValue> aValues;

		for(int i = 0; i < values.count(); i++){
			NPPValue v;
			v.nppVariable = values[i].nppVariable;
			v.nppValue = values[i].nppValue;
			aValues.append(v);
		}
		d->settings->setPlatformNPPValues(aValues);
	}
}

QVector<QNPPValue>& QWebSettings::platformNPPValues()
{
	d->m_nppValue.clear();

	if(d->settings && d->settings->usePlatformNppValues()){
		Vector<NPPValue> values = d->settings->platformNPPValues();

		for(int i = 0; i < values.size(); i++){
			NPPValue v = values.at(i);
			QNPPValue qv;
			qv.nppVariable = v.nppVariable;
			qv.nppValue = v.nppValue;
			d->m_nppValue.append(qv);
		}
	}

	return d->m_nppValue;
}

void QWebSettings::setAboutBlankBackGroundColor(quint8 red, quint8 green, quint8 blue)
{
    d->aboutBlankBackGroundColor = (0xff000000 | red << 16 | green << 8 | blue);
    d->apply();
}

quint32 QWebSettings::aboutBlankBackGroundColor()
{
    return d->aboutBlankBackGroundColor & 0X00ffffff;
}

void QWebSettings::setEnableInputFileDialog(bool on)
{
	if(d->settings) {
		d->settings->setEnableInputFileDialog(on);
	}
}

bool QWebSettings::enableInputFileDialog()
{
	bool retval;
	if(d->settings) {
		retval = d->settings->enableInputFileDialog();
	}
	else {
		retval = true;
	}
	return retval;
}

void QWebSettings::setShouldDisplayCaptions(bool enabled)
{
#if ENABLE(VIDEO_TRACK)
    if(d->settings) {
        d->settings->setShouldDisplayCaptions(enabled);
    }
#endif
}

bool QWebSettings::shouldDisplayCaptions()
{
#if ENABLE(VIDEO_TRACK)
    bool retval;
    if(d->settings) {
        retval = d->settings->shouldDisplayCaptions();
    }
    else {
        retval = false;
    }
    return retval;
#else
	return false;
#endif
}

void QWebSettings::setShouldDisplaySubtitles(bool enabled)
{
#if ENABLE(VIDEO_TRACK)
    if(d->settings) {
        d->settings->setShouldDisplaySubtitles(enabled);
    }
#endif
}

bool QWebSettings::shouldDisplaySubtitles()
{
#if ENABLE(VIDEO_TRACK)
    bool retval;
    if(d->settings) {
        retval = d->settings->shouldDisplaySubtitles();
    }
    else {
        retval = false;
    }
    return retval;
#else
	return false;
#endif
}

void QWebSettings::setShouldDisplayTextDescriptions(bool enabled)
{
#if ENABLE(VIDEO_TRACK)
    if(d->settings) {
        d->settings->setShouldDisplayTextDescriptions(enabled);
    }
#endif
}

bool QWebSettings::shouldDisplayTextDescriptions()
{
#if ENABLE(VIDEO_TRACK)
    bool retval;
    if(d->settings) {
        retval = d->settings->shouldDisplayTextDescriptions();
    }
    else {
        retval = false;
    }
    return retval;
#else
	return false;
#endif
}
/**/

void QWebSettings::setPDFPasswordRetryCount(unsigned int count)
{
#if ENABLE(FJIB_PDF)
    WebCore::Settings::setPDFPasswordRetryCount(count);
#endif
}

unsigned int QWebSettings::pdfPasswordRetryCount()
{
#if ENABLE(FJIB_PDF)
    return WebCore::Settings::pdfPasswordRetryCount();
#else
	return 0;
#endif
}

void QWebSettings::setPDFToolBarHTMLSource(const QString& html)
{
#if ENABLE(FJIB_PDF)
    WebCore::Settings::setPDFToolBarHTMLSource(html);
#endif
}

QString QWebSettings::pdfToolBarHTMLSource()
{
#if ENABLE(FJIB_PDF)
    return WebCore::Settings::pdfToolBarHTMLSource();
#else
	return QString::fromUtf8("");
#endif
}

void QWebSettings::setPDFToolBarHeight(unsigned int height)
{
#if ENABLE(FJIB_PDF)
    WebCore::Settings::setPDFToolBarHeight(height);
#endif
}

unsigned int QWebSettings::pdfToolBarHeight()
{
#if ENABLE(FJIB_PDF)
    return WebCore::Settings::pdfToolBarHeight();
#else
	return 0;
#endif
}

void QWebSettings::setPDFZoomInMax(float zoomInMax)
{
#if ENABLE(FJIB_PDF)
    WebCore::Settings::setPDFZoomInMax(zoomInMax);
#endif
}

float QWebSettings::pdfZoomInMax()
{
#if ENABLE(FJIB_PDF)
    return WebCore::Settings::pdfZoomInMax();
#else
	return 0.0f;
#endif
}

void QWebSettings::setPDFZoomOutMin(float zoomOutMin)
{
#if ENABLE(FJIB_PDF)
    WebCore::Settings::setPDFZoomOutMin(zoomOutMin);
#endif
}

float QWebSettings::pdfZoomOutMin()
{
#if ENABLE(FJIB_PDF)
    return WebCore::Settings::pdfZoomOutMin();
#else
	return 0.0f;
#endif
}

void QWebSettings::setPDFZoomStep(float zoomStep)
{
#if ENABLE(FJIB_PDF)
    WebCore::Settings::setPDFZoomStep(zoomStep);
#endif
}

float QWebSettings::pdfZoomStep()
{
#if ENABLE(FJIB_PDF)
    return WebCore::Settings::pdfZoomStep();
#else
	return 0.0f;
#endif
}

void QWebSettings::setWatchDogEnabled(bool on)
{
    WebCore::Settings::setWatchDogEnabled(on);
}

bool QWebSettings::watchDogEnabled()
{
    return WebCore::Settings::watchDogEnabled();
}

void QWebSettings::setWatchDogTimeoutLimit(double second)
{
    WebCore::Settings::setWatchDogTimeoutLimit(second);
}

double QWebSettings::watchDogTimeoutLimit()
{
    return WebCore::Settings::watchDogTimeoutLimit();
}

void QWebSettings::setPopupMenuSupport(bool on)
{
	d->settings->setPopupMenuSupport(on);
}

bool QWebSettings::popupMenuSupport()
{
	return d->settings->popupMenuSupport();
}

#if USE(V8)
void QWebSettings::setV8Flags(WebKit::V8Feature feature, bool value) 
{ 
    String str;
    if (value == true)
    {
        str = String("--");
    }
    else
    {
        str = String("--no");
    }
    switch(feature) { 
        case WebKit::use_strict:
            str = str + String("use_strict");
            break;
        case WebKit::es5_readonly:
            str = str + String("es5_readonly");   
            break;
        case WebKit::es52_globals:
            str = str + String("es52_globals");   
            break;
        case WebKit::harmony_typeof:
            str = str + String("harmony_typeof");   
            break;
        case WebKit::harmony_scoping:
            str = str + String("harmony_scoping");    
            break;
        case WebKit::harmony_modules:
            str = str + String("harmony_modules");   
            break;
        case WebKit::harmony_proxies:
            str = str + String("harmony_proxies");    
            break;
        case WebKit::harmony_collections:
            str = str + String("harmony_collections");    
            break;
        case WebKit::harmony_observation:
            str = str + String("harmony_observation");  
            break;
        case WebKit::harmony:
            str = str + String("harmony");  
            break;
        case WebKit::packed_arrays:
            str = str + String("packed_arrays");    
            break;
        case WebKit::smi_only_arrays:
            str = str + String("smi_only_arrays");   
            break;
        case WebKit::clever_optimizations:
            str = str + String("clever_optimizations");  
            break;
        case WebKit::unbox_double_arrays:
            str = str + String("unbox_double_arrays");   
            break;
        case WebKit::string_slices:
            str = str + String("string_slices");   
            break;
        case WebKit::crankshaft:
            str = str + String("crankshaft");  
            break;
        case WebKit::use_range:
            str = str + String("use_range");   
            break;
        case WebKit::eliminate_dead_phis:
            str = str + String("eliminate_dead_phis");   
            break;
        case WebKit::use_gvn:
            str = str + String("use_gvn");   
            break;
        case WebKit::use_canonicalizing:
            str = str + String("use_canonicalizing");   
            break;
        case WebKit::use_inlining:
            str = str + String("use_inlining");  
            break;
        case WebKit::loop_invariant_code_motion:
            str = str + String("loop_invariant_code_motion");    
            break;
        case WebKit::collect_megamorphic_maps_from_stub_cache:
            str = str + String("collect_megamorphic_maps_from_stub_cache");   
            break;
        case WebKit::hydrogen_stats:
            str = str + String("hydrogen_stats");  
            break;
        case WebKit::trace_hydrogen:
            str = str + String("trace_hydrogen");   
            break;
        case WebKit::trace_inlining:
            str = str + String("trace_inlining");    
            break;
        case WebKit::trace_alloc:
            str = str + String("trace_alloc");    
            break;
        case WebKit::trace_all_uses:
            str = str + String("trace_all_uses");    
            break;
        case WebKit::trace_range:
            str = str + String("trace_range");   
            break;
        case WebKit::trace_gvn:
            str = str + String("trace_gvn");   
            break;
        case WebKit::trace_representation:
            str = str + String("trace_representation");    
            break;
        case WebKit::stress_pointer_maps:
            str = str + String("stress_pointer_maps");   
            break;
        case WebKit::stress_environments:
            str = str + String("stress_environments");   
            break;
        case WebKit::trap_on_deopt:
            str = str + String("trap_on_deopt");    
            break;
        case WebKit::deoptimize_uncommon_cases:
            str = str + String("deoptimize_uncommon_cases");    
            break;
        case WebKit::polymorphic_inlining: 
            str = str + String("polymorphic_inlining");   
            break;
        case WebKit::use_osr:
            str = str + String("use_osr");   
            break;
        case WebKit::array_bounds_checks_elimination:
            str = str + String("array_bounds_checks_elimination");   
            break;
        case WebKit::array_index_dehoisting:
            str = str + String("array_index_dehoisting");    
            break;
        case WebKit::dead_code_elimination:
            str = str + String("dead_code_elimination");    
            break;
        case WebKit::trace_dead_code_elimination:
            str = str + String("trace_dead_code_elimination");   
            break;
        case WebKit::trace_osr:
            str = str + String("trace_osr");    
            break;
        case WebKit::optimize_closures:
            str = str + String("optimize_closures");    
            break;
        case WebKit::lookup_sample_by_shared:
            str = str + String("lookup_sample_by_shared");   
            break;
        case WebKit::cache_optimized_code:
            str = str + String("cache_optimized_code");   
            break;
        case WebKit::inline_construct:
            str = str + String("inline_construct");    
            break;
        case WebKit::inline_arguments:
            str = str + String("inline_arguments");    
            break;
        case WebKit::inline_accessors:
            str = str + String("inline_accessors");    
            break;
        case WebKit::optimize_for_in:
            str = str + String("optimize_for_in");   
            break;
        case WebKit::opt_safe_uint32_operations:
            str = str + String("opt_safe_uint32_operations");   
            break;
        case WebKit::parallel_recompilation:
            str = str + String("parallel_recompilation");   
            break;
        case WebKit::trace_parallel_recompilation:
            str = str + String("trace_parallel_recompilation");   
            break;
        case WebKit::experimental_profiler:
            str = str + String("experimental_profiler");    
            break;
        case WebKit::watch_ic_patching:
            str = str + String("watch_ic_patching");    
            break;
        case WebKit::self_optimization:
            str = str + String("self_optimization");   
            break;
        case WebKit::direct_self_opt:
            str = str + String("direct_self_opt");    
            break;
        case WebKit::retry_self_opt:
            str = str + String("retry_self_opt");    
            break;
        case WebKit::interrupt_at_exit:
            str = str + String("interrupt_at_exit");   
            break;
        case WebKit::weighted_back_edges:
            str = str + String("weighted_back_edges");    
            break;
        case WebKit::trace_opt_verbose:
            str = str + String("trace_opt_verbose");    
            break;
        case WebKit::debug_code:
            str = str + String("debug_code");   
            break;
        case WebKit::code_comments:
            str = str + String("code_comments");    
            break;
        case WebKit::enable_sse2:
            str = str + String("enable_sse2");   
            break;
        case WebKit::enable_sse3:
            str = str + String("enable_sse3");   
            break;
        case WebKit::enable_sse4_1:
            str = str + String("enable_sse4_1");   
            break;
        case WebKit::enable_cmov:
            str = str + String("enable_cmov");    
            break;
        case WebKit::enable_rdtsc:
            str = str + String("enable_rdtsc");   
            break;
        case WebKit::enable_sahf:
            str = str + String("enable_sahf");   
            break;
        case WebKit::enable_vfp3:
            str = str + String("enable_vfp3");    
            break;
        case WebKit::enable_vfp2:
            str = str + String("enable_vfp2");    
            break;
        case WebKit::enable_armv7:
            str = str + String("enable_armv7");    
            break;
        case WebKit::enable_sudiv:
            str = str + String("enable_sudiv");    
            break;
        case WebKit::enable_movw_movt:
            str = str + String("enable_movw_movt");    
            break;
        case WebKit::enable_unaligned_accesses:
            str = str + String("enable_unaligned_accesses");    
            break;
        case WebKit::enable_fpu:
            str = str + String("enable_fpu");  
            break;
        case WebKit::expose_gc:
            str = str + String("expose_gc");    
            break;
        case WebKit::expose_externalize_string:
            str = str + String("expose_externalize_string");    
            break;
        case WebKit::builtins_in_stack_traces:
            str = str + String("builtins_in_stack_traces");    
            break;
        case WebKit::disable_native_files:
            str = str + String("disable_native_files");    
            break;
        case WebKit::inline_new:
            str = str + String("inline_new");   
            break;
        case WebKit::stack_trace_on_abort:
            str = str + String("stack_trace_on_abort");    
            break;
        case WebKit::trace:
            str = str + String("trace");  
            break;
        case WebKit::mask_constants_with_cookie:
            str = str + String("mask_constants_with_cookie");  
            break;
        case WebKit::lazy:
            str = str + String("lazy");  
            break;
        case WebKit::trace_opt:
            str = str + String("trace_opt");  
            break;
        case WebKit::trace_opt_stats:
            str = str + String("trace_opt_stats");   
            break;
        case WebKit::opt:
            str = str + String("opt");    
            break;
        case WebKit::always_opt:
            str = str + String("always_opt");    
            break;
        case WebKit::prepare_always_opt:
            str = str + String("prepare_always_opt");   
            break;
        case WebKit::trace_deopt:
            str = str + String("trace_deopt");    
            break;
        case WebKit::always_full_compiler:
            str = str + String("always_full_compiler");   
            break;
        case WebKit::compilation_cache:
            str = str + String("compilation_cache");    
            break;
        case WebKit::cache_prototype_transitions:
            str = str + String("cache_prototype_transitions");    
            break;
        case WebKit::trace_debug_json:
            str = str + String("trace_debug_json");    
            break;
        case WebKit::debugger_auto_break:
            str = str + String("debugger_auto_break");    
            break;
        case WebKit::enable_liveedit:
            str = str + String("enable_liveedit");   
            break;
        case WebKit::break_on_abort:
            str = str + String("break_on_abort");    
            break;
        case WebKit::always_inline_smi_code:
            str = str + String("always_inline_smi_code");    
            break;
        case WebKit::gc_global:
            str = str + String("gc_global");    
            break;
        case WebKit::trace_gc:
            str = str + String("trace_gc");   
            break;
        case WebKit::trace_gc_nvp:
            str = str + String("trace_gc_nvp");   
            break;
        case WebKit::trace_gc_ignore_scavenger:
            str = str + String("trace_gc_ignore_scavenger");    
            break;
        case WebKit::print_cumulative_gc_stat:
            str = str + String("print_cumulative_gc_stat");    
            break;
        case WebKit::trace_gc_verbose:
            str = str + String("trace_gc_verbose");    
            break;
        case WebKit::trace_fragmentation:
            str = str + String("trace_fragmentation");    
            break;
        case WebKit::trace_external_memory:
            str = str + String("trace_external_memory");   
            break;
        case WebKit::collect_maps:
            str = str + String("collect_maps");    
            break;
        case WebKit::flush_code:
            str = str + String("flush_code");    
            break;
        case WebKit::flush_code_incrementally:
            str = str + String("flush_code_incrementally");    
            break;
        case WebKit::age_code:
            str = str + String("age_code");  
            break;
        case WebKit::incremental_marking:
            str = str + String("incremental_marking");   
            break;
        case WebKit::incremental_marking_steps:
            str = str + String("incremental_marking_steps");   
            break;
        case WebKit::trace_incremental_marking:
            str = str + String("trace_incremental_marking");    
            break;
        case WebKit::track_gc_object_stats:
            str = str + String("track_gc_object_stats");    
            break;

#ifdef VERIFY_HEAP
        case WebKit::verify_heap:
            str = str + String("verify_heap");    
            break;
#endif

        case WebKit::use_idle_notification:
            str = str + String("use_idle_notification");   
            break;
        case WebKit::use_ic:
            str = str + String("use_ic");    
            break;
        case WebKit::native_code_counters:
            str = str + String("native_code_counters");    
            break;
        case WebKit::always_compact:
            str = str + String("always_compact");   
            break;
        case WebKit::lazy_sweeping:
            str = str + String("lazy_sweeping");    
            break;
        case WebKit::never_compact:
            str = str + String("never_compact");    
            break;
        case WebKit::compact_code_space:
            str = str + String("compact_code_space");    
            break;
        case WebKit::incremental_code_compaction:
            str = str + String("incremental_code_compaction");    
            break;
        case WebKit::cleanup_code_caches_at_gc:
            str = str + String("cleanup_code_caches_at_gc");    
            break;
        case WebKit::use_marking_progress_bar:
            str = str + String("use_marking_progress_bar");    
            break;
        case WebKit::use_verbose_printer:
            str = str + String("use_verbose_printer");   
            break;
        case WebKit::allow_natives_syntax:
            str = str + String("allow_natives_syntax");    
            break;
        case WebKit::trace_parse:
            str = str + String("trace_parse");   
            break;
        case WebKit::trace_sim:
            str = str + String("trace_sim");    
            break;
        case WebKit::check_icache:
            str = str + String("check_icache");    
            break;
        case WebKit::trace_exception:
            str = str + String("trace_exception");    
            break;
        case WebKit::preallocate_message_memory:
            str = str + String("preallocate_message_memory");    
            break;
        case WebKit::randomize_hashes:
            str = str + String("randomize_hashes");  
            break;
        case WebKit::preemption:
            str = str + String("preemption");
            break;
        case WebKit::regexp_optimization:
            str = str + String("regexp_optimization");    
            break;
        case WebKit::help:
            str = str + String("help"); 
            break;
        case WebKit::dump_counters:
            str = str + String("dump_counters");    
            break;

#ifdef ENABLE_DEBUGGER_SUPPORT
        case WebKit::debugger: 
            str = str + String("debugger");    
            break;
        case WebKit::remote_debugger:
            str = str + String("remote_debugger");    
            break;
        case WebKit::debugger_agent: 
            str = str + String("debugger_agent");    
            break;
#endif

        case WebKit::debug_compile_events:
            str = str + String("debug_compile_events");    
            break;
        case WebKit::debug_script_collected_events:
            str = str + String("debug_script_collected_events");    
            break;
        case WebKit::gdbjit:
            str = str + String("gdbjit");   
            break;
        case WebKit::gdbjit_full:
            str = str + String("gdbjit_full");    
            break;
        case WebKit::gdbjit_dump:
            str = str + String("gdbjit_dump");   
            break;
        case WebKit::force_marking_deque_overflows:
            str = str + String("force_marking_deque_overflows");    
            break;
        case WebKit::stress_compaction:
            str = str + String("stress_compaction");    
            break;
        case WebKit::log:
            str = str + String("log");    
            break;
        case WebKit::log_all:
            str = str + String("log_all");    
            break;
        case WebKit::log_runtime:
            str = str + String("log_runtime");    
            break;
        case WebKit::log_api:
            str = str + String("log_api");    
            break;
        case WebKit::log_code:
            str = str + String("log_code");    
            break;
        case WebKit::log_gc:
            str = str + String("log_gc");    
            break;
        case WebKit::log_handles:
            str = str + String("log_handles");    
            break;
        case WebKit::log_snapshot_positions:
            str = str + String("log_snapshot_positions");    
            break;
        case WebKit::log_suspect:
            str = str + String("log_suspect");    
            break;
        case WebKit::prof:
            str = str + String("prof");    
            break;
        case WebKit::prof_auto:
            str = str + String("prof_auto");    
            break;
        case WebKit::prof_lazy:
            str = str + String("prof_lazy");    
            break;
        case WebKit::prof_browser_mode:
            str = str + String("prof_browser_mode");    
            break;
        case WebKit::log_regexp:
            str = str + String("log_regexp");   
            break;
        case WebKit::ll_prof:
            str = str + String("ll_prof");   
            break;
        case WebKit::log_timer_events:
            str = str + String("log_timer_events");    
            break;
        //v8-3.17.15 add
        case WebKit::harmony_symbols:
            str = str + String("harmony_symbols");
            break;
        case WebKit::compiled_transitions:
            str = str + String("compiled_transitions");
            break;
        case WebKit::compiled_keyed_stores:
            str = str + String("compiled_keyed_stores");
            break;
        case WebKit::pretenure_literals:
            str = str + String("pretenure_literals");
            break;
        case WebKit::fast_math:
            str = str + String("fast_math");
            break;
        case WebKit::trace_track_allocation_sites:
            str = str + String("trace_track_allocation_sites");
            break;
        case WebKit::idefs:
            str = str + String("idefs");
            break;
        case WebKit::fold_constants:
            str = str + String("fold_constants");
            break;
        case WebKit::unreachable_code_elimination:
            str = str + String("unreachable_code_elimination");
            break;
        case WebKit::track_allocation_sites:
            str = str + String("track_allocation_sites");
            break;
        case WebKit::optimize_constructed_arrays:
            str = str + String("optimize_constructed_arrays");
            break;
        case WebKit::omit_prototype_checks_for_leaf_maps:
            str = str + String("omit_prototype_checks_for_leaf_maps");
            break;
        case WebKit::enable_32dregs:
            str = str + String("enable_32dregs");
            break;
        case WebKit::enable_vldr_imm:
            str = str + String("enable_vldr_imm");
            break;
        case WebKit::trace_stub_failures:
            str = str + String("trace_stub_failures");
            break;
        case WebKit::trace_js_array_abuse:
            str = str + String("trace_js_array_abuse");
            break;
        case WebKit::trace_external_array_abuse:
            str = str + String("trace_external_array_abuse");
            break;
        case WebKit::trace_array_abuse:
            str = str + String("trace_array_abuse");
            break;
        case WebKit::weak_embedded_maps_in_optimized_code:
            str = str + String("weak_embedded_maps_in_optimized_code");
            break;
        case WebKit::parallel_sweeping:
            str = str + String("parallel_sweeping");
            break;
        case WebKit::concurrent_sweeping:
            str = str + String("concurrent_sweeping");
            break;
        case WebKit::parallel_marking:
            str = str + String("parallel_marking");
            break;
        case WebKit::log_internal_timer_events:
            str = str + String("log_internal_timer_events");
            break;
        //v8-3.17.15 add end

        case WebKit::harmony_promises:
            str = str + String("harmony_promises");
         break;

        default:
            return;
    } 
    WebCore::ScriptController::setFlags(str.latin1().data(), str.length());
}

void QWebSettings::setV8Flags(WebKit::V8Feature feature, int value) 
{ 
    String str;
    switch(feature) { 
        case WebKit::max_inlined_source_size:
            str = "--max_inlined_source_size " + String::number(value);
            break;
        case WebKit::max_inlined_nodes:
            str = "--max_inlined_nodes " + String::number(value);
            break;
        case WebKit::max_inlined_nodes_cumulative:
            str = "--max_inlined_nodes_cumulative " + String::number(value);
            break;
        case WebKit::deopt_every_n_times:
            str = "--deopt_every_n_times " + String::number(value);
            break;
        case WebKit::stress_runs:
            str = "--stress_runs " + String::number(value);
            break;
        case WebKit::loop_weight:
            str = "--loop_weight " + String::number(value);
            break;
        case WebKit::parallel_recompilation_queue_length:
            str = "--parallel_recompilation_queue_length " + String::number(value);
            break;
        case WebKit::frame_count:
            str = "--frame_count " + String::number(value);
            break;
        case WebKit::interrupt_budget:
            str = "--interrupt_budget " + String::number(value);
            break;
        case WebKit::type_info_threshold:
            str = "--type_info_threshold " + String::number(value);
            break;
        case WebKit::self_opt_count:
            str = "--self_opt_count " + String::number(value);       
            break;
        case WebKit::stack_trace_limit:
            str = "--stack_trace_limit " + String::number(value);      
            break;
        case WebKit::min_preparse_length:
            str = "--min_preparse_length " + String::number(value);       
            break;
        case WebKit::max_opt_count:
            str = "--max_opt_count " + String::number(value);      
            break;
        case WebKit::stack_size:
            str = "--stack_size  " + String::number(value);      
            break;
        case WebKit::max_stack_trace_source_length:
            str = "--max_stack_trace_source_length " + String::number(value);     
            break;
        case WebKit::max_new_space_size:
            str = "--max_new_space_size " + String::number(value);      
            break;
        case WebKit::max_old_space_size:
            str = "--max_old_space_size " + String::number(value);      
            break;
        case WebKit::max_executable_size:
            str = "--max_executable_size " + String::number(value);       
            break;
        case WebKit::gc_interval:
            str = "--gc_interval " + String::number(value);      
            break;
        case WebKit::random_seed:
           str = "--random_seed " + String::number(value);       
            break;
        case WebKit::stop_sim_at:
            str = "--stop_sim_at " + String::number(value);       
            break;
        case WebKit::sim_stack_alignment:
            str = "--sim_stack_alignment " + String::number(value);       
            break;
        case WebKit::hash_seed:
            str = "--hash_seed " + String::number(value);       
            break;

#ifdef ENABLE_DEBUGGER_SUPPORT
        case WebKit::debugger_port:
            str = "--debugger_port " + String::number(value);     
            break;
#endif
    //v8-3.17.15 add 
        case WebKit::parallel_recompilation_delay:
             str = "--parallel_recompilation_delay " + String::number(value);      
            break;
        case WebKit::sweeper_threads:
            str = "--sweeper_threads " + String::number(value);      
            break;
        case WebKit::marking_threads:
            str = "--marking_threads " + String::number(value);      
            break;
    //v8-3.17.15 add end
        default:
            return;
    } 
    WebCore::ScriptController::setFlags(str.latin1().data(), str.length());
} 

void QWebSettings::setV8Flags(WebKit::V8Feature feature, const char* value) 
{ 
    UChar separator = ' ';
    Vector<String> result;
    String vatostr = String(value);
    vatostr.split(separator, result);
    String str;
    String strValue;
    if(result.size() == 0)
    {
        strValue = "";
    }
    else
    {
        strValue = result[0];
    }
    switch(feature) { 
        case WebKit::hydrogen_filter:
            str = "--hydrogen_filter " + strValue;       
            break;
        case WebKit::trace_phase:
            str = "--trace_phase " + strValue;    
            break;
        case WebKit::expose_natives_as:
            str = "--expose_natives_as " + strValue;      
            break;
        case WebKit::expose_debug_as:
            str = "--expose_debug_as " + strValue;     
            break;
        case WebKit::extra_code:
            str = "--extra_code " + strValue;   
            break;
        case WebKit::map_counters:
            str = "--map_counters " + strValue; 
            break;
        case WebKit::gdbjit_dump_filter:
            str = "--gdbjit_dump_filter " + strValue;     
            break;
        case WebKit::logfile:
            str = "--logfile " + strValue;       
            break;
        case WebKit::gc_fake_mmap:
            str = "--gc_fake_mmap " + strValue;  
            break;
        default:
            return;
    } 
    WebCore::ScriptController::setFlags(str.latin1().data(), str.length());
}
#endif

/*!
    \fn QWebSettingsPrivate* QWebSettings::handle() const
    \internal
*/
