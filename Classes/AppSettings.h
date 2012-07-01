//
//  AppSettings.h
//  iEnum
//
//  Created by Maarten Wullink on 12-01-11.
//  Copyright 2011 SIDN. All rights reserved.
//

#import <Foundation/Foundation.h>


@interface AppSettings : NSObject {

	NSString *server;
	NSString *suffix;
	NSString *countrycode;
}

@property (nonatomic, retain) NSString *server;
@property (nonatomic, retain) NSString *suffix;
@property (nonatomic, retain) NSString *countrycode;

@end
