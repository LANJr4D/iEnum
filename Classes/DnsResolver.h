//
//  DnsResolver.h
//  DotTel SDK
//
//  Created by Henri Asseily on 9/14/08.
//  Copyright 2008 Telnic Ltd. All rights reserved.
//
// 
//  This class is the entry point for handling the .tel DNS record classes
//  (RecordNatpr, RecordTxt and RecordLoc).
//  Simply allocate and initialize an instance of DnsResolver, and
//  call the get[Type]ForTel methods to populate your arrays.
//  Then iterate through those arrays, and ensure you check the isValid property
//  of each record.
//  Note that getNAPTRForTel, getTXTForTel and getLocForTel have different return
//  value types and are thus used in slightly different manners.

#import <UIKit/UIKit.h>
#import "ldns.h"
#import "RecordNaptr.h"
#import <SystemConfiguration/SCNetworkReachability.h>
#include <netinet/in.h>


// Shorthand for getting localized strings
#define LocTelStr(key) [[NSBundle mainBundle] localizedStringForKey:(key) value:@"" table:@"DotTel"]



extern NSString * const ENUM_E164_SUFFIX;

@interface DnsResolver : NSObject {

	ldns_resolver *res;
	NSString *suffix;
}

@property(nonatomic, retain)NSString *suffix;

/**
 * Retrieves all NAPTR records in a domain and adds them to the provided
 * array. NAPTR records are encapsulated in the RecordNaptr class.
 * 
 * @param domain       the domain to query
 * @param naptrArray   the array to add the naptrs to
 *
 */
- (void)getNAPTRList:(NSString *)domain inArray:(NSMutableArray *)naptrArray;


/**
 * Converts an input value (in Application Unique String format) into the
 * database key (in this case a domain name).
 * 
 * The ENUM key format is the AUS (with the leading '+' removed) in reverse
 * order, with each number treated as a separate label.  The current suffix
 * is then appended, eg.:
 * 
 * +31123456789</code> becomes
 *   <code>9.8.7.6.5.4.3.2.1.1.3.e164.arpa.
 * 
 * @param key The Application Unique String
 * @return    The AUS converted to ENUM database format (e.g. a domain name)
 * 
 */
- (NSString *)convertPhone2Enum:(NSString *)key; 

/**
 * Perform a enum query for a phonenumber
 * @return a array with the enum records for the phonenumber
 */
-(NSArray *)doEnumQuery:(NSString *)forNumber;

/**
 * Check if there is a network connection available
 */
+ (BOOL) connectedToNetwork;

@end
