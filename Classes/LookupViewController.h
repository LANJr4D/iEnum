//
//  LookupViewController.h
//  iEnum
//
//  Created by Maarten Wullink on 12-01-11.
//  Copyright 2011 SIDN. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "DnsResolver.h"
#import "RecordNaptr.h"
#import "AppDelegate.h"
#import "ContactProfile.h"
#import "ModalAlert.h"
#import "ContactDetailsViewController.h"
#import "ContactDetailView.h"



@interface LookupViewController : UIViewController <UISearchBarDelegate,ProfileDelegate > {
	UISearchBar *searchBar;
	ContactProfile *profile;
	UIView *detailView;
}

@property (retain) UISearchBar *searchBar;
@property (retain) ContactProfile *profile;
@property (retain) UIView *detailView;


/**
* Do an enum lookup
* The query parameter must contain a valid phonenumer including 
* international prefix.
* for example: +3112345678
*/
-(void)doLookup:(NSString *)number;


@end
