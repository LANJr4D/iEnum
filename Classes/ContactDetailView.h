//
//  Test.h
//  iEnum
//
//  Created by Maarten Wullink on 12-01-11.
//  Copyright 2011 SIDN. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <QuartzCore/QuartzCore.h>
#import "ContactProfile.h"
#import "RecordUtil.h"

@interface ContactDetailView : UIView {

	NSString *number;
	ContactProfile *profile;	
	
}

@property (nonatomic, retain) 	NSString *number;
@property (nonatomic, retain) 	ContactProfile *profile;


-(id)initWithFrame:(CGRect)rect andProfile:(ContactProfile *)profile;
-(void)load;
-(void)createBottomButtons;
-(UIButton *)createButtonWithX:(int)xPos andImage:(UIImage *) image;
- (id) traverseResponderChainForUIViewController;
- (UIViewController *) firstAvailableUIViewController;
-(UIImageView *)createServicesIcon:(BOOL)active andRect:(CGRect)rect;
-(void)createServicesPart;
-(void)createAddressPart;
-(void)createGui;
-(UILabel *) createLabelWithRect:(CGRect)rect andText:(NSString *)text andFont:(UIFont *)font;


@end
