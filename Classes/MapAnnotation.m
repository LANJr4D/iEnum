//
//  MapAnnotation.m
//  iEnum
//
//  Created by Maarten Wullink on 04-01-11.
//  Copyright 2011 SIDN. All rights reserved.
//

#import "MapAnnotation.h"


@implementation MapAnnotation

@synthesize coordinate;
@synthesize title;
@synthesize subtitle;


- (id) initWithLatitude:(CLLocationDegrees) lat longitude:(CLLocationDegrees) lng title:(NSString *)aTitle subTitle:(NSString *)aSubTitle {
	coordinate.latitude = lat;
	coordinate.longitude = lng;
	
	self.title = aTitle;
	self.subtitle = aSubTitle;
	return self;
}

- (void)dealloc {
    [super dealloc];
}


@end
