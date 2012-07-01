//
//  MapViewController.h
//  iEnum
//
//  Created by Maarten Wullink on 04-01-11.
//  Copyright 2011 SIDN. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <MapKit/MapKit.h>
#import "MapAnnotation.h"


@interface MapViewController : UIViewController <MKMapViewDelegate, CLLocationManagerDelegate> {
	MKMapView *mapview;
	MapAnnotation *annotation;
	CLLocationManager *locationManager;
	 CLLocation *currentLocation;
	MapAnnotation *currentLocationAnotation;
}

@property(nonatomic, retain) MKMapView *mapview;
@property(nonatomic, retain) MapAnnotation *annotation;
@property (nonatomic, retain) CLLocationManager *locationManager;
@property (nonatomic, retain) CLLocation *currentLocation;
@property (nonatomic, retain) MapAnnotation *currentLocationAnotation;

-(id)initWithAnnotation:(MapAnnotation *)annotation;
-(void) startLocationManager;
-(void) stopLocationManager;

@end
