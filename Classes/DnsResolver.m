//
//  DnsResolver.m
//  DotTel SDK
//
//  Created by Henri Asseily on 9/14/08.
//  Copyright 2008 Telnic Ltd. All rights reserved.
//

#import "DnsResolver.h"

NSString * const ENUM_E164_SUFFIX =  @"e164.arpa";

@interface DnsResolver (PrivateMethods)

- (ldns_resolver *)createLdnsResolver;
- (ldns_rr_list *)retrieveResourceRecordsOfType:(ldns_rr_type)rrType fromDomain:(NSString *)domain;

@end

@implementation DnsResolver

@synthesize suffix;

- (id)init {
	self = [super init];
	res = [self createLdnsResolver];
	return self;
}

- (void)dealloc {
	ldns_resolver_deep_free(res);
	[super dealloc];
}


- (void)getNAPTRList:(NSString *)domain inArray:(NSMutableArray *)naptrArray {
	ldns_rr_list *naptrs = [self retrieveResourceRecordsOfType:LDNS_RR_TYPE_NAPTR fromDomain:domain];
	if (!naptrs) {
		return;
	}
	
	NSDate *lookupDate = [[NSDate date] retain];
	NSUInteger i, count = ldns_rr_list_rr_count(naptrs);
	
	for (i = 0; i < count; i++) {
		RecordNaptr *theRec = [RecordNaptr recordWithRr:ldns_rr_list_rr(naptrs, i) date:lookupDate];
		if (theRec.isValid)
			[naptrArray addObject:theRec];
	}
	
	[naptrArray sortUsingSelector:@selector(comparator:)];
		
	ldns_rr_list_deep_free(naptrs);
	[lookupDate release];
	
	return;
	
}


#pragma mark ------------ Enum number conversion methods ----------


- (NSString *)convertPhone2Enum:(NSString *)aus{
	
	NSMutableString *key = [NSMutableString stringWithCapacity:15];
	int len = [aus length];
	
	for (int pos = len-1; pos >= 0; pos--) {
		
		NSString *element = [NSString stringWithFormat:@"%C",[aus characterAtIndex:pos]];
		[key appendString:element];
		[key appendString:@"."];
	}
	if (self.suffix) {
		[key appendString:self.suffix];
	}else {
		[key appendString:ENUM_E164_SUFFIX];
	}

	
	return key;
}



-(NSArray *)doEnumQuery:(NSString *)forNumber{
	NSString *cleanNumber = [[forNumber componentsSeparatedByCharactersInSet:[[NSCharacterSet characterSetWithCharactersInString:@"0123456789"] invertedSet]] componentsJoinedByString:@""];
	NSLog(@"doEnumQuery:cleanNumber %@", cleanNumber);
	NSString *e164number = [self convertPhone2Enum:cleanNumber];
	NSMutableArray *results = [NSMutableArray arrayWithCapacity:15];
	[self getNAPTRList:e164number inArray:results];
		
	return results;
}

#pragma mark ------------ private methods -------------------


+ (BOOL) connectedToNetwork{
    // Create zero addy
    struct sockaddr_in zeroAddress;
    bzero(&zeroAddress, sizeof(zeroAddress));
    zeroAddress.sin_len = sizeof(zeroAddress);
    zeroAddress.sin_family = AF_INET;
	
    // Recover reachability flags
    SCNetworkReachabilityRef defaultRouteReachability = SCNetworkReachabilityCreateWithAddress(NULL, (struct sockaddr *)&zeroAddress);
    SCNetworkReachabilityFlags flags;
	
    BOOL didRetrieveFlags = SCNetworkReachabilityGetFlags(defaultRouteReachability, &flags);
    CFRelease(defaultRouteReachability);
	
    if (!didRetrieveFlags)
    {
        printf("Error. Could not recover network reachability flags\n");
        return 0;
    }
	
    BOOL isReachable = flags & kSCNetworkFlagsReachable;
    BOOL needsConnection = flags & kSCNetworkFlagsConnectionRequired;
    return (isReachable && !needsConnection) ? YES : NO;
}

- (ldns_resolver *)createLdnsResolver {
	ldns_resolver *resolver;
	ldns_pkt *p;
	ldns_status s;
	
	p = NULL;
	resolver = NULL;
	
	
	//check if the hosts.conf exists
	
	NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
	NSString *documentsDirectory = [paths objectAtIndex:0];
	//check if the appHosts.conf file exists, if so use it, otherwise use default resolv.conf
	//with the default google nameservers
	NSString *resolverFilePath = [NSString stringWithFormat:@"%@/appHosts.conf", documentsDirectory];
	if (![[NSFileManager defaultManager] fileExistsAtPath:resolverFilePath]) {
		//use the default google nameservers
		resolverFilePath = [[NSBundle mainBundle] pathForResource:@"resolv" ofType:@"conf"];
	}

	/* create a new resolver from resolv.conf */
	/* on the iphone we may not have a resolv.conf so we provide one */

	s = ldns_resolver_new_frm_file(&resolver, [resolverFilePath cStringUsingEncoding:NSASCIIStringEncoding]);
	return resolver;
}

- (ldns_rr_list *)retrieveResourceRecordsOfType:(ldns_rr_type)rrType fromDomain:(NSString *)domain {
	
	ldns_rr_list *rrlist;
	
	ldns_rdf *ldnsdomain = ldns_dname_new_frm_str([domain UTF8String]);
	
	if (!ldnsdomain) {
		return rrlist;
	}
	
	ldns_pkt *p;
	
	p = ldns_resolver_query(res,
							ldnsdomain,
							rrType,
							LDNS_RR_CLASS_IN,
							LDNS_RD);
	
	
	if (!p)  {
		return rrlist;
	}
	
	/* retrieve the records from the answer section of that
	 * packet
	 */
	rrlist = ldns_pkt_rr_list_by_type(p,
									  rrType,
									  LDNS_SECTION_ANSWER);
	if (!rrlist) {
		ldns_pkt_free(p);
		return rrlist;
	}
	
	ldns_pkt_free(p);
	return rrlist;
}

@end
