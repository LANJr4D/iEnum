//
//  ContactProfile.h
//  iEnum
//
//  Created by Maarten Wullink on 06-12-10.
//  Copyright 2010 SIDN. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "RecordNaptr.h"
#import "Vcard.h"
#import "EnumServiceManager.h"
#import "DnsResolver.h"
#import "EnumService.h"
#import "AppDelegate.h"


@protocol ProfileDelegate <NSObject>
@optional
- (void) profileProcessed:(NSNumber *)count;
@end




@interface ContactProfile : NSObject {

	NSMutableArray *enumServices;
	NSString *publicKey;
	UIImage *image;
	Vcard *vcard;
	NSArray *naptrList;
	NSObject<ProfileDelegate> *delegate;
	NSString *number;
	NSTimer *timer;
	NSThread *thread;
	NSString *status;

}

@property(nonatomic, retain) NSMutableArray *enumServices;
@property(nonatomic, retain) NSString *publicKey;
@property(nonatomic, retain) UIImage *image;
@property(nonatomic, retain) Vcard *vcard;
@property(nonatomic, retain) NSArray *naptrList;
@property(nonatomic, retain) NSObject<ProfileDelegate> *delegate;
@property(nonatomic, retain) NSString *number;
@property(assign) NSTimer *timer;
@property(assign) NSThread *thread;
@property(nonatomic, retain) NSString *status;


-(void)loadProfile;

//private

-(id)initWithNumber:(NSString *)aNumber;
-(void)processDnsRecords;
-(void)doQuery;
-(void)initProfile;
-(BOOL)isLocationAvailable;
-(BOOL)isKeyAvailable;


@end
