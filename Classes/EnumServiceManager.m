//
//  EnumService.m
//  iEnum
//
//  Created by Maarten Wullink on 02-01-11.
//  Copyright 2011 SIDN. All rights reserved.
//

#import "EnumServiceManager.h"


@implementation EnumServiceManager


+(EnumService *)createEnumServiceWithUrl:(NSString *)url andType:(int) type{
	EnumService *es = nil;
	
	if (type == SERVICE_WEB) {
		return [self getEnumSericeForWeb:url];
	}else if (type == SERVICE_LOC) {
		es = [[[LocationService alloc] initWithString:url] autorelease];
		es.icon = [UIImage imageNamed:@"map-32.png"];
		[es setType:type];
		return es;
		
	}else if (type == SERVICE_KEY) {
		es = [[[EnumService alloc] init] autorelease];
		es.icon = [UIImage imageNamed:@"secure-email.png"];
		es.url = url;
		[es setType:type];
		return es;
	}else if (type == SERVICE_MAIL) {
		return [self getEnumSericeForMailWithUrl:url andType:type];
	}else if (type == SERVICE_VOICE) {
		return [self getEnumSericeForVoiceWithUrl:url andType:type];
	}
	
	//no image found means the site is not supported, return nil
	NSLog(@"service %@ is not supported", url);
	return nil;
	
}

+(EnumService *)getEnumSericeForMailWithUrl:(NSString *)url andType:(int) type{

	EnumService *es = [[[EnumService alloc] init] autorelease];
	
	NSArray *chunks = [url componentsSeparatedByString:@":"];
	if (chunks && [chunks count] == 2) {
		//get the phone uri
		es.url = [chunks objectAtIndex:1];
	}
	

	es.icon = [UIImage imageNamed:@"plain-email.png"];
	//es.url = url;
	es.title = @"email";
	[es setType:type];
	
	return es;
}

+(EnumService *)getEnumSericeForVoiceWithUrl:(NSString *)url andType:(int) type{
	//split tel:+316666
	EnumService *es = [[[EnumService alloc] init] autorelease];
	
	NSArray *chunks = [url componentsSeparatedByString:@":"];
	if (chunks && [chunks count] == 2) {
		//get the phone uri
		es.url = [chunks objectAtIndex:1];
	}
	
	es.icon = [UIImage imageNamed:@"iphone-icon-32.png"];
	
	es.title = @"voice";
	[es setType:type];
	
	
	return es;

}


+(EnumService *)getEnumSericeForWeb:(NSString *)url{
	EnumService *es = [[[EnumService alloc] init] autorelease];
	
	if ([url rangeOfString:@"linkedin.com"].location != NSNotFound) {
		//linkedin site found
		es.icon = [UIImage imageNamed:@"linkedin.png"];
		es.title = @"LinkedIn";
	}else if ([url rangeOfString:@"facebook.com"].location != NSNotFound) {
		//facebook site found
		es.icon = [UIImage imageNamed:@"facebook.png"];
		es.title = @"Facebook";
	}else if ([url rangeOfString:@"hyves.nl"].location != NSNotFound) {
		//hyves site found
		es.icon = [UIImage imageNamed:@"hyves.png"];
		es.title = @"Hyves";
	}else if ([url rangeOfString:@"twitter.com"].location != NSNotFound) {
		//twitter site found
		es.icon = [UIImage imageNamed:@"twitter.png"];
		es.title = @"Twitter";
	}else if ([url rangeOfString:@"foursquare.com"].location != NSNotFound) {
		//foursquare site found
		es.icon = [UIImage imageNamed:@"foursquare.png"];
		es.title = @"Foursquare";
	}else if ([url rangeOfString:@"youtube.com"].location != NSNotFound) {
		//youtube site found
		es.icon = [UIImage imageNamed:@"youtube.png"];
		es.title = @"Youtube";
	}else if ([url rangeOfString:@"skype.com"].location != NSNotFound) {
		//skype site found
		es.icon = [UIImage imageNamed:@"skype.png"];
		es.title = @"Skype";
	}else if ([url rangeOfString:@"myspace.com"].location != NSNotFound) {
		//myspace site found
		es.icon = [UIImage imageNamed:@"myspace.png"];
		es.title = @"Myspace";
	}else if ([url rangeOfString:@"flickr.com"].location != NSNotFound) {
		//flickr site found
		es.icon = [UIImage imageNamed:@"flickr.png"];
		es.title = @"Flickr";
	}else if ([url rangeOfString:@"myopenid.com"].location != NSNotFound) {
		//myopenid site found
		es.icon = [UIImage imageNamed:@"openid-32.png"];
		es.title = @"myOpenId";
	}

	if (es.icon) {
		//found a supported website
		es.url = url;
		[es setType:SERVICE_WEB];
		
		NSLog(@"found web service: %@", es.title);
		
		return es;
	}
	
	
	
	return nil;

}

@end
