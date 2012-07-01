//
//  NumberSelectionController.h
//  iEnum
//
//  Created by Maarten Wullink on 04-12-10.
//  Copyright 2010 SIDN. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "ABContact.h"
#import "ContactDetailsViewController.h"
#import "ContactProfile.h"
#import "ModalAlert.h"
#import "DnsResolver.h"

@interface NumberSelectionController : UITableViewController<ProfileDelegate> {

	ABContact *contact;
	ContactProfile *profile;
}

@property (nonatomic, retain) ABContact *contact;
@property (nonatomic, retain) ContactProfile *profile;

- (NSString *) displayPropertyName:(NSString *) propConst;


@end
