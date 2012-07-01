//
//  InstellingenView.h
//  DomeinZoeker
//
//  Created by Maarten Wullink on 01-05-10.
//  Copyright 2010 SIDN. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <QuartzCore/QuartzCore.h>
#import "AppDelegate.h"
#import "ModalAlert.h"
#import "SettingsViewEditorController.h"


@interface SettingsView : UITableViewController<SettingDelegate> {
	AppSettings *settings;
	
}


@property(nonatomic, retain) AppSettings *settings;

-(void)settingUpdated:(NSString *)settingValue;
-(NSString *)getValue:(NSString *)value;
@end
