//
//  MapAnnotation.h
//  iEnum
//
//  Created by Maarten Wullink on 04-01-11.
//  Copyright 2011 SIDN. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <MapKit/MapKit.h>

@interface MapAnnotation : NSObject<MKAnnotation> {
    CLLocationCoordinate2D coordinate;
	NSString * title;
	NSString * subtitle;
}

@property (nonatomic, readwrite) CLLocationCoordinate2D coordinate;
@property (nonatomic, retain) NSString *title;
@property (nonatomic, retain) NSString *subtitle;


- (id) initWithLatitude:(CLLocationDegrees) lat longitude:(CLLocationDegrees) lng title:(NSString *)aTitle subTitle:(NSString *)aSubTitle;

@end
