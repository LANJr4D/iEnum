//
//  ContactsViewController.h
//  iEnum
//
//  Created by Maarten Wullink on 12-01-11.
//  Copyright 2011 SIDN. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <AddressBookUI/AddressBookUI.h>
#import "ABContact.h"
#import "NumberSelectionController.h"
#import "DnsResolver.h"


@interface ContactsViewController : UITableViewController<ProfileDelegate> {

	NSArray *contactList;
	ContactProfile *profile;
}

@property (nonatomic, retain) NSArray *contactList;
@property (nonatomic, retain) ContactProfile *profile;

@end
