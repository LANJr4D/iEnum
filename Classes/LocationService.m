//
//  LocationService.m
//  iEnum
//
//  Created by Maarten Wullink on 08-01-11.
//  Copyright 2011 SIDN. All rights reserved.
//

#import "LocationService.h"


@implementation LocationService

@synthesize latitude;
@synthesize longtitude;


-(id)initWithString:(NSString *)data{
	self = [super init];
	if(self){
		//set url, only used internally
		self.url = @"map service";
		//parse raw input to longtitude and latitude
		[self parse:data];
	}
	
	return self;
}


-(void)parse:(NSString *)data{
	if(!data || [data length] == 0){
		//nothing to parse
		return;
	}

	//repmove the "geo:" part
	NSString *cleanString = [data stringByReplacingOccurrencesOfString:@"geo:" withString:@""];
	NSArray *chunks = [cleanString componentsSeparatedByString: @","];
	
	if (!chunks || [chunks count] != 2) {
		//error must be two parts
		return;
	}
	self.latitude = [chunks objectAtIndex:0];
	self.longtitude = [chunks objectAtIndex:1];

}


@end
