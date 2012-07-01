//
//  EnumService.h
//  iEnum
//
//  Created by Maarten Wullink on 04-01-11.
//  Copyright 2011 SIDN. All rights reserved.
//

#import <UIKit/UIKit.h>

#define SERVICE_WEB  0
#define SERVICE_KEY  1
#define SERVICE_LOC  2
#define SERVICE_MAIL 3
#define SERVICE_VOICE 4

@interface EnumService : NSObject {

	NSString *url;
	UIImage *icon;
	int type;
	NSString *title;
	
}

@property(nonatomic, retain) NSString *url;
@property(nonatomic, retain) UIImage *icon;
@property(nonatomic, retain) NSString *title;

-(int)getType;
-(void)setType:(int)type;

@end
