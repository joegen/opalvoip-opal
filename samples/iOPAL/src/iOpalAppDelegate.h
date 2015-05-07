//
//  iOpalAppDelegate.h
//  iOPAL
//
//  Created by Robert Jongbloed on 4/04/13.
//  Copyright (c) 2013 Vox Lucida Pty. Ltd. All rights reserved.
//

#import <UIKit/UIKit.h>

#include <opal.h>

@interface iOpalAppDelegate : UIResponder <UIApplicationDelegate>

@property (strong, nonatomic) UIWindow *window;

- (OpalHandle)getOpalHandle;

@end
