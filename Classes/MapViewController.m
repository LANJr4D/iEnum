//
//  MapViewController.m
//  iEnum
//
//  Created by Maarten Wullink on 04-01-11.
//  Copyright 2011 SIDN. All rights reserved.
//

#import "MapViewController.h"


@implementation MapViewController

@synthesize mapview;
@synthesize annotation;
@synthesize locationManager;
@synthesize currentLocation;
@synthesize currentLocationAnotation;

BOOL showUserLocation = NO;

-(id)initWithAnnotation:(MapAnnotation *)aAnnotation{
	self = [super init];
	if(self){
		self.annotation = aAnnotation;
		self.locationManager = [[[CLLocationManager alloc] init] autorelease];
		self.locationManager.delegate = self;
	}
	
	return self;
}


-(void)loadView{
	[super loadView];
	
	self.title = NSLocalizedString(@"MAP.TITLE", nil);
	
	self.view = [[UIView alloc] initWithFrame:CGRectMake(0, 0, 320, 480)];
	
	UIBarButtonItem *btnShowLocation = [[[UIBarButtonItem alloc]
								 initWithTitle:NSLocalizedString(@"MAP.BUTTON.SHOWLOCATION",nil)
								 style:UIBarButtonItemStyleBordered 
								 target:self 
								 action:@selector(showUserLocation:)] autorelease];
	
	 self.navigationItem.rightBarButtonItem = btnShowLocation;
	
	self.mapview = [[MKMapView alloc] initWithFrame:self.view.bounds];
	[self.view insertSubview:self.mapview atIndex:0];
	
	
	MKCoordinateRegion region;
	
	region.center.latitude = self.annotation.coordinate.latitude;
	region.center.longitude = self.annotation.coordinate.longitude ;
	region.span.latitudeDelta=0.1f;
	region.span.longitudeDelta=0.1f;
	
	[self.mapview setRegion:region animated:TRUE];
	[self.mapview regionThatFits:region];	
	self.mapview.zoomEnabled = YES;
	
	[self.mapview addAnnotation:self.annotation];
	self.mapview.delegate = self;
	
	[self.mapview selectAnnotation:self.annotation animated:YES];
}



- (MKAnnotationView *) mapView:(MKMapView *)mapView viewForAnnotation:(id <MKAnnotation>)aAnnotation{
	MKPinAnnotationView *annView=[[[MKPinAnnotationView alloc] initWithAnnotation:aAnnotation reuseIdentifier:@"currentloc"] autorelease];
	
	//if current anotation then pin color is blue, else color will be red
	if (self.currentLocationAnotation == aAnnotation) {
		annView.pinColor = MKPinAnnotationColorGreen;	
	}else {
		annView.pinColor = MKPinAnnotationColorRed;
	}

    annView.animatesDrop=TRUE;
	[annView setCanShowCallout:YES];
	[annView setSelected:YES animated:YES];
    annView.calloutOffset = CGPointMake(-5, 5);
	annView.selected = YES;
    return annView;
}

 - (void)mapViewDidFinishLoadingMap:(MKMapView *)mapView{
	 //select the annotation to display the callut without the user having to tocu it first
	 [self.mapview selectAnnotation:self.annotation animated:NO];
	 
 }

-(void)showUserLocation:(id)sender{

	if (!showUserLocation) {
		//show user location
        showUserLocation = YES;
		
		[self startLocationManager];
		
		//the pin will be added when the locationmanager send the location the the delegate
	}else {
		//do not show user location
		[self stopLocationManager];
		
		//remove the annotation pin from the map
		if(self.currentLocationAnotation){
			[self.mapview removeAnnotation:self.currentLocationAnotation];
			self.currentLocationAnotation = nil;
		}
		
		showUserLocation = NO;
	}
}

-(void) startLocationManager {
    [self.locationManager startUpdatingLocation];
}

-(void) stopLocationManager {
	if (self.locationManager) {
		[self.locationManager stopUpdatingLocation];
	}
    
}
	 
- (void)locationManager:(CLLocationManager *)manager didUpdateToLocation:(CLLocation *)newLocation fromLocation:(CLLocation *)oldLocation{
	//if the time interval returned from core location is more than two minutes we ignore it because it might be from an old session
    if ( abs([newLocation.timestamp timeIntervalSinceDate: [NSDate date]]) < 60) {   
		if(!signbit(newLocation.horizontalAccuracy) && newLocation.horizontalAccuracy <= kCLLocationAccuracyThreeKilometers){
			self.currentLocation = newLocation;
			if (!self.currentLocationAnotation) {
				//add a new annotation wit the current location to the map
				self.currentLocationAnotation =  [[MapAnnotation alloc] initWithLatitude:self.currentLocation.coordinate.latitude longitude:self.currentLocation.coordinate.longitude
																				   title:NSLocalizedString(@"MAP.PIN.CURRENT.LOCATION", nil) subTitle:@""];
				
				[self.mapview addAnnotation:self.currentLocationAnotation];
				[self.mapview selectAnnotation:self.currentLocationAnotation animated:YES];
				
				//make sure both the contact and current user are visible
				MKCoordinateRegion newRegion;
				newRegion.center.latitude = self.currentLocationAnotation.coordinate.latitude;
				newRegion.center.longitude = self.currentLocationAnotation.coordinate.longitude;        
				newRegion.span.latitudeDelta = 0.112872;
				newRegion.span.longitudeDelta = 0.109863;
				
				[self.mapview setRegion:newRegion animated:YES];
				
			}else {
				//update current location annotation which is already on the map
				
				//update the annotation coordinates
				self.currentLocationAnotation.coordinate = newLocation.coordinate;
			}
			
			
		}
        
	}	
}
	 

- (void)dealloc {
   
	if(self.locationManager){
		[self stopLocationManager];
		self.locationManager.delegate = nil;
		self.locationManager = nil;
	}
	
	self.mapview.delegate = nil;
	self.mapview = nil;
	showUserLocation = NO;
	
    [super dealloc];
}


@end
