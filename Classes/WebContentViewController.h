//
//  WebContentViewController.h
//  DomeinZoeker
//
//  Created by Maarten Wullink on 09-05-10.
//  Copyright 2010 SIDN. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "EnumService.h"


@interface WebContentViewController : UIViewController<UIWebViewDelegate> {

	EnumService *enumService;
}

@property(nonatomic, retain) EnumService *enumService;

- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil andService:(EnumService *)aEnumService;

@end
