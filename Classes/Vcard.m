//
//  Vcard.m
//  iEnum
//
//  Created by Maarten Wullink on 06-12-10.
//  Copyright 2010 SIDN. All rights reserved.
//

#import "Vcard.h"


@implementation Vcard


@synthesize fullname;
@synthesize poBox;
@synthesize extAddress;
@synthesize street;
@synthesize city;
@synthesize region;
@synthesize state;
@synthesize postalCode;
@synthesize country;
@synthesize photo;
@synthesize url;
@synthesize raw;
@synthesize status;


-(id)initWithUrl:(NSString *)pUrl{
	self = [super init];
	if(self){
		[self loadVcardWithUrlPath:pUrl];
	}

	return self;
}

-(void)loadVcardWithUrlPath:(NSString *)urlPath{
	//download the raw vcard data
	//NSData *theData = [NSData dataWithContentsOfURL:[NSURL URLWithString:urlPath]];
	
	// Create the URL request
	NSMutableURLRequest *request = [[NSMutableURLRequest alloc] 
									initWithURL:[NSURL URLWithString:urlPath]
									cachePolicy:NSURLRequestUseProtocolCachePolicy
									timeoutInterval:5];
	
	
	NSError *err;
	NSURLResponse *resp;
	NSData* theData = [NSURLConnection sendSynchronousRequest: request
							  returningResponse: &resp error: &err];
	
	
	/* Populate the request, this part works fine */
	
	[NSURLConnection connectionWithRequest:request delegate:self];
	
	if (resp == nil) {
		NSLog(@"ERROR while downloading: %@", urlPath);
		return;
	}
	
	
	raw = [[[NSString alloc] initWithData:theData encoding:NSUTF8StringEncoding] autorelease];
	
	//split the data into an array
	NSArray *chunks = [raw componentsSeparatedByString:@"\n"];

	//process each line
	[self processVcardElements:chunks];
	
}

-(void)processVcardElements:(NSArray *)elements{
	int elementCount = [elements count];
	for (int i=0; i<[elements count]; i++) {
		NSString *element = (NSString *) [elements objectAtIndex:i];
	    //the first line of a vcard must begin with BEGIN:VCARD
		//if this is not the case then this is not a correct vcard and will be ignored.
		if (i == 0) {
			//check begin
			if ([element rangeOfString:@"BEGIN:VCARD" options:NSCaseInsensitiveSearch].location == NSNotFound ) {
				//not a valid card, quit
				NSLog(@"card did not start with BEGIN:VCARD");
				self.status = NO;
				return;
			}
		}
		
		//process all the lines, but only use the lines we are interested in
		if([element rangeOfString:@"FN:" options:NSCaseInsensitiveSearch].location == 0){
			//fullname found
			self.fullname = [element substringWithRange:NSMakeRange(3,[element length]-3)];
			
		}else if([element rangeOfString:@"PHOTO:" options:NSCaseInsensitiveSearch].location == 0){
			NSString *photoUrl = [element substringWithRange:NSMakeRange(6,[element length]-7)];
			[self loadPhotoWithUrlPath:photoUrl];	
			
		}else if([element rangeOfString:@"ADR:" options:NSCaseInsensitiveSearch].location == 0){
	       //fullname found
		   NSArray *chunks = [element componentsSeparatedByString:@";"];

		   int element = 0;
		   for (int i = [chunks count]-1; i >= 0; i--) {
			   if(element == 0){
				   //country
				   self.country = [chunks objectAtIndex:i];
			   }else if (element == 1) {
				   //postal code
				   self.postalCode = [chunks objectAtIndex:i];
			   }else if (element == 2) {
				   //region
				   self.region = [chunks objectAtIndex:i];
			   }else if (element == 3) {
				   //rcity
				   self.city = [chunks objectAtIndex:i];
			   }else if (element == 4) {
				   //street
				   self.street = [chunks objectAtIndex:i];
			   }else if (element == 5) {
				   //ext address
				   self.extAddress = [chunks objectAtIndex:i];
			   }else if (element == 6) {
				   //pobox
				   self.poBox = [chunks objectAtIndex:i];
			   }
			   element++;
		   }

		}
		
		if (i == elementCount-1) {
			//check if the vcard ends with END:VCARD
			if ([element rangeOfString:@"END:VCARD" options:NSCaseInsensitiveSearch].location == NSNotFound ) {
				//not a valid card, quit
				NSLog(@"card did not end with END:VCARD, value =%@", element);
			}
		}
	}
	
	//vcard parsing ok
	self.status = YES;
	
}


-(void)loadPhotoWithUrlPath:(NSString *)urlPath{
	NSLog(@"loadPhotoWithUrlPath %@", urlPath);
	
	//check if url starts with http:// or https://
	if(! ([urlPath rangeOfString:@"http://"].location == 0) &&
	   ! ([urlPath rangeOfString:@"https://"].location == 0)){
		//no http or https prefox, use default http
		urlPath = [@"http://" stringByAppendingString:urlPath];
	}
	
	//NSData *theData = [NSData dataWithContentsOfURL:[NSURL URLWithString:urlPath]];
	
	NSMutableURLRequest *request = [[NSMutableURLRequest alloc] 
									initWithURL:[NSURL URLWithString:urlPath]
									cachePolicy:NSURLRequestUseProtocolCachePolicy
									timeoutInterval:8];
	
	
	NSError *err;
	NSURLResponse *resp;
	NSData* theData = [NSURLConnection sendSynchronousRequest: request
											returningResponse: &resp error: &err];
	
	
	/* Populate the request, this part works fine */
	
	[NSURLConnection connectionWithRequest:request delegate:self];
	
	if (resp != nil) {
	    self.photo = [[UIImage alloc] initWithData:theData];
	}
	
	
	
}

@end
