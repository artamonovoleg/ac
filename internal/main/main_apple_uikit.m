#include "ac_private.h"

#if (AC_PLATFORM_APPLE_NOT_MACOS)

#import <UIKit/UIKit.h>

int
main(int argc, char** argv)
{
  int res = 0;

  AC_OBJC_BEGIN_ARP();

  NSBundle* bundle = [NSBundle mainBundle];
  [[NSFileManager defaultManager]
    changeCurrentDirectoryPath:[bundle resourcePath]];
  res = UIApplicationMain(argc, argv, nil, @"ACAppDelegate");

  AC_OBJC_END_ARP();

  return res;
}

#endif
