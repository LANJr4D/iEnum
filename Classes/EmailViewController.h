//
//  EmailViewController.h
//  iEnum
//
//  Created by Maarten Wullink on 06-01-11.
//  Copyright 2011 SIDN. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "CryptoService.h"
#import <QuartzCore/QuartzCore.h>
#import <MessageUI/MessageUI.h>
#import <MessageUI/MFMailComposeViewController.h>
#import "ModalAlert.h"
#import "AppDelegate.h"



@interface EmailViewController : UIViewController <CryptoDelegate, MFMailComposeViewControllerDelegate> {

	UITextView *textview;
	CryptoService *cryptoService; 
}

@property(nonatomic, retain) UITextView *textviev;
@property(nonatomic, retain) CryptoService *cryptoService; 

- (id)initWithCrypto:(CryptoService *)crypto;


@end
