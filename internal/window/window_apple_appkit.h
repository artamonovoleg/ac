#pragma once

#include "ac_private.h"

#if (AC_INCLUDE_WINDOW && AC_PLATFORM_APPLE_MACOS)

#import <Cocoa/Cocoa.h>
#import <QuartzCore/CAMetalLayer.h>
#import <crt_externs.h>

#define VK_NO_PROTOTYPES 1
#define VK_USE_PLATFORM_MACOS_MVK 1
#include <vulkan/vulkan.h>
#include "window.h"

@interface
NSApplication (NSAppleMenu)
- (void)setAppleMenu:(NSMenu*)menu;
@end

@interface ACApplication : NSApplication {
  ac_window window_handle;
}

- (void)setAcWindow:(ac_window)window;
- (void)terminate:(id)sender;
+ (void)registerUserDefaults;
@end

@interface ACAppDelegate : NSObject <NSApplicationDelegate> {
  ac_window window_handle;
}

- (void)setAcWindow:(ac_window)window;
- (NSApplicationTerminateReply)applicationShouldTerminate:
  (NSApplication*)sender;
- (void)applicationDidChangeScreenParameters:(NSNotification*)notification;
- (void)applicationDidFinishLaunching:(NSNotification*)notification;
- (void)applicationDidHide:(NSNotification*)notification;
- (BOOL)applicationSupportsSecureRestorableState:(NSApplication*)app;
@end

@interface ACView : NSView <NSTextInputClient> {
  ac_window                  window_handle;
  NSTrackingArea*            tracking_area;
  NSMutableAttributedString* marked_text;
}

- (instancetype)initWithAcWindow:(ac_window)window;

+ (Class)layerClass;
- (CALayer*)makeBackingLayer;
- (BOOL)isOpaque;
- (BOOL)canBecomeKeyView;
- (BOOL)acceptsFirstResponder;
- (BOOL)wantsUpdateLayer;
- (void)updateLayer;
- (void)cursorUpdate:(NSEvent*)event;
- (BOOL)acceptsFirstMouse:(NSEvent*)event;
- (void)viewDidChangeBackingProperties;
- (void)updateTrackingAreas;
- (void)keyDown:(NSEvent*)event;
- (NSDragOperation)draggingEntered:(id<NSDraggingInfo>)sender;
- (BOOL)performDragOperation:(id<NSDraggingInfo>)sender;
- (BOOL)hasMarkedText;
- (NSRange)markedRange;
- (NSRange)selectedRange;
- (void)unmarkText;
- (NSArray*)validAttributesForMarkedText;
- (NSAttributedString*)attributedSubstringForProposedRange:(NSRange)range
                                               actualRange:
                                                 (NSRangePointer)actualRange;
- (NSUInteger)characterIndexForPoint:(NSPoint)point;
- (NSRect)firstRectForCharacterRange:(NSRange)range
                         actualRange:(NSRangePointer)actualRange;
- (void)insertText:(id)str replacementRange:(NSRange)replacementRange;
@end

@interface ACWindow : NSWindow
- (BOOL)canBecomeKeyWindow;
- (BOOL)canBecomeMainWindow;
- (BOOL)wantsPeriodicDraggingUpdates;
- (void)doCommandBySelector:(SEL)selector;
@end

@interface ACWindowDelegate : NSObject <NSWindowDelegate> {
  ac_window window_handle;
}

- (void)setAcWindow:(ac_window)window;

- (BOOL)windowShouldClose:(id)sender;
- (void)windowDidResize:(NSNotification*)notification;
- (void)windowDidBecomeKey:(NSNotification*)notification;
- (void)windowDidResignKey:(NSNotification*)notification;
@end

typedef struct ac_macos_monitor {
  ac_monitor_internal common;
  CGDirectDisplayID   display;
} ac_macos_monitor;

typedef struct ac_macos_window {
  ac_window_internal common;
  id                 app_delegate;
  NSWindow*          window;
  ACWindowDelegate*  window_delegate;
  ACView*            view;

  bool hide_cursor;

  void*                     vk;
  PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr;
} ac_macos_window;

#endif
