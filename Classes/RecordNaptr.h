//
//  RecordNaptr.h
//  DotTel SDK
//
//  Created by Henri Asseily on 9/12/08.
//  Copyright 2008 Telnic Ltd. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "ldns.h"

// Shorthand for getting localized strings
#define LocTelStr(key) [[NSBundle mainBundle] localizedStringForKey:(key) value:@"" table:@"DotTel"]

@interface RecordNaptr : NSObject {

	// All string tokens in a NAPTR
	NSNumber *order;
	NSNumber *preference;
	NSString *flags;
	NSString *services;
	NSString *regexp;
	NSString *replacement;
	
	// Parsed services string for .tel-specific tokens
	NSMutableArray *serviceTypeArray;	// e.g. "voice:tel", "web:http"...
	NSMutableArray *labelArray;			// x-lbl values
	NSMutableArray *lihArray;			// Location Indicator Hints (x-mobile, x-work, ...)
	
	// Useful properties of a NAPTR
	BOOL isTerminal;
	BOOL isPrivate;
	BOOL isEncrypted;
	BOOL isHidden;
	BOOL isValid;

	// Derived attributes
	NSString *serviceDescription;		// descriptive services token element + LIH
	NSString *labelDescription;			// descriptive label
	NSString *uriContent;				// value of the NAPTR after regexp has been applied
	NSDate *expiryDate;					// Expiry date of the record, based on time-to-live
	
}

/**
 * Class methods to initialize a NAPTR record from a ldns resource record
 * This method calls recordWithRr:date: using the current date and time
 * 
 * @return     the record
 */
+ (id)recordWithRr:(ldns_rr *)rr;

/**
 * Class methods to initialize a NAPTR record from a ldns resource record
 * Sets property isValid=NO on error or if the provided ldns rr is not a NAPTR.
 * 
 * @return     the record
 */
+ (id)recordWithRr:(ldns_rr *)rr date:(NSDate *)lookupDate;

/**
 * Initializer for a NAPTR record from a ldns resource record
 * Sets property isValid=NO on error or if the provided ldns rr is not a NAPTR.
 * You shouldn't need to call this directly. Instead call one of the class methods
 * recordWithRr.
 * 
 * @return     the record
 */
- (id)initWithRr:(ldns_rr *)rr date:(NSDate *)lookupDate;

/**
 * Comparison selector for ordering NAPTR records. Use this to properly sort
 * an array of RecordNaptr. Example usage:
 * [naptrArray sortUsingSelector:@selector(comparator:)];
 *
 * @return     an NSComparisonResult
 */
- (NSComparisonResult)comparator:(RecordNaptr *)aNaptr;

@property (readonly) BOOL isTerminal;
@property (readonly) BOOL isPrivate;
@property (readonly) BOOL isEncrypted;
@property (readonly) BOOL isHidden;
@property (readonly) BOOL isValid;

@property (readonly, nonatomic, retain) NSNumber *order; 
@property (readonly, nonatomic, retain) NSNumber *preference; 
@property (readonly, nonatomic, retain) NSString *serviceDescription;
@property (readonly, nonatomic, retain) NSString *labelDescription;
@property (readonly, nonatomic, retain) NSString *uriContent;
@property (readonly, nonatomic, retain) NSDate *expiryDate;

@end
