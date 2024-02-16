#include "ac_private.h"

#if (AC_INCLUDE_WINDOW && AC_PLATFORM_APPLE_MACOS)

#include "window_apple_appkit.h"

#if !__has_feature(objc_arc)
#error AC must be built with Objective-C ARC (automatic reference counting) enabled
#endif

static const NSRange EMPTY_RANGE = {NSNotFound, 0};

static inline void
ac_macos_window_create_menu(void)
{
  NSString*   appName;
  NSString*   title;
  NSMenu*     appleMenu;
  NSMenu*     serviceMenu;
  NSMenu*     windowMenu;
  NSMenuItem* menuItem;
  NSMenu*     mainMenu;

  if (NSApp == nil)
  {
    return;
  }

  mainMenu = [[NSMenu alloc] init];

  /* Create the main menu bar */
  [NSApp setMainMenu:mainMenu];

  char** progname = _NSGetProgname();
  if (progname && *progname)
  {
    appName = @(*progname);
  }
  else
  {
    appName = @"ac application";
  }

  /* Create the application menu */
  appleMenu = [[NSMenu alloc] initWithTitle:@""];

  /* Add menu items */
  title = [@"About " stringByAppendingString:appName];
  [appleMenu addItemWithTitle:title
                       action:@selector(orderFrontStandardAboutPanel:)
                keyEquivalent:@""];

  [appleMenu addItem:[NSMenuItem separatorItem]];

  [appleMenu addItemWithTitle:@"Preferences…" action:nil keyEquivalent:@","];

  [appleMenu addItem:[NSMenuItem separatorItem]];

  serviceMenu = [[NSMenu alloc] initWithTitle:@""];
  menuItem = [appleMenu addItemWithTitle:@"Services"
                                  action:nil
                           keyEquivalent:@""];
  [menuItem setSubmenu:serviceMenu];

  [NSApp setServicesMenu:serviceMenu];

  [appleMenu addItem:[NSMenuItem separatorItem]];

  title = [@"Hide " stringByAppendingString:appName];
  [appleMenu addItemWithTitle:title action:@selector(hide:) keyEquivalent:@"h"];

  menuItem = [appleMenu addItemWithTitle:@"Hide Others"
                                  action:@selector(hideOtherApplications:)
                           keyEquivalent:@"h"];
  [menuItem setKeyEquivalentModifierMask:(NSEventModifierFlagOption |
                                          NSEventModifierFlagCommand)];

  [appleMenu addItemWithTitle:@"Show All"
                       action:@selector(unhideAllApplications:)
                keyEquivalent:@""];

  [appleMenu addItem:[NSMenuItem separatorItem]];

  title = [@"Quit " stringByAppendingString:appName];
  [appleMenu addItemWithTitle:title
                       action:@selector(terminate:)
                keyEquivalent:@"q"];

  /* Put menu into the menubar */
  menuItem = [[NSMenuItem alloc] initWithTitle:@""
                                        action:nil
                                 keyEquivalent:@""];
  [menuItem setSubmenu:appleMenu];
  [[NSApp mainMenu] addItem:menuItem];

  /* Tell the application object that this is now the application menu */
  [NSApp setAppleMenu:appleMenu];

  /* Create the window menu */
  windowMenu = [[NSMenu alloc] initWithTitle:@"Window"];

  /* Add menu items */
  [windowMenu addItemWithTitle:@"Close"
                        action:@selector(performClose:)
                 keyEquivalent:@"w"];

  [windowMenu addItemWithTitle:@"Minimize"
                        action:@selector(performMiniaturize:)
                 keyEquivalent:@"m"];

  [windowMenu addItemWithTitle:@"Zoom"
                        action:@selector(performZoom:)
                 keyEquivalent:@""];

  /* Add the fullscreen toggle menu option. */
  /* Cocoa should update the title to Enter or Exit Full Screen automatically.
   * But if not, then just fallback to Toggle Full Screen.
   */
  menuItem = [[NSMenuItem alloc] initWithTitle:@"Toggle Full Screen"
                                        action:@selector(toggleFullScreen:)
                                 keyEquivalent:@"f"];
  [menuItem setKeyEquivalentModifierMask:NSEventModifierFlagControl |
                                         NSEventModifierFlagCommand];
  [windowMenu addItem:menuItem];

  /* Put menu into the menubar */
  menuItem = [[NSMenuItem alloc] initWithTitle:@"Window"
                                        action:nil
                                 keyEquivalent:@""];
  [menuItem setSubmenu:windowMenu];
  [[NSApp mainMenu] addItem:menuItem];

  /* Tell the application object that this is now the window menu */
  [NSApp setWindowsMenu:windowMenu];
}

@implementation ACApplication

- (void)setAcWindow:(ac_window)window
{
  window_handle = window;
}

- (void)terminate:(id)sender
{
  AC_OBJC_BEGIN_ARP();

  ac_window_event event;
  AC_ZERO(event);
  event.type = ac_window_event_type_close;
  ac_window_send_event(window_handle, &event);

  AC_OBJC_END_ARP();
}

+ (void)registerUserDefaults
{
  NSDictionary* appDefaults = [[NSDictionary alloc]
    initWithObjectsAndKeys:[NSNumber numberWithBool:NO],
                           @"AppleMomentumScrollSupported",
                           [NSNumber numberWithBool:NO],
                           @"ApplePressAndHoldEnabled",
                           [NSNumber numberWithBool:YES],
                           @"ApplePersistenceIgnoreState",
                           nil];
  [[NSUserDefaults standardUserDefaults] registerDefaults:appDefaults];
}

@end

@implementation ACAppDelegate

- (void)setAcWindow:(ac_window)window
{
  window_handle = window;
}

- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication*)sender
{
  AC_OBJC_BEGIN_ARP();

  return YES;

  AC_OBJC_END_ARP();
}

- (void)applicationDidChangeScreenParameters:(NSNotification*)notification
{
  AC_OBJC_BEGIN_ARP();

  ac_window_event event;
  AC_ZERO(event);
  event.type = ac_window_event_type_monitor_change;
  ac_window_send_event(window_handle, &event);

  AC_OBJC_END_ARP();
}

- (void)applicationDidFinishLaunching:(NSNotification*)notification
{
  AC_OBJC_BEGIN_ARP();

  [NSApp activateIgnoringOtherApps:YES];

  NSDictionary* app_defaults = [[NSDictionary alloc]
    initWithObjectsAndKeys:[NSNumber numberWithBool:NO],
                           @"AppleMomentumScrollSupported",
                           [NSNumber numberWithBool:NO],
                           @"ApplePressAndHoldEnabled",
                           [NSNumber numberWithBool:YES],
                           @"ApplePersistenceIgnoreState",
                           nil];

  [[NSUserDefaults standardUserDefaults] registerDefaults:app_defaults];

  AC_OBJC_END_ARP();
}

- (void)applicationDidHide:(NSNotification*)notification
{}

- (BOOL)applicationSupportsSecureRestorableState:(NSApplication*)app
{
  return YES;
}

@end

@implementation ACView

- (instancetype)initWithAcWindow:(ac_window)a_window
{
  AC_OBJC_BEGIN_ARP();

  AC_FROM_HANDLE2(window, a_window, ac_macos_window);

  NSRect rect = [window->window contentRectForFrameRect:[window->window frame]];
  self = [super initWithFrame:rect];

  if (self != NULL)
  {
    window_handle = a_window;
    tracking_area = NULL;
    marked_text = [[NSMutableAttributedString alloc] init];

    [self updateTrackingAreas];
    [self registerForDraggedTypes:@[NSPasteboardTypeURL]];

    self.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
  }

  return self;

  AC_OBJC_END_ARP();
}

+ (Class)layerClass
{
  AC_OBJC_BEGIN_ARP();

  return NSClassFromString(@"CAMetalLayer");

  AC_OBJC_END_ARP();
}

- (CALayer*)makeBackingLayer
{
  AC_OBJC_BEGIN_ARP();

  return [self.class.layerClass layer];

  AC_OBJC_END_ARP();
}

- (BOOL)isOpaque
{
  AC_OBJC_BEGIN_ARP();

  AC_FROM_HANDLE(window, ac_macos_window);

  return [window->window isOpaque];

  AC_OBJC_END_ARP();
}

- (BOOL)canBecomeKeyView
{
  return YES;
}

- (BOOL)acceptsFirstResponder
{
  return YES;
}

- (BOOL)wantsUpdateLayer
{
  return YES;
}

- (void)updateLayer
{}

- (void)cursorUpdate:(NSEvent*)event
{
  AC_OBJC_BEGIN_ARP();


  AC_FROM_HANDLE(window, ac_macos_window);

  if (window->hide_cursor)
  {
    [NSCursor hide];
  }
  else
  {
    [NSCursor unhide];
  }

  AC_OBJC_END_ARP();
}

- (BOOL)acceptsFirstMouse:(NSEvent*)event
{
  return YES;
}

- (void)viewDidChangeBackingProperties
{
  AC_OBJC_BEGIN_ARP();

  AC_FROM_HANDLE(window, ac_macos_window);

  const NSRect rc = [window->view frame];
  const NSRect fb_rc = [window->view convertRectToBacking:rc];

  ac_window_event event;
  event.type = ac_window_event_type_resize;
  event.resize.width = (uint32_t)fb_rc.size.width;
  event.resize.height = (uint32_t)fb_rc.size.height;
  ac_window_send_event(&window->common, &event);

  AC_OBJC_END_ARP();
}

- (void)updateTrackingAreas
{
  AC_OBJC_BEGIN_ARP();

  if (tracking_area != NULL)
  {
    [self removeTrackingArea:tracking_area];
    tracking_area = NULL;
  }

  const NSTrackingAreaOptions options =
    NSTrackingMouseEnteredAndExited | NSTrackingActiveInKeyWindow |
    NSTrackingEnabledDuringMouseDrag | NSTrackingCursorUpdate |
    NSTrackingInVisibleRect | NSTrackingAssumeInside;

  tracking_area = [[NSTrackingArea alloc] initWithRect:[self bounds]
                                               options:options
                                                 owner:self
                                              userInfo:NULL];

  [self addTrackingArea:tracking_area];
  [super updateTrackingAreas];

  AC_OBJC_END_ARP();
}

- (void)keyDown:(NSEvent*)event
{
  AC_OBJC_BEGIN_ARP();

  [self interpretKeyEvents:@[event]];

  AC_OBJC_END_ARP();
}

- (NSDragOperation)draggingEntered:(id<NSDraggingInfo>)sender
{
  return NSDragOperationGeneric;
}

- (BOOL)performDragOperation:(id<NSDraggingInfo>)sender
{
  return YES;
}

- (BOOL)hasMarkedText
{
  AC_OBJC_BEGIN_ARP();

  return [marked_text length] > 0;

  AC_OBJC_END_ARP();
}

- (NSRange)markedRange
{
  AC_OBJC_BEGIN_ARP();

  if ([marked_text length] > 0)
  {
    return NSMakeRange(0, [marked_text length] - 1);
  }
  else
  {
    return EMPTY_RANGE;
  }

  AC_OBJC_END_ARP();
}

- (NSRange)selectedRange
{
  return EMPTY_RANGE;
}

- (void)setMarkedText:(id)string
        selectedRange:(NSRange)selectedRange
     replacementRange:(NSRange)replacementRange
{
  AC_OBJC_BEGIN_ARP();

  if ([string isKindOfClass:[NSAttributedString class]])
  {
    marked_text =
      [[NSMutableAttributedString alloc] initWithAttributedString:string];
  }
  else
  {
    marked_text = [[NSMutableAttributedString alloc] initWithString:string];
  }

  AC_OBJC_END_ARP();
}

- (void)unmarkText
{
  AC_OBJC_BEGIN_ARP();

  [[marked_text mutableString] setString:@""];

  AC_OBJC_END_ARP();
}

- (NSArray*)validAttributesForMarkedText
{
  AC_OBJC_BEGIN_ARP();

  return [NSArray array];

  AC_OBJC_END_ARP();
}

- (NSAttributedString*)attributedSubstringForProposedRange:(NSRange)range
                                               actualRange:
                                                 (NSRangePointer)actualRange
{
  return NULL;
}

- (NSUInteger)characterIndexForPoint:(NSPoint)point
{
  return 0;
}

- (NSRect)firstRectForCharacterRange:(NSRange)range
                         actualRange:(NSRangePointer)actualRange
{
  AC_OBJC_BEGIN_ARP();

  AC_FROM_HANDLE(window, ac_macos_window);

  const NSRect frame = [window->view frame];
  return NSMakeRect(frame.origin.x, frame.origin.y, 0.0, 0.0);

  AC_OBJC_END_ARP();
}

- (void)insertText:(id)str replacementRange:(NSRange)replacementRange
{
  AC_OBJC_BEGIN_ARP();

  AC_FROM_HANDLE(window, ac_macos_window);

  NSString* characters;

  if ([str isKindOfClass:[NSAttributedString class]])
  {
    characters = [str string];
  }
  else
  {
    characters = (NSString*)str;
  }

  NSRange range = NSMakeRange(0, [characters length]);
  while (range.length)
  {
    uint32_t codepoint = 0;

    if ([characters getBytes:&codepoint
                   maxLength:sizeof(codepoint)
                  usedLength:NULL
                    encoding:NSUTF32StringEncoding
                     options:0
                       range:range
              remainingRange:&range])
    {
      if (codepoint >= 0xf700 && codepoint <= 0xf7ff)
      {
        continue;
      }

      ac_window_event event;
      event.type = ac_window_event_type_character;
      event.character = codepoint;
      ac_window_send_event(&window->common, &event);
    }
  }

  AC_OBJC_END_ARP();
}

@end

@implementation ACWindow

- (BOOL)canBecomeKeyWindow
{
  return YES;
}

- (BOOL)canBecomeMainWindow
{
  return YES;
}

- (BOOL)wantsPeriodicDraggingUpdates
{
  return NO;
}

- (void)doCommandBySelector:(SEL)selector
{}

@end

@implementation ACWindowDelegate

- (void)setAcWindow:(ac_window)window
{
  window_handle = window;
}

- (BOOL)windowShouldClose:(id)sender
{
  AC_OBJC_BEGIN_ARP();

  ac_window_event event;
  AC_ZERO(event);
  event.type = ac_window_event_type_close;
  ac_window_send_event(window_handle, &event);

  return NO;

  AC_OBJC_END_ARP();
}

- (void)windowDidResize:(NSNotification*)notification
{
  AC_OBJC_BEGIN_ARP();

  AC_FROM_HANDLE(window, ac_macos_window);

  const NSRect rc = [window->view frame];
  const NSRect fb_rc = [window->view convertRectToBacking:rc];

  ac_window_event event;
  event.type = ac_window_event_type_resize;
  event.resize.width = (uint32_t)fb_rc.size.width;
  event.resize.height = (uint32_t)fb_rc.size.height;
  ac_window_send_event(&window->common, &event);

  AC_OBJC_END_ARP();
}

- (void)windowDidBecomeKey:(NSNotification*)notification
{
  AC_OBJC_BEGIN_ARP();

  ac_window_event event;
  event.type = ac_window_event_type_focus_own;
  ac_window_send_event(window_handle, &event);

  AC_OBJC_END_ARP();
}

- (void)windowDidResignKey:(NSNotification*)notification
{
  AC_OBJC_BEGIN_ARP();

  ac_window_event event;
  event.type = ac_window_event_type_focus_lost;
  ac_window_send_event(window_handle, &event);

  AC_OBJC_END_ARP();
}

@end

static void
ac_macos_window_cgerror_callback(void)
{
  AC_ERROR("[ window ] [ macos ] : CoreGraphics.framework error detected");
  AC_DEBUGBREAK();
}

static inline ac_result
ac_macos_window_poll_monitors(ac_window window)
{
  AC_OBJC_BEGIN_ARP();

  uint32_t count;
  if (CGGetActiveDisplayList(0, NULL, &count) != kCGErrorSuccess)
  {
    return ac_result_unknown_error;
  }
  CGDirectDisplayID displays[AC_MAX_MONITORS];
  if (
    CGGetActiveDisplayList(AC_MAX_MONITORS, displays, &count) !=
    kCGErrorSuccess)
  {
    return ac_result_unknown_error;
  }

  for (uint32_t i = 0; i < AC_MIN(count, AC_MAX_MONITORS); ++i)
  {
    CGDirectDisplayID display = displays[i];

    const CGRect bounds = CGDisplayBounds(display);

    ac_monitor* monitor_handle = &window->monitors[i];
    AC_INIT_INTERNAL(monitor, ac_macos_monitor);
    monitor->display = display;
    monitor->common.x = bounds.origin.x;
    monitor->common.y = bounds.origin.y;
    monitor->common.width = bounds.size.width;
    monitor->common.height = bounds.size.height;
  }
  window->monitor_count = count;

  return ac_result_success;

  AC_OBJC_END_ARP();
}

static void
ac_macos_window_shutdown(ac_window window_handle)
{
  AC_OBJC_BEGIN_ARP();

  AC_FROM_HANDLE(window, ac_macos_window);

  window->common.native_window = NULL;
  window->view = NULL;

  if (window->window)
  {
    [window->window orderOut:NULL];

    [window->window setContentView:NULL];
    [window->window setDelegate:NULL];

    window->window = NULL;
  }

  window->window_delegate = NULL;

  [NSApp setDelegate:NULL];
  window->app_delegate = NULL;

  CGErrorSetCallback(NULL);

  ac_unload_library(window->vk);

  AC_OBJC_END_ARP();
}

static ac_result
ac_macos_window_set_state(ac_window window_handle, const ac_window_state* src)
{
  AC_OBJC_BEGIN_ARP();

  AC_FROM_HANDLE(window, ac_macos_window);

  ac_window_state* dst = &window->common.state;

  CGFloat width;
  CGFloat height;

  if (src->monitor)
  {
    dst->monitor = src->monitor;
    width = (CGFloat)src->monitor->width;
    height = (CGFloat)src->monitor->height;
  }
  else
  {
    width = (CGFloat)src->width;
    height = (CGFloat)src->height;
  }

  CGFloat x = dst->monitor->x + (dst->monitor->width / 2.0) - (width / 2.0);
  CGFloat y = dst->monitor->y + (dst->monitor->height / 2.0) - (height / 2.0);
  NSRect  rect = NSMakeRect(x, y, width, height);

  NSWindowStyleMask          style_mask = NSWindowStyleMaskMiniaturizable;
  NSWindowCollectionBehavior collection_behavior =
    NSWindowCollectionBehaviorManaged;
  NSWindowLevel level = NSNormalWindowLevel;

  if (src->monitor)
  {
    level = NSMainMenuWindowLevel + 1;
    style_mask |= NSWindowStyleMaskBorderless;
  }
  else
  {
    if (src->borderless)
    {
      style_mask |= NSWindowStyleMaskBorderless;
    }
    else
    {
      style_mask |= (NSWindowStyleMaskTitled | NSWindowStyleMaskClosable);
    }

    if (src->resizable)
    {
      collection_behavior = NSWindowCollectionBehaviorFullScreenPrimary;
      style_mask |= NSWindowStyleMaskResizable;
    }
  }

  [window->window setStyleMask:style_mask];
  [window->window setFrame:[window->window frameRectForContentRect:rect]
                   display:YES];
  [window->window setLevel:level];
  [window->window setCollectionBehavior:collection_behavior];
  [window->window orderFront:NULL];
  [window->window makeKeyAndOrderFront:NULL];

  [NSApp activateIgnoringOtherApps:YES];

  const NSRect rc = [window->view frame];
  NSSize       size = [window->view convertSizeToBacking:rc.size];

  dst->width = (uint32_t)size.width;
  dst->height = (uint32_t)size.height;

  return ac_result_success;

  AC_OBJC_END_ARP();
}

static float
ac_macos_window_get_dpi_scale(ac_window window_handle)
{
  AC_OBJC_BEGIN_ARP();

  AC_FROM_HANDLE(window, ac_macos_window);
  return (float)window->window.backingScaleFactor;

  AC_OBJC_END_ARP();
}

static void
ac_macos_window_hide_cursor(ac_window window_handle, bool hide)
{
  AC_OBJC_BEGIN_ARP();

  AC_FROM_HANDLE(window, ac_macos_window);

  window->hide_cursor = hide;

  if (hide)
  {
    [NSCursor hide];
  }
  else
  {
    [NSCursor unhide];
  }

  AC_OBJC_END_ARP();
}

static void
ac_macos_window_grab_cursor(ac_window window_handle, bool grab)
{
  AC_OBJC_BEGIN_ARP();

  if (grab)
  {
    AC_FROM_HANDLE(window, ac_macos_window);

    ac_monitor monitor_handle = window->common.state.monitor;
    AC_FROM_HANDLE(monitor, ac_macos_monitor);

    const NSRect rect = [window->window frame];

    CGFloat x = monitor->common.x + rect.origin.x + (rect.size.width / 2);
    CGFloat y = monitor->common.y + rect.origin.y + (rect.size.height / 2);

    CGAssociateMouseAndMouseCursorPosition(NO);
    CGDisplayMoveCursorToPoint(monitor->display, CGPointMake(x, y));
  }
  else
  {
    CGAssociateMouseAndMouseCursorPosition(YES);
  }

  AC_OBJC_END_ARP();
}

static ac_result
ac_macos_window_get_cursor_position(ac_window window_handle, float* x, float* y)
{
  AC_OBJC_BEGIN_ARP();

  AC_FROM_HANDLE(window, ac_macos_window);

  NSSize  size = [window->view convertSizeToBacking:[window->view frame].size];
  NSPoint pt = [window->view
    convertPointToBacking:[window->window mouseLocationOutsideOfEventStream]];

  *x = (float)(pt.x);
  *y = (float)(size.height - pt.y);

  return ac_result_success;

  AC_OBJC_END_ARP();
}

static void
ac_macos_window_poll_events(ac_window window_handle)
{
  AC_OBJC_BEGIN_ARP();

  AC_UNUSED(window_handle);

  while (true)
  {
    NSEvent* e = [NSApp nextEventMatchingMask:NSEventMaskAny
                                    untilDate:[NSDate distantPast]
                                       inMode:NSDefaultRunLoopMode
                                      dequeue:YES];

    if (e == NULL)
    {
      break;
    }

    [NSApp sendEvent:e];
  }

  AC_OBJC_END_ARP();
}

static ac_result
ac_macos_window_get_vk_instance_extensions(
  ac_window    window_handle,
  uint32_t*    count,
  const char** names)
{
  AC_UNUSED(window_handle);

  static const char* extensions[] = {
    VK_KHR_SURFACE_EXTENSION_NAME,
    VK_MVK_MACOS_SURFACE_EXTENSION_NAME,
  };

  *count = AC_COUNTOF(extensions);

  if (names == NULL)
  {
    return ac_result_success;
  }

  for (uint32_t i = 0; i < *count; ++i)
  {
    names[i] = extensions[i];
  }

  return ac_result_success;
}

static ac_result
ac_macos_window_create_vk_surface(
  ac_window window_handle,
  void*     instance,
  void**    surface)
{
  AC_OBJC_BEGIN_ARP();

  AC_FROM_HANDLE(window, ac_macos_window);

  if (!window->vkGetInstanceProcAddr)
  {
    return ac_result_unknown_error;
  }

  PFN_vkCreateMacOSSurfaceMVK vkCreateMacOSSurfaceMVK =
    (PFN_vkCreateMacOSSurfaceMVK)(void*)(window->vkGetInstanceProcAddr(
      (VkInstance)instance,
      "vkCreateMacOSSurfaceMVK"));

  if (!vkCreateMacOSSurfaceMVK)
  {
    return ac_result_unknown_error;
  }

  VkMacOSSurfaceCreateInfoMVK info = {
    .sType = VK_STRUCTURE_TYPE_MACOS_SURFACE_CREATE_INFO_MVK,
    .pView = (__bridge const void*)(window->view),
  };

  window->view.wantsLayer = YES;

  VkResult res = vkCreateMacOSSurfaceMVK(
    (VkInstance)instance,
    &info,
    NULL,
    (VkSurfaceKHR*)surface);

  if (res != VK_SUCCESS)
  {
    return ac_result_unknown_error;
  }

  return ac_result_success;

  AC_OBJC_END_ARP();
}

ac_result
ac_macos_window_init(const char* app_name, ac_window* window_handle)
{
  AC_OBJC_BEGIN_ARP();

  AC_INIT_INTERNAL(window, ac_macos_window);

  window->common.shutdown = ac_macos_window_shutdown;
  window->common.set_state = ac_macos_window_set_state;
  window->common.get_dpi_scale = ac_macos_window_get_dpi_scale;
  window->common.hide_cursor = ac_macos_window_hide_cursor;
  window->common.grab_cursor = ac_macos_window_grab_cursor;
  window->common.get_cursor_position = ac_macos_window_get_cursor_position;
  window->common.poll_events = ac_macos_window_poll_events;
  window->common.get_vk_instance_extensions =
    ac_macos_window_get_vk_instance_extensions;
  window->common.create_vk_surface = ac_macos_window_create_vk_surface;

  CGErrorSetCallback(ac_macos_window_cgerror_callback);

  if (!NSApp)
  {
    [ACApplication sharedApplication];
    if (!NSApp)
    {
      return ac_result_unknown_error;
    }

    [NSApp setAcWindow:&window->common];
    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];

    if (![NSApp mainMenu])
    {
      ac_macos_window_create_menu();
      [NSMenu setMenuBarVisible:YES];
    }

    [NSApp finishLaunching];

    if ([NSApp delegate])
    {
      NSDictionary* app_defaults = [[NSDictionary alloc]
        initWithObjectsAndKeys:[NSNumber numberWithBool:NO],
                               @"AppleMomentumScrollSupported",
                               [NSNumber numberWithBool:NO],
                               @"ApplePressAndHoldEnabled",
                               [NSNumber numberWithBool:YES],
                               @"ApplePersistenceIgnoreState",
                               nil];
      [[NSUserDefaults standardUserDefaults] registerDefaults:app_defaults];
    }
  }

  if (NSApp && !window->app_delegate)
  {
    window->app_delegate = [[ACAppDelegate alloc] init];
    [window->app_delegate setAcWindow:&window->common];
    [NSApp setDelegate:window->app_delegate];
  }

  ac_macos_window_poll_monitors(*window_handle);

  window->window_delegate = [[ACWindowDelegate alloc] init];

  if (window->window_delegate == NULL)
  {
    AC_ERROR("[ window ] [ macos ] : failed to create window delegate");
    return ac_result_unknown_error;
  }

  [window->window_delegate setAcWindow:&window->common];

  window->window = [[ACWindow alloc] init];

  if (window->window == NULL)
  {
    AC_ERROR("[ window ] [ macos ] : failed to create window");
    return ac_result_unknown_error;
  }

  window->view = [[ACView alloc] initWithAcWindow:&window->common];
  [window->view setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];

  if (window->view == NULL)
  {
    AC_ERROR("[ window ] [ macos ]: failed to create view");
    return ac_result_unknown_error;
  }

  [window->window setTitle:@(app_name)];
  [window->window setContentView:window->view];
  [window->window makeFirstResponder:window->view];
  [window->window setDelegate:window->window_delegate];
  [window->window setAcceptsMouseMovedEvents:YES];
  [window->window setTabbingMode:NSWindowTabbingModeDisallowed];

  window->common.native_window = (__bridge void*)(window->window);

  const char* vk_lib_names[] = {
    "libvulkan.dylib",
    "libvulkan.1.dylib",
    "libMoltenVK.dylib",
  };

  window->vkGetInstanceProcAddr = NULL;

  for (uint32_t i = 0; i < AC_COUNTOF(vk_lib_names); ++i)
  {
    ac_result res = ac_load_library(vk_lib_names[i], &window->vk, false);

    if (res != ac_result_success)
    {
      continue;
    }

    AC_RIF(ac_load_function(
      window->vk,
      "vkGetInstanceProcAddr",
      (void**)&window->vkGetInstanceProcAddr));
  }

  return ac_result_success;

  AC_OBJC_END_ARP();
}

#endif
