//
//  iOpalAppDelegate.m
//  iOPAL
//
//  Created by Robert Jongbloed on 4/04/13.
//  Copyright (c) 2013 Vox Lucida Pty. Ltd. All rights reserved.
//

#import "iOpalAppDelegate.h"

@implementation iOpalAppDelegate

OpalHandle opalHandle;
unsigned   opalVersion = OPAL_C_API_VERSION;

- (OpalHandle)getOpalHandle
{
  return opalHandle;
}


- (void)opalMain
{
  OpalMessage * msg;
  while ((msg = OpalGetMessage(opalHandle, 0x7fffffff)) != 0) {
    [self.window.rootViewController
        performSelectorOnMainThread:@selector(handleMessage:)
                        withObject:[NSValue valueWithPointer:msg]
                     waitUntilDone:YES
    ];
    OpalFreeMessage(msg);
  }
}


- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions
{
  if (opalHandle == 0) {
    NSString * dir = NSHomeDirectory();
    NSString * args = [NSString stringWithFormat:@
            " " OPAL_PREFIX_H323
            " " OPAL_PREFIX_SIP
            " " OPAL_PREFIX_PCSS
            " --config \"%@/Documents\""
            " --plugin \"%@/Library\""
            " --output \"%@/tmp/opal.log\""
            " -ttttt", dir, dir, dir];
    if ((opalHandle = OpalInitialise(&opalVersion, [args UTF8String])) == 0)
      return NO;
    
    [NSThread detachNewThreadSelector:@selector(opalMain) toTarget:self withObject:nil];
  }

  return YES;
}


- (void)applicationWillResignActive:(UIApplication *)application
{
  // Sent when the application is about to move from active to inactive state. This can occur for certain types of temporary interruptions (such as an incoming phone call or SMS message) or when the user quits the application and it begins the transition to the background state.
  // Use this method to pause ongoing tasks, disable timers, and throttle down OpenGL ES frame rates. Games should use this method to pause the game.
}


- (void)applicationDidEnterBackground:(UIApplication *)application
{
  // Use this method to release shared resources, save user data, invalidate timers, and store enough application state information to restore your application to its current state in case it is terminated later. 
  // If your application supports background execution, this method is called instead of applicationWillTerminate: when the user quits.
}


- (void)applicationWillEnterForeground:(UIApplication *)application
{
  // Called as part of the transition from the background to the inactive state; here you can undo many of the changes made on entering the background.
}


- (void)applicationDidBecomeActive:(UIApplication *)application
{
  // Restart any tasks that were paused (or not yet started) while the application was inactive. If the application was previously in the background, optionally refresh the user interface.
}


- (void)applicationWillTerminate:(UIApplication *)application
{
  // Called when the application is about to terminate. Save data if appropriate. See also applicationDidEnterBackground:.
  if (opalHandle != 0) {
    OpalShutDown(opalHandle);
    opalHandle = 0;
  }
}


@end
