//
//  ContactProfile.m
//  iEnum
//
//  Created by Maarten Wullink on 06-12-10.
//  Copyright 2010 SIDN. All rights reserved.
//

#import "ContactProfile.h"


@implementation ContactProfile

@synthesize enumServices;
@synthesize publicKey;
@synthesize image;
@synthesize vcard;
@synthesize naptrList;
@synthesize delegate;
@synthesize number;
@synthesize timer;
@synthesize thread;
@synthesize status;


NSInteger servicecount;

-(id)initWithNumber:(NSString *)aNumber{
	self = [super init];
	if(self){
		self.number = aNumber;
	}
	
	return self;
}


-(void)loadProfile{
	NSLog(@"loadProfile");
	servicecount = 0;
	//set a 10 second timeout value
	self.timer = [NSTimer scheduledTimerWithTimeInterval:12 
									 target:self 
								   selector:@selector(timeout) 
								   userInfo:nil 
									repeats:NO];
	
	
	self.thread = [[NSThread alloc] initWithTarget:self selector:@selector(start) object:nil];
	[self.thread start];
}


-(void)timeout{
	NSLog(@"timeout");
	//creating profile failed to complete on time
	if (self.timer!= nil) {
		[self.timer invalidate];
	}
	
	[self.thread cancel];
	self.status = @"ERROR";
	[delegate profileProcessed:[[NSNumber alloc] initWithInt:10]];
	
}
-(void)start{
	NSLog(@"start");
	//create release pool for use in the thread
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	//do the loading
	[self doQuery];
	[self initProfile];
	[self processDnsRecords];
	NSLog(@"done processing profile, stop timer");
	//stop the timeout timer
	if (self.timer != nil) {
		[self.timer invalidate];
	}
	
	
	//signal caller loading is done
	self.status = @"OK";
	[delegate performSelectorOnMainThread:@selector(profileProcessed:) withObject:[[NSNumber alloc] initWithInt:servicecount] waitUntilDone:true];
	
	//release the pool
	[pool release];
}

-(void)doQuery{
	NSLog(@"doQuery");
	AppDelegate *appDelegate= (AppDelegate *) [[UIApplication sharedApplication] delegate];
	
	DnsResolver *dns = [[[DnsResolver alloc] init] autorelease];
    dns.suffix = appDelegate.settings.suffix;
	
	//remove all non digits and non + character
	self.number = [[self.number componentsSeparatedByCharactersInSet:[[NSCharacterSet characterSetWithCharactersInString:@"0123456789+"] invertedSet]] componentsJoinedByString:@""];
	if(!([self.number rangeOfString:@"+"].location == 0)){
		//remove any leading zero
		if([self.number rangeOfString:@"0"].location == 0){
			self.number = [self.number substringWithRange:NSMakeRange(1,[self.number length]-1)];
		}
		
		//add the user defined country code or the default +31 country code
		if (appDelegate.settings.countrycode) {
			//remove leading zero
			self.number = [appDelegate.settings.countrycode stringByAppendingString:self.number];
		}else {
			//use default +31
			self.number = [@"+31" stringByAppendingString:self.number];
		}

	}

	self.naptrList = [dns doEnumQuery:self.number];
	
	for(RecordNaptr *rec in self.naptrList){
		NSLog(@"naptr record: %@ value = %@", rec.serviceDescription, rec.uriContent );
	}
}

-(void)initProfile{
	NSLog(@"initProfile");
	self.enumServices = [NSMutableArray array];
}


-(void)processDnsRecords{
	for (RecordNaptr *rec in naptrList) {
		NSLog(@"service %@, met value %@", rec.serviceDescription, rec.uriContent);
		//check if the thread doing the processing has been cancelled
		if([self.thread isCancelled]){
			NSLog(@"thread cancelled by timer");
			return;
		}
		
		if ([[rec serviceDescription] rangeOfString:@"vcard:"].location != NSNotFound) {
			//detected vcard record
			if(rec.uriContent != nil){
				self.vcard = [[Vcard alloc] initWithUrl:rec.uriContent];
				servicecount++;
			}
		}else if( [[rec serviceDescription] isEqual:@"web:http" ]){
			//web location found
			if(rec.uriContent != nil){
				EnumService *es = [EnumServiceManager createEnumServiceWithUrl:rec.uriContent andType:SERVICE_WEB];
				if(es){
					[self.enumServices addObject:es];
					servicecount++;
				}
				
				
			}
		}else if( [[rec serviceDescription] isEqual:@"key:http" ]){
			//public key found
			EnumService *es = [EnumServiceManager createEnumServiceWithUrl:rec.uriContent andType:SERVICE_KEY];
			if(es){
				[self.enumServices addObject:es];
				servicecount++;
			}
		}else if( [[rec serviceDescription] isEqual:@"loc:geo" ]){
			//map location found
			EnumService *es = [EnumServiceManager createEnumServiceWithUrl:rec.uriContent andType:SERVICE_LOC];
			if(es){
				[self.enumServices addObject:es];
				servicecount++;
			}
		}else if( [[rec serviceDescription] isEqual:@"Email" ]){
			//map location found
			EnumService *es = [EnumServiceManager createEnumServiceWithUrl:rec.uriContent andType:SERVICE_MAIL];
			if(es){
				[self.enumServices addObject:es];
				servicecount++;
			}
		}else if( [[rec serviceDescription] isEqual:@"voice" ]){
			//map location found
			EnumService *es = [EnumServiceManager createEnumServiceWithUrl:rec.uriContent andType:SERVICE_VOICE];
			if(es){
				[self.enumServices addObject:es];
				servicecount++;
			}
		}
		
	}
}
			 
			 
-(BOOL)isLocationAvailable{
	for (int count = 0; count < [enumServices count]; count++) {
		EnumService *es = (EnumService *) [enumServices objectAtIndex:count]; 
		if ([es class] == [LocationService class]) {
			//found location
			return YES;
		}
	}
	
	//no location found
	return NO;
}

-(BOOL)isKeyAvailable{
	for (int count = 0; count < [enumServices count]; count++) {
		EnumService *es = (EnumService *) [enumServices objectAtIndex:count]; 
		if ([es getType] == SERVICE_KEY ) {
			//found key
			return YES;
		}
	}
	
	//no key found
	return NO;
}
			 
			 

@end
