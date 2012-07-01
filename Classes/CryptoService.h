//
//  CryptoService.h
//  iEnum
//
//  Created by Maarten Wullink on 05-01-11.
//  Copyright 2011 SIDN. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "NSData+Base64.h"

@protocol CryptoDelegate<NSObject>
@optional
- (void)cryptoReady:(NSString*)ciphertext;
@end

@interface CryptoService : NSObject {
	
	NSString *keyUrl;
	NSString *status;
	NSTimer *timer;
	NSThread *thread;
	NSObject<CryptoDelegate> *delegate;
	NSData *certificate;
	NSString *plaintext;
	NSString *ciphertext;
	SecKeyRef publickey;

}

@property(nonatomic, retain) NSString *keyUrl;
@property(assign) NSTimer *timer;
@property(assign) NSThread *thread;
@property(nonatomic, retain) NSString *status;
@property(nonatomic, retain) NSObject<CryptoDelegate> *delegate;
@property(nonatomic, retain) NSData *certificate;
@property(nonatomic, retain) NSString *plaintext;
@property(nonatomic, retain) NSString *ciphertext;
@property(nonatomic, assign) SecKeyRef publickey;

-(BOOL)loadCertificate;
- (NSData *)encryptString:(NSString *)plaintext;
-(void)startCryptoWithPlaintext:(NSString *)aPlaintext;
-(id)initWithUrl:(NSString *)aUrl;
-(SecCertificateRef)createCertificate;
-(void)cancel;


@end
