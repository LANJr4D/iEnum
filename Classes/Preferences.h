//
//  Preferences.h
//  DomeinZoeker
//
//  Created by Maarten Wullink on 01-05-10.
//  Copyright 2010 SIDN. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface Preferences : NSObject 
{
}

+ (NSString *)serverHostName;
+ (BOOL) setPreferences:(NSString *)serverHostName;

@end