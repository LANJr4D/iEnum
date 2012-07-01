//
//  InstellingenViewEditorController.h
//  DomeinZoeker
//
//  Created by Maarten Wullink on 04-05-10.
//  Copyright 2010 SIDN. All rights reserved.
//

#import <UIKit/UIKit.h>


@protocol SettingDelegate <NSObject>
-(void)settingUpdated:(NSString *)settingValue;
@end

@interface SettingsViewEditorController : UIViewController <UITextFieldDelegate> {
	IBOutlet UITextField *txtField;
	NSString *txtText;
	NSString *txtPlaceHolder;
	NSObject<SettingDelegate> *delegate;

}

@property (nonatomic, retain) UITextField *txtField;
@property (nonatomic, retain) NSString *txtText;
@property (nonatomic, retain) NSString *txtPlaceHolder;
@property (nonatomic, retain) NSObject<SettingDelegate> *delegate;

- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil fieldTitle:(NSString *)fieldTitle fieldText:(NSString *)fieldText;
@end
