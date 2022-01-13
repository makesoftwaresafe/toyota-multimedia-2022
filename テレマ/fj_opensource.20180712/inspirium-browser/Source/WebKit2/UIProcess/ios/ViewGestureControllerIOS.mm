/*
 * Copyright (C) 2013, 2014 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#import "config.h"
#import "ViewGestureController.h"

#import "WebPageGroup.h"
#import "ViewGestureControllerMessages.h"
#import "ViewGestureGeometryCollectorMessages.h"
#import "ViewSnapshotStore.h"
#import "WebBackForwardList.h"
#import "WebPageMessages.h"
#import "WebPageProxy.h"
#import "WebProcessProxy.h"
#import <QuartzCore/QuartzCore.h>
#import <UIKit/UIApplication_Private.h>
#import <UIKit/UIScreenEdgePanGestureRecognizer.h>
#import <UIKit/UIViewControllerTransitioning_Private.h>
#import <UIKit/UIWebTouchEventsGestureRecognizer.h>
#import <UIKit/_UINavigationInteractiveTransition.h>
#import <UIKit/_UINavigationParallaxTransition.h>

#if USE(IOSURFACE)
#import <IOSurface/IOSurface.h>
#import <IOSurface/IOSurfacePrivate.h>
#endif

using namespace WebCore;

@interface WKSwipeTransitionController : NSObject <_UINavigationInteractiveTransitionBaseDelegate>
- (instancetype)initWithViewGestureController:(WebKit::ViewGestureController*)gestureController swipingView:(UIView *)view;
@end

@implementation WKSwipeTransitionController
{
    WebKit::ViewGestureController *_gestureController;
    RetainPtr<_UINavigationInteractiveTransitionBase> _backTransitionController;
}

static const float swipeSnapshotRemovalRenderTreeSizeTargetFraction = 0.5;
static const std::chrono::seconds swipeSnapshotRemovalWatchdogDuration = 3_s;

- (instancetype)initWithViewGestureController:(WebKit::ViewGestureController*)gestureController swipingView:(UIView *)view
{
    self = [super init];
    if (self) {
        _gestureController = gestureController;

        _backTransitionController = adoptNS([_UINavigationInteractiveTransitionBase alloc]);
        _backTransitionController = [_backTransitionController initWithGestureRecognizerView:view animator:nil delegate:self];
    }
    return self;
}

- (WebKit::ViewGestureController::SwipeDirection)directionForTransition:(_UINavigationInteractiveTransitionBase *)transition
{
    return transition == _backTransitionController ? WebKit::ViewGestureController::SwipeDirection::Left : WebKit::ViewGestureController::SwipeDirection::Right;
}

- (void)startInteractiveTransition:(_UINavigationInteractiveTransitionBase *)transition
{
    _gestureController->beginSwipeGesture(transition, [self directionForTransition:transition]);
}

- (BOOL)shouldBeginInteractiveTransition:(_UINavigationInteractiveTransitionBase *)transition
{
    return _gestureController->canSwipeInDirection([self directionForTransition:transition]);
}

- (BOOL)interactiveTransition:(_UINavigationInteractiveTransitionBase *)transition gestureRecognizer:(UIGestureRecognizer *)gestureRecognizer shouldRecognizeSimultaneouslyWithGestureRecognizer:(UIGestureRecognizer *)otherGestureRecognizer
{
    return [otherGestureRecognizer isKindOfClass:[UITapGestureRecognizer class]];
}

- (BOOL)interactiveTransition:(_UINavigationInteractiveTransitionBase *)transition gestureRecognizer:(UIGestureRecognizer *)gestureRecognizer shouldReceiveTouch:(UITouch *)touch
{
    return YES;
}

- (UIPanGestureRecognizer *)gestureRecognizerForInteractiveTransition:(_UINavigationInteractiveTransitionBase *)transition WithTarget:(id)target action:(SEL)action
{
    UIScreenEdgePanGestureRecognizer *recognizer = [[UIScreenEdgePanGestureRecognizer alloc] initWithTarget:target action:action];
    switch ([self directionForTransition:transition]) {
    case WebKit::ViewGestureController::SwipeDirection::Left:
        [recognizer setEdges:UIMinXEdge];
        break;
    case WebKit::ViewGestureController::SwipeDirection::Right:
        [recognizer setEdges:UIMaxXEdge];
        break;
    }
    return recognizer;
}

@end

namespace WebKit {

ViewGestureController::ViewGestureController(WebPageProxy& webPageProxy)
    : m_webPageProxy(webPageProxy)
    , m_activeGestureType(ViewGestureType::None)
    , m_swipeWatchdogTimer(this, &ViewGestureController::swipeSnapshotWatchdogTimerFired)
    , m_targetRenderTreeSize(0)
{
}

ViewGestureController::~ViewGestureController()
{
}

void ViewGestureController::installSwipeHandler(UIView *view)
{
    ASSERT(!m_swipeInteractiveTransitionDelegate);
    m_swipeInteractiveTransitionDelegate = adoptNS([[WKSwipeTransitionController alloc] initWithViewGestureController:this swipingView:view]);
    m_liveSwipeView = view;
}

void ViewGestureController::beginSwipeGesture(_UINavigationInteractiveTransitionBase *transition, SwipeDirection direction)
{
    if (m_activeGestureType != ViewGestureType::None)
        return;

    WebKit::WebBackForwardListItem* targetItem = direction == SwipeDirection::Left ? m_webPageProxy.backForwardList().backItem() : m_webPageProxy.backForwardList().forwardItem();

    auto snapshot = WebKit::ViewSnapshotStore::shared().snapshotAndRenderTreeSize(targetItem).first;

    RetainPtr<UIViewController> snapshotViewController = adoptNS([[UIViewController alloc] init]);
    m_snapshotView = adoptNS([[UIView alloc] initWithFrame:[m_liveSwipeView bounds]]);
    if (snapshot) {
#if USE(IOSURFACE)
        uint32_t purgeabilityState = kIOSurfacePurgeableNonVolatile;
        IOSurfaceSetPurgeable(snapshot.get(), kIOSurfacePurgeableNonVolatile, &purgeabilityState);
        
        if (purgeabilityState != kIOSurfacePurgeableEmpty)
            [m_snapshotView layer].contents = (id)snapshot.get();
#else
        [m_snapshotView layer].contents = (id)snapshot.get();
#endif
    }
    [m_snapshotView setBackgroundColor:[UIColor whiteColor]];
    [m_snapshotView layer].contentsGravity = @"topLeft";
    [m_snapshotView layer].contentsScale = m_liveSwipeView.window.screen.scale;
    [snapshotViewController setView:m_snapshotView.get()];

    m_transitionContainerView = adoptNS([[UIView alloc] initWithFrame:[m_liveSwipeView frame]]);

    [m_liveSwipeView.superview insertSubview:m_transitionContainerView.get() belowSubview:m_liveSwipeView];
    [m_transitionContainerView addSubview:m_liveSwipeView];

    RetainPtr<UIViewController> targettedViewController = adoptNS([[UIViewController alloc] init]);
    [targettedViewController setView:m_liveSwipeView];

    m_originalLiveSwipeViewFrame = m_liveSwipeView.frame;
    m_liveSwipeView.frame = m_liveSwipeView.bounds;
    
    UINavigationControllerOperation transitionOperation = direction == SwipeDirection::Left ? UINavigationControllerOperationPop : UINavigationControllerOperationPush;
    RetainPtr<_UINavigationParallaxTransition> animationController = adoptNS([[_UINavigationParallaxTransition alloc] initWithCurrentOperation:transitionOperation]);

    RetainPtr<_UIViewControllerOneToOneTransitionContext> transitionContext = adoptNS([[_UIViewControllerOneToOneTransitionContext alloc] init]);
    [transitionContext _setFromViewController:targettedViewController.get()];
    [transitionContext _setToViewController:snapshotViewController.get()];
    [transitionContext _setContainerView:m_transitionContainerView.get()];
    [transitionContext _setFromStartFrame:[m_liveSwipeView bounds]];
    [transitionContext _setToEndFrame:[m_liveSwipeView bounds]];
    [transitionContext _setToStartFrame:CGRectZero];
    [transitionContext _setFromEndFrame:CGRectZero];
    [transitionContext _setAnimator:animationController.get()];
    [transitionContext _setInteractor:transition];
    [transitionContext _setTransitionIsInFlight:YES];
    [transitionContext _setCompletionHandler:^(_UIViewControllerTransitionContext *context, BOOL didComplete) { endSwipeGesture(targetItem, context, !didComplete); }];

    [transition setAnimationController:animationController.get()];
    [transition startInteractiveTransition:transitionContext.get()];

    m_activeGestureType = ViewGestureType::Swipe;
}

bool ViewGestureController::canSwipeInDirection(SwipeDirection direction)
{
    if (direction == SwipeDirection::Left)
        return !!m_webPageProxy.backForwardList().backItem();
    return !!m_webPageProxy.backForwardList().forwardItem();
}

void ViewGestureController::endSwipeGesture(WebBackForwardListItem* targetItem, _UIViewControllerTransitionContext *context, bool cancelled)
{
    [context _setTransitionIsInFlight:NO];
    [context _setInteractor:nil];
    [context _setAnimator:nil];
    
    [[m_transitionContainerView superview] insertSubview:m_snapshotView.get() aboveSubview:m_transitionContainerView.get()];
    [[m_transitionContainerView superview] insertSubview:m_liveSwipeView aboveSubview:m_transitionContainerView.get()];
    [m_transitionContainerView removeFromSuperview];
    m_transitionContainerView = nullptr;
    
    [m_liveSwipeView setFrame:m_originalLiveSwipeViewFrame];
    [m_snapshotView setFrame:m_originalLiveSwipeViewFrame];
    
    if (cancelled) {
        removeSwipeSnapshot();
        return;
    }
    
    m_targetRenderTreeSize = ViewSnapshotStore::shared().snapshotAndRenderTreeSize(targetItem).second * swipeSnapshotRemovalRenderTreeSizeTargetFraction;
    
    // We don't want to replace the current back-forward item's snapshot
    // like we normally would when going back or forward, because we are
    // displaying the destination item's snapshot.
    ViewSnapshotStore::shared().disableSnapshotting();
    m_webPageProxy.goToBackForwardItem(targetItem);
    ViewSnapshotStore::shared().enableSnapshotting();
    
    if (!m_targetRenderTreeSize) {
        removeSwipeSnapshot();
        return;
    }

    m_swipeWatchdogTimer.startOneShot(swipeSnapshotRemovalWatchdogDuration.count());
}
    
void ViewGestureController::setRenderTreeSize(uint64_t renderTreeSize)
{
    if (m_activeGestureType != ViewGestureType::Swipe)
        return;

    if (m_targetRenderTreeSize && renderTreeSize > m_targetRenderTreeSize)
        removeSwipeSnapshot();
}

void ViewGestureController::swipeSnapshotWatchdogTimerFired(Timer<ViewGestureController>*)
{
    removeSwipeSnapshot();
}

void ViewGestureController::removeSwipeSnapshot()
{
    m_swipeWatchdogTimer.stop();

    if (m_activeGestureType != ViewGestureType::Swipe)
        return;
    
#if USE(IOSURFACE)
    IOSurfaceRef snapshotSurface = (IOSurfaceRef)[m_snapshotView layer].contents;
    if (snapshotSurface)
        IOSurfaceSetPurgeable(snapshotSurface, kIOSurfacePurgeableVolatile, nullptr);
#endif
    
    [m_snapshotView removeFromSuperview];
    m_snapshotView = nullptr;
    
    m_targetRenderTreeSize = 0;
    m_activeGestureType = ViewGestureType::None;
}

} // namespace WebKit
