//
//  CryptoService.m
//  iEnum
//
//  Created by Maarten Wullink on 05-01-11.
//  Copyright 2011 SIDN. All rights reserved.
//

#import "CryptoService.h"


@implementation CryptoService

@synthesize keyUrl;
@synthesize status;
@synthesize timer;
@synthesize thread;
@synthesize delegate;
@synthesize certificate;
@synthesize plaintext;
@synthesize ciphertext;
@synthesize publickey;


-(id)initWithUrl:(NSString *)aUrl{
	self = [super init];
	if(self){
		self.keyUrl = aUrl;
	}
	
	return self;
}


-(void)startCryptoWithPlaintext:(NSString *)aPlaintext{
	//set a 10 second timeout value
	self.plaintext = aPlaintext;
	self.timer = [NSTimer scheduledTimerWithTimeInterval:10 target:self selector:@selector(timeout) userInfo:nil repeats:YES];
	self.thread = [[NSThread alloc] initWithTarget:self selector:@selector(start) object:nil];
	[self.thread start];
}


-(void)timeout{
	//creating profile failed to complete on time
	[self cancel];
	self.status = @"ERROR";
	[delegate cryptoReady:nil];
	
}

-(void)cancel{
	//creating profile failed to complete on time
	[self.timer invalidate];
	[self.thread cancel];
}

-(void)start{
	//create release pool for use in the thread
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	//do the work
	
	if ([self loadCertificate]) {
		NSData *cipher = [self encryptString:self.plaintext];
		self.ciphertext = [cipher base64EncodedString];
		
		//stop the timeout timer
		[timer invalidate];
		
		//signal caller loading is done
		self.status = @"OK";
		[self.delegate performSelectorOnMainThread:@selector(cryptoReady:) withObject:self.ciphertext waitUntilDone:true];
	}else {
		//cannot do encryption, key loading failed
		[self cancel];
		self.status = @"Failed";
		[self.delegate performSelectorOnMainThread:@selector(cryptoFailed) withObject:nil waitUntilDone:true];
	}

	
	
	
	//release the pool
	[pool release];
}

-(BOOL)loadCertificate{
	//load the file from url 

	if(! ([self.keyUrl rangeOfString:@"http://"].location == 0) &&
	   ! ([self.keyUrl rangeOfString:@"https://"].location == 0)){
		//no http or https prefix, use default http
		   self.keyUrl = [@"http://" stringByAppendingString:self.keyUrl];
	}
	self.certificate = [NSData dataWithContentsOfURL:[NSURL URLWithString:self.keyUrl]];
	if (self.certificate == nil) {
		//loading the certificate went wrong, exit
		return NO;
	}
	//convert the data to a SecCertificate
	SecCertificateRef cert =  [self createCertificate];
	if(!cert){
		//certificate could not be loaded
		return NO;
	}   
	   
	CFArrayRef certs = CFArrayCreate(kCFAllocatorDefault, (const void **) &cert, 1, NULL); 
	SecTrustRef trust;
	SecPolicyRef policy = SecPolicyCreateBasicX509();
	SecTrustCreateWithCertificates(certs, policy, &trust);
	SecTrustResultType trustResult;
	SecTrustEvaluate(trust, &trustResult);
	self.publickey = SecTrustCopyPublicKey(trust);
	return YES;
}

-(SecCertificateRef)createCertificate{
	NSString* certText = [[NSString alloc] initWithData:self.certificate encoding:NSASCIIStringEncoding];
	if ([certText rangeOfString:@"BEGIN"].location == NSNotFound) {
		//did not find the "-----BEGIN CERTIFICATE-----" header, so it must be a DER encoded x509 cert
		return SecCertificateCreateWithData(NULL, (CFDataRef) self.certificate);
	}
	//it must be a base64 encode x509
	
	//strip the whitespace and comments
	NSString *cleanKey = @"";
	NSArray *chunks = [certText componentsSeparatedByString:@"\n"];
	for (int i=0; i<[chunks count]; i++) {
		NSString *chunk = [chunks objectAtIndex:i];
	
		//ignore prefix and postfix and the version and comment lines
		if( ([chunk rangeOfString:@"BEGIN"].location == NSNotFound) &&
			([chunk rangeOfString:@"END"].location == NSNotFound)){
		
			//certifcate line, clean whitespaces en newlines
			NSString *line = [chunk stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceAndNewlineCharacterSet]];
			//check if line is not empty
			if ([line length] > 0) {
				cleanKey = [NSString stringWithFormat:@"%@%@\n", cleanKey, line];
			}
			
			
		}
	}
	//decode the base64 string to a nsdata object containing the key in DER encoding
	
	NSData *certData = [NSData dataFromBase64String:cleanKey];
	return SecCertificateCreateWithData(NULL, (CFDataRef) certData);
	
	
}

- (NSData *)encryptString:(NSString *)aPlaintext{
	NSData* plainData = [plaintext dataUsingEncoding: NSASCIIStringEncoding];
	
	//create variables
	///encryption status
	OSStatus sanityCheck = noErr;
	//size of the buffer required for the public key
	size_t keyBufferSize = 0;
	//buffer required for the encrypted result.
	uint8_t * cipherBuffer = NULL;
	//calculate th size of the buffer rquired fort the public key	
	size_t cipherBufferSize = SecKeyGetBlockSize(self.publickey);
	//calculate the size of the plaintext
	keyBufferSize = [aPlaintext length];
	cipherBuffer = malloc( cipherBufferSize * sizeof(uint8_t) );
	//reset the reserved memory for the encrypted text
	memset((void *)cipherBuffer, 0x0, cipherBufferSize);
	
	//encrypt the plaintext data
	sanityCheck = SecKeyEncrypt(self.publickey, kSecPaddingPKCS1, (const uint8_t *)[plainData bytes], keyBufferSize,
								cipherBuffer, &cipherBufferSize);
	
	
	
//	if(sanityCheck == errSecSuccess){
//		NSLog(@"encrypting worked!!, OSStatus == %d.", sanityCheck );
//	}else {
//		NSLog(@"Error encrypting, OSStatus == %d.", sanityCheck );
//	}
	
	NSData *cipher = [NSData dataWithBytes:(const void *)cipherBuffer length:(NSUInteger)cipherBufferSize];
	//check if encrypted text is present, is so the release the buffer
	if (cipherBuffer){
		free(cipherBuffer);	
	}
	
	return cipher;
}



@end
