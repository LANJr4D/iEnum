//
//  ContactDetailsView.h
//  iEnum
//
//  Created by Maarten Wullink on 05-12-10.
//  Copyright 2010 SIDN. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <QuartzCore/QuartzCore.h>
#import "RecordUtil.h"
#import "ABContact.h"
#import "DnsResolver.h"
#import "AppDelegate.h"
#import "Vcard.h"
#import "ContactProfile.h"
#import "EnumServiceManager.h"
#import "ContactDetailView.h"
#import "WebContentViewController.h"
#import "MapViewController.h"
#import "EmailViewController.h"
#import "LocationService.h"
#import "MapAnnotation.h"
#import <MessageUI/MFMailComposeViewController.h>
#import "ModalAlert.h"
#import <MessageUI/MessageUI.h>



@interface ContactDetailsViewController : UIViewController<ProfileDelegate, MFMailComposeViewControllerDelegate> {

	NSString *number;
	ContactProfile *profile;

}

@property (nonatomic, retain) 	NSString *number;
@property (nonatomic, retain) 	ContactProfile *profile;


- (id)initWithProfile:(ContactProfile *)aProfile;
-(void)sendEmailToAddress:(NSString *)address;


@end
