#include "ac_private.h"

#if (AC_PLATFORM_APPLE && AC_INCLUDE_INPUT)

#include "input_apple.h"

#if !__has_feature(objc_arc)
#error AC must be built with Objective-C ARC (automatic reference counting) enabled
#endif

static inline void
ac_apple_input_mouse_moved_handler(
  ac_input      input_handle,
  GCMouseInput* mouse,
  float         dx,
  float         dy)
{
  AC_OBJC_BEGIN_ARP();

  AC_UNUSED(mouse);

  ac_input_event event;
  AC_ZERO(event);
  event.type = ac_input_event_type_mouse_move;
  event.mouse_move.dx = dx;
  event.mouse_move.dy = dy;
  ac_input_send_event(input_handle, &event);

  AC_OBJC_END_ARP();
}

static inline void
ac_apple_input_mouse_button_handler(
  ac_input        input_handle,
  bool            pressed,
  ac_mouse_button button)
{
  AC_OBJC_BEGIN_ARP();

  ac_input_event event;
  AC_ZERO(event);
  event.type = pressed ? ac_input_event_type_mouse_button_down
                       : ac_input_event_type_mouse_button_up;
  event.mouse_button = button;
  ac_input_send_event(input_handle, &event);

  AC_OBJC_END_ARP();
}

static inline void
ac_apple_input_mouse_connect_handler(ac_input input_handle, GCMouse* mouse)
{
  AC_OBJC_BEGIN_ARP();

  GCMouseInput* input = mouse.mouseInput;

  input.mouseMovedHandler = ^(GCMouseInput* mouse, float dx, float dy) {
    AC_OBJC_BEGIN_ARP();

    ac_apple_input_mouse_moved_handler(input_handle, mouse, dx, dy);

    AC_OBJC_END_ARP();
  };

  input.leftButton.pressedChangedHandler =
    ^(GCControllerButtonInput* button, float value, BOOL pressed) {
      AC_OBJC_BEGIN_ARP();

      AC_UNUSED(button);
      AC_UNUSED(value);

      ac_apple_input_mouse_button_handler(
        input_handle,
        pressed,
        ac_mouse_button_left);

      AC_OBJC_END_ARP();
    };

  input.rightButton.pressedChangedHandler =
    ^(GCControllerButtonInput* button, float value, BOOL pressed) {
      AC_OBJC_BEGIN_ARP();

      AC_UNUSED(button);
      AC_UNUSED(value);

      ac_apple_input_mouse_button_handler(
        input_handle,
        pressed,
        ac_mouse_button_right);

      AC_OBJC_END_ARP();
    };

  input.middleButton.pressedChangedHandler =
    ^(GCControllerButtonInput* button, float value, BOOL pressed) {
      AC_OBJC_BEGIN_ARP();

      AC_UNUSED(button);
      AC_UNUSED(value);

      ac_apple_input_mouse_button_handler(
        input_handle,
        pressed,
        ac_mouse_button_middle);

      AC_OBJC_END_ARP();
    };

  input.scroll.valueChangedHandler =
    ^(GCControllerDirectionPad* dpad, float dx, float dy) {
      AC_OBJC_BEGIN_ARP();

      AC_UNUSED(dpad);

      ac_input_event event;
      AC_ZERO(event);
      event.type = ac_input_event_type_scroll;
      event.scroll.dx = dx;
      event.scroll.dy = dy;
      ac_input_send_event(input_handle, &event);

      AC_OBJC_END_ARP();
    };

  AC_OBJC_END_ARP();
}

static inline void
ac_apple_input_keyboard_connect_handler(
  ac_input    input_handle,
  GCKeyboard* keyboard)
{
  AC_OBJC_BEGIN_ARP();

  GCKeyboardInput* input = keyboard.keyboardInput;

  input.keyChangedHandler =
    ^(GCKeyboardInput*     keyboard,
      GCDeviceButtonInput* key,
      GCKeyCode            keycode,
      BOOL                 pressed) {
      AC_OBJC_BEGIN_ARP();

      AC_UNUSED(keyboard);
      AC_UNUSED(key);

      ac_input_event event;
      AC_ZERO(event);
      event.type =
        pressed ? ac_input_event_type_key_down : ac_input_event_type_key_up;
      event.key = ac_key_from_apple(keycode);
      ac_input_send_event(input_handle, &event);

      AC_OBJC_END_ARP();
    };

  AC_OBJC_END_ARP();
}

static inline void
ac_apple_input_controller_connect_handler(
  ac_input      input_handle,
  GCController* gc)
{
  AC_OBJC_BEGIN_ARP();

  AC_FROM_HANDLE(input, ac_apple_input);

  if (!gc.extendedGamepad)
  {
    return;
  }

  uint32_t i = 0;
  while (i < AC_MAX_GAMEPADS && input->controllers[i] != nil)
  {
    i++;
  }

  if (i >= AC_MAX_GAMEPADS)
  {
    return;
  }

  input->controllers[i] = gc;
  input->types[i] = ac_gamepad_type_unknown;

  if (
    [gc.productCategory isEqualToString:GCProductCategoryXboxOne] ||
    [gc.productCategory localizedCaseInsensitiveContainsString:@"xbox"])
  {
    input->types[i] = ac_gamepad_type_xbox;
  }
  else if (
    [gc.productCategory isEqualToString:GCProductCategoryDualShock4] ||
    [gc.productCategory isEqualToString:GCProductCategoryDualSense])
  {
    input->types[i] = ac_gamepad_type_ps;
  }
  else if ([gc.productCategory
             localizedCaseInsensitiveContainsString:@"switch"])
  {
    input->types[i] = ac_gamepad_type_switch;
  }

  AC_OBJC_END_ARP();
}

static inline void
ac_apple_input_controller_disconnect_handler(
  ac_input      input_handle,
  GCController* gc)
{
  AC_OBJC_BEGIN_ARP();

  AC_FROM_HANDLE(input, ac_apple_input);

  for (uint32_t i = 0; i < AC_MAX_GAMEPADS; ++i)
  {
    if (input->controllers[i] != gc)
    {
      continue;
    }

    input->controllers[i] = nil;
    input->types[i] = ac_gamepad_type_unknown;
    break;
  }

  AC_OBJC_END_ARP();
}

static ac_result
ac_apple_input_init(ac_input* input_handle)
{
  AC_OBJC_BEGIN_ARP();

  AC_INIT_INTERNAL(input, ac_apple_input);

  input->observers = [NSMutableArray new];
  id o = NULL;

  NSNotificationCenter* center = [NSNotificationCenter defaultCenter];

  o = [center
    addObserverForName:GCMouseDidConnectNotification
                object:NULL
                 queue:NULL
            usingBlock:^(NSNotification* note) {
              AC_OBJC_BEGIN_ARP();

              ac_apple_input_mouse_connect_handler(&input->common, note.object);

              AC_OBJC_END_ARP();
            }];
  [input->observers addObject:o];

  o = [center addObserverForName:GCKeyboardDidConnectNotification
                          object:NULL
                           queue:NULL
                      usingBlock:^(NSNotification* note) {
                        AC_OBJC_BEGIN_ARP();

                        ac_apple_input_keyboard_connect_handler(
                          &input->common,
                          note.object);

                        AC_OBJC_END_ARP();
                      }];
  [input->observers addObject:o];

  o = [center addObserverForName:GCControllerDidConnectNotification
                          object:nil
                           queue:nil
                      usingBlock:^(NSNotification* note) {
                        AC_OBJC_BEGIN_ARP();
                        ac_apple_input_controller_connect_handler(
                          &input->common,
                          note.object);
                        AC_OBJC_END_ARP();
                      }];
  [input->observers addObject:o];

  o = [center addObserverForName:GCControllerDidDisconnectNotification
                          object:nil
                           queue:nil
                      usingBlock:^(NSNotification* note) {
                        AC_OBJC_BEGIN_ARP();
                        ac_apple_input_controller_disconnect_handler(
                          &input->common,
                          note.object);
                        AC_OBJC_END_ARP();
                      }];
  [input->observers addObject:o];

  return ac_result_success;

  AC_OBJC_END_ARP();
}

static void
ac_apple_input_shutdown(ac_input input_handle)
{
  AC_OBJC_BEGIN_ARP();

  AC_FROM_HANDLE(input, ac_apple_input);

  NSNotificationCenter* center = [NSNotificationCenter defaultCenter];

  if (input->observers)
  {
    for (uint32_t i = 0; i < input->observers.count; ++i)
    {
      [center removeObserver:input->observers[i]];
    }
    input->observers = NULL;
  }

  for (uint32_t i = 0; i < AC_MAX_GAMEPADS; ++i)
  {
    input->controllers[i] = NULL;
    input->types[i] = ac_gamepad_type_unknown;
  }

  AC_OBJC_END_ARP();
}

static void
ac_apple_input_poll_events(ac_input input_handle)
{
  AC_FROM_HANDLE(input, ac_apple_input);

  for (uint32_t i = 0; i < AC_MAX_GAMEPADS; ++i)
  {
    ac_gamepad_state* dst = &input->common.gamepads[i].state;

    dst->connected = input->controllers[i] != nil;

    if (!dst->connected)
    {
      continue;
    }

    const GCExtendedGamepad* src = input->controllers[i].extendedGamepad;

    dst->buttons[ac_gamepad_button_a] = src.buttonA.pressed;
    dst->buttons[ac_gamepad_button_b] = src.buttonB.pressed;
    dst->buttons[ac_gamepad_button_x] = src.buttonX.pressed;
    dst->buttons[ac_gamepad_button_y] = src.buttonY.pressed;
    dst->buttons[ac_gamepad_button_left_shoulder] = src.leftShoulder.pressed;
    dst->buttons[ac_gamepad_button_right_shoulder] = src.rightShoulder.pressed;
    dst->buttons[ac_gamepad_button_left_thumbstick] =
      src.leftThumbstickButton.pressed;
    dst->buttons[ac_gamepad_button_right_thumbstick] =
      src.rightThumbstickButton.pressed;
    dst->buttons[ac_gamepad_button_dpad_left] = src.dpad.left.pressed;
    dst->buttons[ac_gamepad_button_dpad_right] = src.dpad.right.pressed;
    dst->buttons[ac_gamepad_button_dpad_up] = src.dpad.up.pressed;
    dst->buttons[ac_gamepad_button_dpad_down] = src.dpad.down.pressed;
    dst->buttons[ac_gamepad_button_menu] = src.buttonMenu.pressed;
    dst->buttons[ac_gamepad_button_options] = src.buttonOptions.pressed;

    dst->axis[ac_gamepad_axis_left_thumbstick_x] =
      src.leftThumbstick.xAxis.value;
    dst->axis[ac_gamepad_axis_left_thumbstick_y] =
      src.leftThumbstick.yAxis.value;
    dst->axis[ac_gamepad_axis_left_trigger] = src.leftTrigger.value;

    dst->axis[ac_gamepad_axis_right_thumbstick_x] =
      src.rightThumbstick.xAxis.value;
    dst->axis[ac_gamepad_axis_right_thumbstick_y] =
      src.rightThumbstick.yAxis.value;
    dst->axis[ac_gamepad_axis_right_trigger] = src.rightTrigger.value;
  }
}

void
ac_apple_input_get_impl(ac_input_implementation* impl)
{
  impl->init = ac_apple_input_init;
  impl->shutdown = ac_apple_input_shutdown;
  impl->poll_events = ac_apple_input_poll_events;
}

#endif
