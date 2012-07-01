//
//  AppDelegate.h
//  iEnum
//
//  Created by Maarten Wullink on 12-01-11.
//  Copyright 2011 SIDN. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "AppSettings.h"
#import "BDHost.h"

@interface AppDelegate : NSObject <UIApplicationDelegate, UITabBarControllerDelegate> {
    UIWindow *window;
    UITabBarController *tabBarController;
	
	UIActivityIndicatorView *activityWheel;
	UIView *activityView;
	BOOL activityViewShowing;
	
	AppSettings *settings;
	
}



@property (nonatomic, retain) IBOutlet UIWindow *window;
@property (nonatomic, retain) IBOutlet UITabBarController *tabBarController;
@property (nonatomic, retain) AppSettings *settings;


-(void)hideActivityViewer;
-(void)showActivityViewer;

-(void)loadSettings;
-(void) saveSettings;
-(void)createHostsFile;


@end
