/*
 * Copyright (C) 2006, 2007, 2008, 2009, 2011, 2012 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "Settings.h"

#include "AudioSession.h"
#include "BackForwardController.h"
#include "CachedResourceLoader.h"
#include "CookieStorage.h"
#include "DOMTimer.h"
#include "Database.h"
#include "Document.h"
#include "Font.h"
#include "FontGenericFamilies.h"
#include "FrameTree.h"
#include "FrameView.h"
#include "HTMLMediaElement.h"
#include "HistoryItem.h"
#include "InspectorInstrumentation.h"
#include "MainFrame.h"
#include "Page.h"
#include "PageCache.h"
#include "StorageMap.h"
#include "TextAutosizer.h"
#include <limits>
#include <wtf/NeverDestroyed.h>
#include <wtf/StdLibExtras.h>
/**/
#include "StorageTracker.h"
//add for application cache
#include "ApplicationCacheStorage.h"

//add for cookie path
#include "CookieJar.h"

#if OS(WINCE) || PLATFORM(INSPIRIUM)
#include "NetworkCacheManager.h"
#endif

//add for web socket
#include "Modules/websockets/WebSocket.h"

//add for watchdog
#if USE(JSC)
#include "JSDOMWindowBase.h"
#endif

#if USE(SKIA)
#include "SkGraphics.h"
#endif
/**/

namespace WebCore {

static void setImageLoadingSettings(Page* page)
{
    for (Frame* frame = &page->mainFrame(); frame; frame = frame->tree().traverseNext()) {
		if(!frame->document())
			continue;
        frame->document()->cachedResourceLoader()->setImagesEnabled(page->settings().areImagesEnabled());
        frame->document()->cachedResourceLoader()->setAutoLoadImages(page->settings().loadsImagesAutomatically());
    }
}

static void invalidateAfterGenericFamilyChange(Page* page)
{
    invalidateFontGlyphsCache();
    if (page)
        page->setNeedsRecalcStyleInAllFrames();
}

double Settings::gDefaultMinDOMTimerInterval = 0.010; // 10 milliseconds
double Settings::gDefaultDOMTimerAlignmentInterval = 0;
double Settings::gHiddenPageDOMTimerAlignmentInterval = 1.0;

#if USE(SAFARI_THEME)
bool Settings::gShouldPaintNativeControls = true;
#endif

#if USE(AVFOUNDATION)
bool Settings::gAVFoundationEnabled = false;
#endif

#if PLATFORM(MAC)
bool Settings::gQTKitEnabled = true;
#endif

#if ENABLE(PLUGIN_PROXY_FOR_VIDEO)
bool Settings::gVideoPluginProxyEnabled = true;
#endif

bool Settings::gMockScrollbarsEnabled = false;
bool Settings::gUsesOverlayScrollbars = false;

/**/
ProxyType Settings::m_proxyType = HTTP;
String Settings::m_proxyHost = "";
int Settings::m_proxyPort = 0;
String Settings::m_proxyIgnoreHosts = "";
bool Settings::m_useProxy = false;

String Settings::m_proxyUserName = "";
String Settings::m_proxyPassword = "";
/**/

/*  ICU */
#if OS(WINCE) //&& USE(ICU_UNICODE)
String Settings::m_icuDataPath = "";
#endif

#if PLATFORM(WIN) || PLATFORM(INSPIRIUM)
bool Settings::gShouldUseHighResolutionTimers = true;
#endif
    
/**/
String	Settings::m_acceptLanguage = "";
bool	Settings::m_useNetworkLog = false;
String*	Settings::m_networkLogPtr = NULL;

NetworkLogCallbackFunction Settings::m_networkLogCallback = NULL;

String	Settings::m_certificatePath = "";
bool	Settings::m_ignoreSSLErrors = false;

#if ENABLE(FJIB_PDF)
unsigned int Settings::m_pdfPasswordRetryCount = 3;
String	Settings::m_pdfToolBarHTMLSource = "";
unsigned int Settings::m_pdfToolBarHeight = 20;
float Settings::m_pdfZoomInMax = 3.0;
float Settings::m_pdfZoomOutMin = 0.2;
float Settings::m_pdfZoomStep = 0.2;
#endif
/**/
    
#if USE(JSC)
bool Settings::gShouldRespectPriorityInCSSAttributeSetters = false;
#endif
bool Settings::gLowPowerVideoAudioBufferSizeEnabled = false;
/**/
//add for clear referer
bool Settings::gClearReferer = false;
/**/

#if PLATFORM(IOS)
bool Settings::gNetworkDataUsageTrackingEnabled = false;
bool Settings::gAVKitEnabled = false;
#endif

// NOTEs
//  1) EditingMacBehavior comprises Tiger, Leopard, SnowLeopard and iOS builds, as well as QtWebKit when built on Mac;
//  2) EditingWindowsBehavior comprises Win32 and WinCE builds, as well as QtWebKit and Chromium when built on Windows;
//  3) EditingUnixBehavior comprises all unix-based systems, but Darwin/MacOS (and then abusing the terminology);
// 99) MacEditingBehavior is used as a fallback.
static EditingBehaviorType editingBehaviorTypeForPlatform()
{
    return
#if OS(DARWIN)
    EditingMacBehavior
#elif OS(WINDOWS)
    EditingWindowsBehavior
#elif OS(UNIX)
    EditingUnixBehavior
#else
    // Fallback
    EditingMacBehavior
#endif
    ;
}

#if PLATFORM(IOS)
static const bool defaultFixedPositionCreatesStackingContext = true;
static const bool defaultAcceleratedCompositingForFixedPositionEnabled = true;
static const bool defaultMediaPlaybackAllowsInline = false;
static const bool defaultMediaPlaybackRequiresUserGesture = true;
static const bool defaultShouldRespectImageOrientation = true;
#else
static const bool defaultFixedPositionCreatesStackingContext = false;
static const bool defaultAcceleratedCompositingForFixedPositionEnabled = false;
static const bool defaultMediaPlaybackAllowsInline = true;
static const bool defaultMediaPlaybackRequiresUserGesture = false;
static const bool defaultShouldRespectImageOrientation = false;
#endif

static const double defaultIncrementalRenderingSuppressionTimeoutInSeconds = 5;
#if USE(UNIFIED_TEXT_CHECKING)
static const bool defaultUnifiedTextCheckerEnabled = true;
#else
static const bool defaultUnifiedTextCheckerEnabled = false;
#endif
static const bool defaultSmartInsertDeleteEnabled = true;
static const bool defaultSelectTrailingWhitespaceEnabled = false;

// This amount of time must have elapsed before we will even consider scheduling a layout without a delay.
// FIXME: For faster machines this value can really be lowered to 200. 250 is adequate, but a little high
// for dual G5s. :)
static const auto layoutScheduleThreshold = std::chrono::milliseconds(250);

Settings::Settings(Page* page)
    : m_page(0)
    , m_mediaTypeOverride("screen")
    , m_fontGenericFamilies(std::make_unique<FontGenericFamilies>())
    , m_storageBlockingPolicy(SecurityOrigin::AllowAllStorage)
    , m_layoutInterval(layoutScheduleThreshold)
#if PLATFORM(IOS)
    , m_maxParseDuration(-1)
#endif
#if ENABLE(TEXT_AUTOSIZING)
    , m_textAutosizingFontScaleFactor(1)
#if HACK_FORCE_TEXT_AUTOSIZING_ON_DESKTOP
    , m_textAutosizingWindowSizeOverride(320, 480)
    , m_textAutosizingEnabled(true)
#else
    , m_textAutosizingEnabled(false)
#endif
#endif
    SETTINGS_INITIALIZER_LIST
    , m_screenFontSubstitutionEnabled(shouldEnableScreenFontSubstitutionByDefault())
    , m_isJavaEnabled(false)
    , m_isJavaEnabledForLocalFiles(true)
    , m_loadsImagesAutomatically(false)
    , m_privateBrowsingEnabled(false)
    , m_areImagesEnabled(true)
    , m_arePluginsEnabled(false)
    , m_isScriptEnabled(false)
    , m_needsAdobeFrameReloadingQuirk(false)
    , m_usesPageCache(false)
    , m_fontRenderingMode(0)
#if PLATFORM(IOS)
    , m_standalone(false)
    , m_telephoneNumberParsingEnabled(false)
    , m_mediaDataLoadsAutomatically(false)
    , m_shouldTransformsAffectOverflow(true)
    , m_shouldDispatchJavaScriptWindowOnErrorEvents(false)
    , m_alwaysUseBaselineOfPrimaryFont(false)
    , m_alwaysUseAcceleratedOverflowScroll(false)
#endif
#if ENABLE(CSS_STICKY_POSITION)
    , m_cssStickyPositionEnabled(true)
#endif
    , m_showTiledScrollingIndicator(false)
    , m_tiledBackingStoreEnabled(false)
    , m_backgroundShouldExtendBeyondPage(false)
    , m_dnsPrefetchingEnabled(false)
#if ENABLE(TOUCH_EVENTS)
    , m_touchEventEmulationEnabled(false)
#endif
    , m_scrollingPerformanceLoggingEnabled(false)
    , m_aggressiveTileRetentionEnabled(false)
    , m_timeWithoutMouseMovementBeforeHidingControls(3)
    , m_setImageLoadingSettingsTimer(this, &Settings::imageLoadingSettingsTimerFired)
#if ENABLE(HIDDEN_PAGE_DOM_TIMER_THROTTLING)
    , m_hiddenPageDOMTimerThrottlingEnabled(false)
#endif
#if ENABLE(PAGE_VISIBILITY_API)
    , m_hiddenPageCSSAnimationSuspensionEnabled(false)
#endif
    , m_fontFallbackPrefersPictographs(false)
/**/
    , m_aboutBlankBackGroundColor(0xffffff)
    , m_enableInputFileDialog(true)
/**/
	, m_popupMenuSupport(true)
{
    // A Frame may not have been created yet, so we initialize the AtomicString
    // hash before trying to use it.
    AtomicString::init();
    initializeDefaultFontFamilies();
    m_page = page; // Page is not yet fully initialized wen constructing Settings, so keeping m_page null over initializeDefaultFontFamilies() call.
}

Settings::~Settings()
{
}

PassRefPtr<Settings> Settings::create(Page* page)
{
    return adoptRef(new Settings(page));
}

SETTINGS_SETTER_BODIES

void Settings::setHiddenPageDOMTimerAlignmentInterval(double hiddenPageDOMTimerAlignmentinterval)
{
    gHiddenPageDOMTimerAlignmentInterval = hiddenPageDOMTimerAlignmentinterval;
}

double Settings::hiddenPageDOMTimerAlignmentInterval()
{
    return gHiddenPageDOMTimerAlignmentInterval;
}

#if !PLATFORM(MAC)
bool Settings::shouldEnableScreenFontSubstitutionByDefault()
{
    return true;
}
#endif

#if !PLATFORM(MAC)
void Settings::initializeDefaultFontFamilies()
{
    // Other platforms can set up fonts from a client, but on Mac, we want it in WebCore to share code between WebKit1 and WebKit2.
}
#endif

const AtomicString& Settings::standardFontFamily(UScriptCode script) const
{
    return m_fontGenericFamilies->standardFontFamily(script);
}

void Settings::setStandardFontFamily(const AtomicString& family, UScriptCode script)
{
    bool changes = m_fontGenericFamilies->setStandardFontFamily(family, script);
/**/
    if (changes && m_page)
/**/
        invalidateAfterGenericFamilyChange(m_page);
}

const AtomicString& Settings::fixedFontFamily(UScriptCode script) const
{
    return m_fontGenericFamilies->fixedFontFamily(script);
}

void Settings::setFixedFontFamily(const AtomicString& family, UScriptCode script)
{
    bool changes = m_fontGenericFamilies->setFixedFontFamily(family, script);
/**/
    if (changes && m_page)
/**/
        invalidateAfterGenericFamilyChange(m_page);
}

const AtomicString& Settings::serifFontFamily(UScriptCode script) const
{
    return m_fontGenericFamilies->serifFontFamily(script);
}

void Settings::setSerifFontFamily(const AtomicString& family, UScriptCode script)
{
    bool changes = m_fontGenericFamilies->setSerifFontFamily(family, script);
/**/
    if (changes && m_page)
/**/
        invalidateAfterGenericFamilyChange(m_page);
}

const AtomicString& Settings::sansSerifFontFamily(UScriptCode script) const
{
    return m_fontGenericFamilies->sansSerifFontFamily(script);
}

void Settings::setSansSerifFontFamily(const AtomicString& family, UScriptCode script)
{
    bool changes = m_fontGenericFamilies->setSansSerifFontFamily(family, script);
/**/
    if (changes && m_page)
/**/
        invalidateAfterGenericFamilyChange(m_page);
}

const AtomicString& Settings::cursiveFontFamily(UScriptCode script) const
{
    return m_fontGenericFamilies->cursiveFontFamily(script);
}

void Settings::setCursiveFontFamily(const AtomicString& family, UScriptCode script)
{
    bool changes = m_fontGenericFamilies->setCursiveFontFamily(family, script);
/**/
    if (changes && m_page)
/**/
        invalidateAfterGenericFamilyChange(m_page);
}

const AtomicString& Settings::fantasyFontFamily(UScriptCode script) const
{
    return m_fontGenericFamilies->fantasyFontFamily(script);
}

void Settings::setFantasyFontFamily(const AtomicString& family, UScriptCode script)
{
    bool changes = m_fontGenericFamilies->setFantasyFontFamily(family, script);
/**/
    if (changes && m_page)
/**/
        invalidateAfterGenericFamilyChange(m_page);
}

const AtomicString& Settings::pictographFontFamily(UScriptCode script) const
{
    return m_fontGenericFamilies->pictographFontFamily(script);
}

void Settings::setPictographFontFamily(const AtomicString& family, UScriptCode script)
{
    bool changes = m_fontGenericFamilies->setPictographFontFamily(family, script);
/**/
    if (changes && m_page)
/**/
        invalidateAfterGenericFamilyChange(m_page);
}

#if ENABLE(TEXT_AUTOSIZING)
void Settings::setTextAutosizingEnabled(bool textAutosizingEnabled)
{
    if (m_textAutosizingEnabled == textAutosizingEnabled)
        return;

    m_textAutosizingEnabled = textAutosizingEnabled;
/**/
    if(m_page)
/**/
    m_page->setNeedsRecalcStyleInAllFrames();
}

void Settings::setTextAutosizingWindowSizeOverride(const IntSize& textAutosizingWindowSizeOverride)
{
    if (m_textAutosizingWindowSizeOverride == textAutosizingWindowSizeOverride)
        return;

    m_textAutosizingWindowSizeOverride = textAutosizingWindowSizeOverride;
/**/
    if(m_page)
/**/
    m_page->setNeedsRecalcStyleInAllFrames();
}

void Settings::setTextAutosizingFontScaleFactor(float fontScaleFactor)
{
    m_textAutosizingFontScaleFactor = fontScaleFactor;
/**/
if(m_page == NULL)
    return;
/**/

    // FIXME: I wonder if this needs to traverse frames like in WebViewImpl::resize, or whether there is only one document per Settings instance?
    for (Frame* frame = m_page->mainFrame(); frame; frame = frame->tree().traverseNext()) {
        if (!frame->document())
            continue;
        frame->document()->textAutosizer()->recalculateMultipliers();
    }

    m_page->setNeedsRecalcStyleInAllFrames();
}

#endif

void Settings::setMediaTypeOverride(const String& mediaTypeOverride)
{
    if (m_mediaTypeOverride == mediaTypeOverride)
        return;

    m_mediaTypeOverride = mediaTypeOverride;

/**/
    if(m_page){
        FrameView* view = m_page->mainFrame().view();
        ASSERT(view);

        view->setMediaType(mediaTypeOverride);
        m_page->setNeedsRecalcStyleInAllFrames();
    }
/**/
}

void Settings::setLoadsImagesAutomatically(bool loadsImagesAutomatically)
{
    m_loadsImagesAutomatically = loadsImagesAutomatically;
    
    // Changing this setting to true might immediately start new loads for images that had previously had loading disabled.
    // If this happens while a WebView is being dealloc'ed, and we don't know the WebView is being dealloc'ed, these new loads
    // can cause crashes downstream when the WebView memory has actually been free'd.
    // One example where this can happen is in Mac apps that subclass WebView then do work in their overridden dealloc methods.
    // Starting these loads synchronously is not important.  By putting it on a 0-delay, properly closing the Page cancels them
    // before they have a chance to really start.
    // See http://webkit.org/b/60572 for more discussion.
    m_setImageLoadingSettingsTimer.startOneShot(0);
}

void Settings::imageLoadingSettingsTimerFired(Timer<Settings>*)
{
/**/
    if(m_page)
/**/
        setImageLoadingSettings(m_page);
}

void Settings::setScriptEnabled(bool isScriptEnabled)
{
#if PLATFORM(IOS)
    if (m_isScriptEnabled == isScriptEnabled)
        return;
#endif

    m_isScriptEnabled = isScriptEnabled;
/**/
    if(m_page)
        return;
/**/
#if PLATFORM(IOS)
    m_page->setNeedsRecalcStyleInAllFrames();
#endif
    InspectorInstrumentation::scriptsEnabled(m_page, m_isScriptEnabled);
}

void Settings::setJavaEnabled(bool isJavaEnabled)
{
    m_isJavaEnabled = isJavaEnabled;
}

void Settings::setJavaEnabledForLocalFiles(bool isJavaEnabledForLocalFiles)
{
    m_isJavaEnabledForLocalFiles = isJavaEnabledForLocalFiles;
}

void Settings::setImagesEnabled(bool areImagesEnabled)
{
    m_areImagesEnabled = areImagesEnabled;

    // See comment in setLoadsImagesAutomatically.
    m_setImageLoadingSettingsTimer.startOneShot(0);
}

void Settings::setPluginsEnabled(bool arePluginsEnabled)
{
    if (m_arePluginsEnabled == arePluginsEnabled)
        return;

    m_arePluginsEnabled = arePluginsEnabled;
    Page::refreshPlugins(false);
}

void Settings::setPrivateBrowsingEnabled(bool privateBrowsingEnabled)
{
    if (m_privateBrowsingEnabled == privateBrowsingEnabled)
        return;

    m_privateBrowsingEnabled = privateBrowsingEnabled;
/**/
    if(m_page)
/**/
        m_page->privateBrowsingStateChanged();
}

void Settings::setUserStyleSheetLocation(const URL& userStyleSheetLocation)
{
    if (m_userStyleSheetLocation == userStyleSheetLocation)
        return;

    m_userStyleSheetLocation = userStyleSheetLocation;

/**/
    if(m_page)
/**/
        m_page->userStyleSheetLocationChanged();
}

// FIXME: This quirk is needed because of Radar 4674537 and 5211271. We need to phase it out once Adobe
// can fix the bug from their end.
void Settings::setNeedsAdobeFrameReloadingQuirk(bool shouldNotReloadIFramesForUnchangedSRC)
{
    m_needsAdobeFrameReloadingQuirk = shouldNotReloadIFramesForUnchangedSRC;
}

void Settings::setDefaultMinDOMTimerInterval(double interval)
{
    gDefaultMinDOMTimerInterval = interval;
}

double Settings::defaultMinDOMTimerInterval()
{
    return gDefaultMinDOMTimerInterval;
}

void Settings::setMinDOMTimerInterval(double interval)
{
/**/
    if(m_page)
/**/
        m_page->setMinimumTimerInterval(interval);
}

double Settings::minDOMTimerInterval()
{
/**/
    return m_page? m_page->minimumTimerInterval() : Settings::defaultMinDOMTimerInterval();
/**/
}

void Settings::setDefaultDOMTimerAlignmentInterval(double interval)
{
    gDefaultDOMTimerAlignmentInterval = interval;
}

double Settings::defaultDOMTimerAlignmentInterval()
{
    return gDefaultDOMTimerAlignmentInterval;
}

void Settings::setDOMTimerAlignmentInterval(double interval)
{
/**/
    if(m_page)
/**/
        m_page->setTimerAlignmentInterval(interval);
}

double Settings::domTimerAlignmentInterval() const
{
/**/
    return m_page? m_page->timerAlignmentInterval() : 0;
/**/
}

void Settings::setLayoutInterval(std::chrono::milliseconds layoutInterval)
{
    // FIXME: It seems weird that this function may disregard the specified layout interval.
    // We should either expose layoutScheduleThreshold or better communicate this invariant.
    m_layoutInterval = std::max(layoutInterval, layoutScheduleThreshold);
}

void Settings::setUsesPageCache(bool usesPageCache)
{
/**/
    if(!m_page)
        return;
/**/
    if (m_usesPageCache == usesPageCache)
        return;
        
    m_usesPageCache = usesPageCache;
    if (!m_usesPageCache) {
        int first = -m_page->backForward().backCount();
        int last = m_page->backForward().forwardCount();
        for (int i = first; i <= last; i++)
            pageCache()->remove(m_page->backForward().itemAtIndex(i));
    }
}

void Settings::setScreenFontSubstitutionEnabled(bool enabled)
{
    if (m_screenFontSubstitutionEnabled == enabled)
        return;
    m_screenFontSubstitutionEnabled = enabled;
    m_page->setNeedsRecalcStyleInAllFrames();
}

void Settings::setFontRenderingMode(FontRenderingMode mode)
{
    if (fontRenderingMode() == mode)
        return;
    m_fontRenderingMode = mode;
/**/
    if(m_page)
/**/
        m_page->setNeedsRecalcStyleInAllFrames();
}

FontRenderingMode Settings::fontRenderingMode() const
{
    return static_cast<FontRenderingMode>(m_fontRenderingMode);
}

void Settings::setLocalStorageDatabasePath(const String& path)
{
    m_localStorageDatabasePath = path;
    StorageTracker::tracker().setDatabaseDirectoryPath(path);
}
#if USE(SAFARI_THEME)
void Settings::setShouldPaintNativeControls(bool shouldPaintNativeControls)
{
    gShouldPaintNativeControls = shouldPaintNativeControls;
}
#endif

void Settings::setDNSPrefetchingEnabled(bool dnsPrefetchingEnabled)
{
    if (m_dnsPrefetchingEnabled == dnsPrefetchingEnabled)
        return;

    m_dnsPrefetchingEnabled = dnsPrefetchingEnabled;
/**/
    if(m_page)
/**/
        m_page->dnsPrefetchingStateChanged();
}

void Settings::setShowTiledScrollingIndicator(bool enabled)
{
    if (m_showTiledScrollingIndicator == enabled)
        return;
        
    m_showTiledScrollingIndicator = enabled;
}

#if PLATFORM(WIN) || PLATFORM(INSPIRIUM)
//#if PLATFORM(WIN)
void Settings::setShouldUseHighResolutionTimers(bool shouldUseHighResolutionTimers)
{
    gShouldUseHighResolutionTimers = shouldUseHighResolutionTimers;
}

/**/
//add for cookie size
void Settings::setMaxCookieSize(unsigned size)
{    
    const Document* doc = NULL;
    if(m_page)
        doc = m_page->mainFrame().document();

    WebCore::setCookieSizeLimit(doc, size);
}

unsigned Settings::maxCookieSize()
{
    const Document* doc = NULL;
    if(m_page)
        doc = m_page->mainFrame().document();

    return WebCore::cookieSizeLimit(doc);
}

//add for cookie path
void Settings::setCookiePath(const String& path)
{
    const Document* doc = NULL;
    if(m_page)
        doc = m_page->mainFrame().document();

    WebCore::setCookieJarPath(doc, path);
}

String Settings::cookiePath()
{
    const Document* doc = NULL;
    if(m_page)
        doc = m_page->mainFrame().document();

    return WebCore::cookieJarPath(doc);
}

//add for application cache size
/****************************************************************************
*
* Description : set the max size of application cache with size. 
*
* Arguments : size		[IN]the max size of application cache.
*
* Return Values : NONE
*
****************************************************************************/
void Settings::setOfflineWebApplicationCacheMaxSize(int64_t size)
{
    WebCore::cacheStorage().setMaximumSize(size);
    int64_t overFlowSpace=0;
    if(!WebCore::cacheStorage().getOverFlowSpace(0,overFlowSpace))
    {
        return;
    }

    if(overFlowSpace>0)
    {
        WebCore::cacheStorage().screwDataBaseSize(overFlowSpace);
    }	
}

/****************************************************************************
*
* Description : Get the max size of application cache with size. 
*
* Arguments : NONE
*
* Return Values : the max size of application cache
*
****************************************************************************/
int64_t Settings::offlineWebApplicationCacheMaxSize()
{
    return WebCore::cacheStorage().maximumSize();
}

/****************************************************************************
*
* Description : Clear all off line application cache data. 
*
* Arguments : NONE
*
* Return Values : NONE
*
****************************************************************************/
void Settings::clearOfflineWebApplicationCache()
{
    WebCore::cacheStorage().deleteAllEntries();
}
//#endif /* PLATFORM(WIN) */

void Settings::setDiskCachePath(const String& path)
{
    NetworkCacheManager::getInstance()->setCacheDir(path);
}

const String& Settings::diskCachePath() const
{
    return NetworkCacheManager::getInstance()->cacheDir();
}

void Settings::setMaxDiskCacheSize(size_t size)
{
    NetworkCacheManager::getInstance()->setCacheSizeLimit(size);
}

size_t Settings::maxDiskCacheSize() const
{
    return NetworkCacheManager::getInstance()->cacheSizeLimit();
}

void Settings::setDiskCacheEnabled(bool enable)
{
    NetworkCacheManager::getInstance()->setCacheEnabled(enable);
}

bool Settings::isDiskCacheEnabled() const
{
    return NetworkCacheManager::getInstance()->cacheEnabled();
}

void Settings::setLocale(int localeCode)
{
    m_locale = localeCode;
#if USE(SKIA)
    SkGraphics::PurgeFontCache();
#endif
}

int Settings::locale()
{
    return m_locale;
}
#endif /* PLATFORM(WIN) || PLATFORM(INSPIRIUM) */
/**/

void Settings::setCertificatePath(const String& certificateDirectory)
{
    m_certificatePath = certificateDirectory;
}

const String& Settings::certificatePath()
{
    return m_certificatePath;
}

/**/
//add for application cache path
/****************************************************************************
*

* Description : set the path of application cache with cacheDirectory.
*
* Arguments : cacheDirectory		[IN]assigned application cache path.

*
* Return Values : NONE
*
****************************************************************************/
void Settings::setOfflineWebApplicationCachePath(const String& cacheDirectory)
{
    WebCore::cacheStorage().setCacheDirectory(cacheDirectory);
}

/****************************************************************************
*

* Description : get the path of application cache with cacheDirectory.
*
* Arguments : NONE

*
* Return Values : the application cache path
*
****************************************************************************/
const String& Settings::offlineWebApplicationCachePath()
{
    return WebCore::cacheStorage().cacheDirectory();
}

//add for application cache size
void Settings::setApplicationCacheMaxSize(int64_t size)
{
    WebCore::cacheStorage().deleteAllEntries();
    WebCore::cacheStorage().setMaximumSize(size);
}

int64_t Settings::applicationCacheMaxSize()
{
    return WebCore::cacheStorage().maximumSize();
}

void Settings::clearApplicationCache()
{
    WebCore::cacheStorage().deleteAllEntries();
}

/****************************************************************************
*
* Description : Delete Applicationcache Files.
*
* Arguments : NONE
*
* Return Values : NONE
*
****************************************************************************/
void Settings::deleteOfflineWebApplicationCacheFiles()
{
    WebCore::cacheStorage().deleteOfflineWebApplicationCacheFiles();
}
/**/

void Settings::setStorageBlockingPolicy(SecurityOrigin::StorageBlockingPolicy enabled)
{
    if (m_storageBlockingPolicy == enabled)
        return;

    m_storageBlockingPolicy = enabled;
/**/
    if(m_page)
/**/
        m_page->storageBlockingStateChanged();
}

void Settings::setTiledBackingStoreEnabled(bool enabled)
{
    m_tiledBackingStoreEnabled = enabled;
#if USE(TILED_BACKING_STORE)
/**/
    if (m_page)
/**/
        m_page->mainFrame().setTiledBackingStoreEnabled(enabled);
#endif
}

void Settings::setBackgroundShouldExtendBeyondPage(bool shouldExtend)
{
    if (m_backgroundShouldExtendBeyondPage == shouldExtend)
        return;

    m_backgroundShouldExtendBeyondPage = shouldExtend;
#if !OS(WINCE)
    m_page->mainFrame().view()->setBackgroundExtendsBeyondPage(shouldExtend);
#endif
}

#if USE(AVFOUNDATION)
void Settings::setAVFoundationEnabled(bool enabled)
{
    if (gAVFoundationEnabled == enabled)
        return;

    gAVFoundationEnabled = enabled;
    HTMLMediaElement::resetMediaEngines();
}
#endif

#if PLATFORM(MAC)
void Settings::setQTKitEnabled(bool enabled)
{
    if (gQTKitEnabled == enabled)
        return;

    gQTKitEnabled = enabled;
    HTMLMediaElement::resetMediaEngines();
}
#endif

#if ENABLE(PLUGIN_PROXY_FOR_VIDEO)
void Settings::setVideoPluginProxyEnabled(bool enabled)
{
    if (gVideoPluginProxyEnabled == enabled)
        return;

    gVideoPluginProxyEnabled = enabled;
    HTMLMediaElement::resetMediaEngines();
}
#endif

void Settings::setScrollingPerformanceLoggingEnabled(bool enabled)
{
    m_scrollingPerformanceLoggingEnabled = enabled;
/**/
    if (m_page && m_page->mainFrame().view())
/**/
        m_page->mainFrame().view()->setScrollingPerformanceLoggingEnabled(enabled);
}
    
void Settings::setAggressiveTileRetentionEnabled(bool enabled)
{
    m_aggressiveTileRetentionEnabled = enabled;
}

void Settings::setMockScrollbarsEnabled(bool flag)
{
    gMockScrollbarsEnabled = flag;
    // FIXME: This should update scroll bars in existing pages.
}

bool Settings::mockScrollbarsEnabled()
{
    return gMockScrollbarsEnabled;
}

void Settings::setUsesOverlayScrollbars(bool flag)
{
    gUsesOverlayScrollbars = flag;
    // FIXME: This should update scroll bars in existing pages.
}

bool Settings::usesOverlayScrollbars()
{
    return gUsesOverlayScrollbars;
}

#if USE(JSC)
void Settings::setShouldRespectPriorityInCSSAttributeSetters(bool flag)
{
    gShouldRespectPriorityInCSSAttributeSetters = flag;
}

bool Settings::shouldRespectPriorityInCSSAttributeSetters()
{
    return gShouldRespectPriorityInCSSAttributeSetters;
}
#endif

/**/
void Settings::setWebSocketEnabled(bool enabled)
{
#if ENABLE(WEB_SOCKETS)
    WebSocket::setIsAvailable(enabled);
#endif
}

bool Settings::webSocketEnabled()
{
#if ENABLE(WEB_SOCKETS)
    return WebSocket::isAvailable();
#else
    return false;
#endif
}

void Settings::setProxyType(ProxyType proxyType)
{
    m_proxyType = proxyType;
}

ProxyType Settings::proxyType()
{
    return m_proxyType;
}

void Settings::setProxyHost(String& proxyHost)
{
    m_proxyHost = proxyHost;
}

void Settings::setProxyPort(int proxyPort)
{
    m_proxyPort = proxyPort;
}

void Settings::setProxyIgnoreHosts(String& proxyIgnoreHosts)
{
    m_proxyIgnoreHosts = proxyIgnoreHosts;
}

String& Settings::proxyHost()
{
    return m_proxyHost;
}

int Settings::proxyPort()
{
    return  m_proxyPort;
}

String& Settings::proxyIgnoreHosts()
{
    return m_proxyIgnoreHosts;
}

void Settings::setUseProxy(bool useProxy)
{
    m_useProxy = useProxy;
}

bool Settings::useProxy()
{
    return m_useProxy;
}

void Settings::setProxyUserName(String& userName)
{
    m_proxyUserName = userName;
}

String& Settings::proxyUserName()
{
    return m_proxyUserName;
}

void Settings::setProxyPassword(String& password)
{
    m_proxyPassword = password;
}

String& Settings::proxyPassword()
{
    return m_proxyPassword;
}

void Settings::setWatchDogEnabled(bool on)
{
#if USE(JSC)
    if (JSDOMWindowBase::commonVM())
        JSDOMWindowBase::commonVM()->setWatchDogEnabled(on);
#endif
}

bool Settings::watchDogEnabled()
{
#if USE(JSC)
    if (JSDOMWindowBase::commonVM())
        return JSDOMWindowBase::commonVM()->watchDogEnabled();
    else
        return false;
#else
    return false;
#endif
} 

void Settings::setWatchDogTimeoutLimit(double second)
{
#if USE(JSC)
    if (JSDOMWindowBase::commonVM())
        JSDOMWindowBase::commonVM()->setWatchDogTimeoutLimit(second);
#endif
}

double Settings::watchDogTimeoutLimit()
{
#if USE(JSC)
    if (JSDOMWindowBase::commonVM())
        return JSDOMWindowBase::commonVM()->watchDogTimeoutLimit();
    else
        return std::numeric_limits<double>::infinity();
#else
    return std::numeric_limits<double>::infinity();
#endif
}

void Settings::setAcceptLanguage(String& acceptLanguage)
{
    m_acceptLanguage = acceptLanguage;
}

String& Settings::acceptLanguage()
{
    return m_acceptLanguage;
}

void Settings::setEnableNetworkLog(bool enableNetworkLog)
{
    m_useNetworkLog = enableNetworkLog;
}

void Settings::setAboutBlankBackGroundColor(unsigned char red, unsigned char green, unsigned char blue)
{
    m_aboutBlankBackGroundColor = (0xff000000 | red << 16 | green << 8 | blue);
}

unsigned int Settings::getAboutBlankBackGroundColor()
{
    return m_aboutBlankBackGroundColor & 0x00ffffff;
}

bool Settings::enableNetworkLog()
{
    return m_useNetworkLog;
}

void Settings::setNetworkLogBuffer(String* aBuffer)
{
    m_networkLogPtr = aBuffer;
}

String* Settings::networkLogBuffer()
{
    return m_networkLogPtr;
}

void Settings::setIgnoreSSLErrors(bool ignore)
{
    m_ignoreSSLErrors = ignore;
}

bool Settings::ignoreSSLErrors()
{
    return m_ignoreSSLErrors;
}

//add for clear referer
void Settings::setClearReferer(bool flag)
{
    gClearReferer = flag;
}

bool Settings::clearReferer()
{
    return gClearReferer;
}

void Settings::setUsePlatformNppValues(bool use)
{
    m_usePlatformNppValues = use;
}

bool Settings::usePlatformNppValues()
{
    return m_usePlatformNppValues;
}

void Settings::setPlatformNPPValues(Vector<NPPValue> values)
{
    m_nppValues = values;
}

Vector<NPPValue>& Settings::platformNPPValues()
{
    return m_nppValues;
}

void Settings::setNetworkLogCallbackFunction(NetworkLogCallbackFunction aFunction)
{
    m_networkLogCallback = aFunction;
}

NetworkLogCallbackFunction Settings::networkLogCallbackFunction()
{
    return m_networkLogCallback;
}
/**/

#if ENABLE(HIDDEN_PAGE_DOM_TIMER_THROTTLING)
void Settings::setHiddenPageDOMTimerThrottlingEnabled(bool flag)
{
    if (m_hiddenPageDOMTimerThrottlingEnabled == flag)
        return;
    m_hiddenPageDOMTimerThrottlingEnabled = flag;
/**/
    if(m_page)
/**/
        m_page->hiddenPageDOMTimerThrottlingStateChanged();
}
#endif

#if ENABLE(PAGE_VISIBILITY_API)
void Settings::setHiddenPageCSSAnimationSuspensionEnabled(bool flag)
{
    if (m_hiddenPageCSSAnimationSuspensionEnabled == flag)
        return;
    m_hiddenPageCSSAnimationSuspensionEnabled = flag;
/**/
    if(m_page)
/**/
        m_page->hiddenPageCSSAnimationSuspensionStateChanged();
}
#endif

void Settings::setFontFallbackPrefersPictographs(bool preferPictographs)
{
    if (m_fontFallbackPrefersPictographs == preferPictographs)
        return;

    m_fontFallbackPrefersPictographs = preferPictographs;
    m_page->setNeedsRecalcStyleInAllFrames();
}

void Settings::setLowPowerVideoAudioBufferSizeEnabled(bool flag)
{
    gLowPowerVideoAudioBufferSizeEnabled = flag;
}

/*  ICU */
#if OS(WINCE)
// #if USE(ICU_UNICODE)
void Settings::setICUDataPath(const String& path) {
    m_icuDataPath = path;

    String _path = path;
	Vector<UChar> temp = path.charactersWithNullTermination();
    const UChar* pathInternal = temp.data();

    int inLength = m_icuDataPath.length();
    int convertedLength = wcstombs(NULL, pathInternal, inLength);

    char* p = (char *)malloc(sizeof(char) * (convertedLength + 1));
    convertedLength = wcstombs(p, pathInternal, inLength);
    p[convertedLength] = '\0';

//    printf(" **** pData = %s\n", p);

    u_setDataDirectory(p);
    free(p);
}

const String& Settings::icuDataPath() {
    return m_icuDataPath;
}
//#endif /* USE(ICU_UNICODE) */
#endif /* OS(WINCE) */

#if PLATFORM(IOS)
void Settings::setStandalone(bool standalone)
{
    m_standalone = standalone;
}

void Settings::setAudioSessionCategoryOverride(unsigned sessionCategory)
{
    AudioSession::sharedSession().setCategoryOverride(static_cast<AudioSession::CategoryType>(sessionCategory));
}

unsigned Settings::audioSessionCategoryOverride()
{
    return AudioSession::sharedSession().categoryOverride();
}

void Settings::setNetworkDataUsageTrackingEnabled(bool trackingEnabled)
{
    gNetworkDataUsageTrackingEnabled = trackingEnabled;
}

bool Settings::networkDataUsageTrackingEnabled()
{
    return gNetworkDataUsageTrackingEnabled;
}

static String& sharedNetworkInterfaceNameGlobal()
{
    static NeverDestroyed<String> networkInterfaceName;
    return networkInterfaceName;
}

void Settings::setNetworkInterfaceName(const String& networkInterfaceName)
{
    sharedNetworkInterfaceNameGlobal() = networkInterfaceName;
}

const String& Settings::networkInterfaceName()
{
    return sharedNetworkInterfaceNameGlobal();
}
#endif

#if ENABLE(FJIB_PDF)
void Settings::setPDFPasswordRetryCount(unsigned int count)
{
    if (count == 0)
        return;
    m_pdfPasswordRetryCount = count;
}

unsigned int Settings::pdfPasswordRetryCount()
{
    return m_pdfPasswordRetryCount;
}

void Settings::setPDFToolBarHTMLSource(const String& html)
{
    m_pdfToolBarHTMLSource = html;
}

const String& Settings::pdfToolBarHTMLSource()
{
    return m_pdfToolBarHTMLSource;
}

void Settings::setPDFToolBarHeight(unsigned int height)
{
    if (height == 0)
        return;
    m_pdfToolBarHeight = height;
}

unsigned int Settings::pdfToolBarHeight()
{
    return m_pdfToolBarHeight;
}

void Settings::setPDFZoomInMax(float zoomInMax)
{
    if (zoomInMax < 1.0f)
        return;
    m_pdfZoomInMax = zoomInMax;
}

float Settings::pdfZoomInMax()
{
    return m_pdfZoomInMax;
}

void Settings::setPDFZoomOutMin(float zoomOutMin)
{
    if (zoomOutMin <= 0.0f || zoomOutMin > 1.0f)
        return;
    m_pdfZoomOutMin = zoomOutMin;
}

float Settings::pdfZoomOutMin()
{
    return m_pdfZoomOutMin;
}

void Settings::setPDFZoomStep(float zoomStep)
{
    if (zoomStep < 0.0f)
        return;
    m_pdfZoomStep = zoomStep;
}

float Settings::pdfZoomStep()
{
    return m_pdfZoomStep;
}
#endif

void Settings::setPopupMenuSupport(bool on)
{
	m_popupMenuSupport = on;
}

bool Settings::popupMenuSupport()
{
	return m_popupMenuSupport;
}

} // namespace WebCore
