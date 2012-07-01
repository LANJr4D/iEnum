//
//  Vcard.h
//  iEnum
//
//  Created by Maarten Wullink on 06-12-10.
//  Copyright 2010 SIDN. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface Vcard : NSObject {
	NSString *fullname;
	NSString *poBox;
	NSString *extAddress;
	NSString *street;
	NSString *city;
	NSString *region;
	NSString *postalCode;
	NSString *country;
	UIImage *photo;
	NSString *url;
	NSString *raw;
	
	BOOL status;
}

/*
 
 the vcard address consists out of
 
 post office box; the extended address; the street
 address; the locality (e.g., city); the region (e.g., state or
 province); the postal code; the country name.
 
 see: http://www.rfc-editor.org/info/rfc2425
 and http://www.rfc-editor.org/info/rfc2426
 
 */

@property (nonatomic, retain ) NSString *fullname;
@property (nonatomic, retain ) NSString *poBox;
@property (nonatomic, retain ) NSString *extAddress;
@property (nonatomic, retain ) NSString *street;
@property (nonatomic, retain ) NSString *city;
@property (nonatomic, retain ) NSString *region;
@property (nonatomic, retain ) NSString *state;
@property (nonatomic, retain ) NSString *postalCode;
@property (nonatomic, retain ) NSString *country;

@property (nonatomic, retain ) UIImage *photo;
@property (nonatomic, retain ) NSString *url;
@property (nonatomic, retain ) NSString *raw;
@property (nonatomic, readwrite ) BOOL status;



-(id)initWithUrl:(NSString *)url;
-(void)loadVcardWithUrlPath:(NSString *)urlPath;
-(void)loadPhotoWithUrlPath:(NSString *)urlPath;
-(void)processVcardElements:(NSArray *)elements;


@end
