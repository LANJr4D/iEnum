//
//  RecordNaptr.m
//  DotTel SDK
//
//  Created by Henri Asseily on 9/12/08.
//  Copyright 2008 Telnic Ltd. All rights reserved.
//

#import "RecordNaptr.h"
@interface RecordNaptr (PrivateMethods)

- (void)parseServices:(NSString *)services;
- (BOOL)tokenIsLih:(NSString *)token;
- (NSString *)generateServiceDescription;
- (NSString *)generateLabelDescription;

- (NSString *)stringFromStringRdf:(const ldns_rdf *)rdf;
- (NSString *)stringFromDnameRdf:(const ldns_rdf *)rdf;
- (NSNumber *)numberFromIntRdf:(const ldns_rdf *)rdf;

@end


@implementation RecordNaptr

@synthesize order;
@synthesize preference;
@synthesize serviceDescription;
@synthesize labelDescription;
@synthesize uriContent;
@synthesize isTerminal;
@synthesize isPrivate;
@synthesize isEncrypted;
@synthesize isHidden;
@synthesize isValid;
@synthesize expiryDate;

+ (id)recordWithRr:(ldns_rr *)rr {
	NSDate *lookupDate = [[NSDate date] retain];
	self = [RecordNaptr recordWithRr:rr date:lookupDate];
	[lookupDate release];
	return self;
}

+ (id)recordWithRr:(ldns_rr *)rr date:(NSDate *)lookupDate {
	self = [[[RecordNaptr alloc] initWithRr:rr date:lookupDate] autorelease];
	return self;
}

- (id)init {
	// Never initialize it empty
	self = [super init];
	isValid = NO;
	isHidden = NO;
	return self;
}

- (id)initWithRr:(ldns_rr *)rr date:(NSDate *)lookupDate {
	self = [super init];
	isValid = NO;
	
	if (ldns_rr_get_type(rr) != LDNS_RR_TYPE_NAPTR)
		return self;
	
	NSTimeInterval ttl = (NSTimeInterval)ldns_rr_ttl(rr);
	expiryDate = [lookupDate addTimeInterval:ttl];
	
	serviceTypeArray = [NSMutableArray arrayWithCapacity:2];
	labelArray = [NSMutableArray arrayWithCapacity:2];
	lihArray = [NSMutableArray arrayWithCapacity:2];
	
	order = [[self numberFromIntRdf:ldns_rr_rdf(rr,0)] retain];
	preference = [[self numberFromIntRdf:ldns_rr_rdf(rr,1)] retain];
	flags = [[self stringFromStringRdf:ldns_rr_rdf(rr,2)] retain];
	services = [[self stringFromStringRdf:ldns_rr_rdf(rr,3)] retain];
	regexp = [[self stringFromStringRdf:ldns_rr_rdf(rr,4)] retain];
	
	if ([services isEqualToString:@"x-crypto:data:8210"]) {
		isPrivate = YES;
		isEncrypted = YES;
		
		//TODO: Try to decrypt the NAPTR record
		// if successful, set isEncrypted to NO;
		
	} else { // Not encrypted
		isPrivate = NO;
		isEncrypted = NO;
	}
	
	if ([flags isEqualToString:@"u"]) { // terminal naptr
		isTerminal = YES;
		replacement = @"";
		[self parseServices:services];

		if (isEncrypted) {
			uriContent = @"";
		} else {
			// Clean Regexp (don't bother with regex, just assume it's a full overwrite)
			NSArray *regexParts = [regexp componentsSeparatedByString:@"!"];
			uriContent = [[NSString stringWithString:[regexParts objectAtIndex:2]] retain];
		}
		
	} else if ([flags isEqualToString:@""]) { // Non-terminal naptr
		isTerminal = NO;
		replacement = [self stringFromDnameRdf:ldns_rr_rdf(rr,5)];
		uriContent = [replacement retain];
		
	} else { // Not valid
		return self;
	}
	
	serviceDescription = [[self generateServiceDescription] retain];
	labelDescription = [[self generateLabelDescription] retain];
	
	isValid = YES;
	return self;
}

- (void)parseServices:(NSString *)_services {
	NSArray *sParts = [_services componentsSeparatedByString:@"+"];
	NSString *aPart;
	for (aPart in sParts) {
		// Discard the E2U prefix
		if ([aPart isEqualToString:@"E2U"])
			continue;
		if ([aPart hasPrefix:@"x-lbl:"]) {
			[labelArray addObject:[aPart substringFromIndex:[@"x-lbl:" length]]];
			continue;
		}
		if ([self tokenIsLih:aPart]) {
			[lihArray addObject:aPart];
			continue;
		}
		// service types are anything not of the above
		[serviceTypeArray addObject:aPart];
	}
}

- (BOOL)tokenIsLih:(NSString *)token {
	
	NSArray *locationIndicatorHints = [NSArray arrayWithObjects:
									   @"x-mobile",
									   @"x-work",
									   @"x-main",
									   @"x-home",
									   @"x-transit",
									   @"x-prs",
									   nil
									   ];
	return ([locationIndicatorHints containsObject:token]);
}

- (NSString *)generateServiceDescription {
	// Create the service description string, concatenating all service types and LIH
	NSMutableString *theDesc = [[[NSMutableString alloc] initWithCapacity:30] autorelease];
	NSMutableString *servicePart = [NSMutableString stringWithString:@""];

	if (! isTerminal) {
		[theDesc appendFormat:@"%@:", LocTelStr(@"ntn")];
		return theDesc;
	}
	
	NSString *aPart;
	NSInteger i = 0, j = 0; // i is # of LIH, j is # of service types
	// Start with LIH
	for (aPart in lihArray) {
		if (i == 0) {
			[theDesc setString:LocTelStr(aPart)];
		} else {
			[theDesc appendFormat:@" and %@", LocTelStr(aPart)];
		}
		i++;
	}
	// Add a separator between LIH and service if there are LIH
	if (i > 0) {
		[theDesc appendString:@" "];
	}

	for (aPart in serviceTypeArray) {
		NSMutableString *strVal = [NSMutableString stringWithString:LocTelStr(aPart)];
		if ([strVal isEqual:aPart]) {
			// Unknown enum service, needs cleaning: remove the leading "x-"
			if ([strVal rangeOfString:@"x-"].location != NSNotFound)
				[strVal deleteCharactersInRange:[strVal rangeOfString:@"x-"]];
		}
		if (i == 0) {
			[servicePart setString:strVal];
		} else {
			// More than one service type, we have to make that look good:
			// Remove redundant words and append an ampersand
			NSArray *wordsInStr = [servicePart componentsSeparatedByString:@" "];
			NSString *aWord;
			for (aWord in wordsInStr) {
				if ([aWord isEqualToString:@"&"])
					continue;
				if ([strVal rangeOfString:aWord].location != NSNotFound)
					[strVal deleteCharactersInRange:[strVal rangeOfString:aWord]];
			}
			if (j > 0) {
				[servicePart appendFormat:@" & %@", strVal];
			}
		}
		j++;
	}
	[theDesc appendString:servicePart];
	return theDesc;
}

- (NSString *)generateLabelDescription {
	// This one is easy, just concatenate all labels and replace dashes with spaces
	NSMutableString *theDesc = [[[NSMutableString alloc] initWithCapacity:20] autorelease];

	[theDesc setString:[labelArray componentsJoinedByString:@" "]];
	[theDesc replaceOccurrencesOfString:@"-"
							 withString:@" "
								options:NSLiteralSearch
								  range:NSMakeRange(0, [theDesc length])];
	[theDesc replaceOccurrencesOfString:@"_"
							 withString:@" "
								options:NSLiteralSearch
								  range:NSMakeRange(0, [theDesc length])];
	// Label is a max of 20 chars
	// TODO: verify if it is 20 chars or 20 bytes
	if ([theDesc length] > 20) {
		[theDesc deleteCharactersInRange:NSMakeRange(20, [theDesc length] - 20)];
	}
	return theDesc;
}

- (NSComparisonResult)comparator:(RecordNaptr *)aNaptr {
	// First sort by order
	switch ([order compare:aNaptr.order]) {
		case NSOrderedAscending:
			return NSOrderedAscending;
			break;
		case NSOrderedDescending:
			return NSOrderedDescending;
			break;
		default:
			break;
	}
	
	// If order is the same, sort by preference
	// .tel specs do not condone same order and preference, so we don't need to check equality
	switch ([preference compare:aNaptr.preference]) {
		case NSOrderedAscending:
			return NSOrderedAscending;
			break;
		default:
			break;
	}
	return NSOrderedDescending;
}	

# pragma mark ------------- Utility functions for parsing record data fields ------------

- (NSString *)stringFromStringRdf:(const ldns_rdf *)rdf {
	NSMutableString *res;
	const uint8_t *data = ldns_rdf_data(rdf);
	uint8_t length = data[0];
	size_t i;
	res = [NSMutableString stringWithCapacity:length];
	for (i = 1; i <= length; ++i) {
		char ch = (char) data[i];
		if (isprint(ch)) {
			if (ch == '"' || ch == '\\') {
				[res appendString:@"\\"];
			}
			[res appendFormat:@"%c", ch];
		} else {
			[res appendFormat:@"\\%03u", (unsigned) ch];
		}
	}
	return res;
}

- (NSString *)stringFromDnameRdf:(const ldns_rdf *)rdf {
	NSMutableString *res;
	uint8_t src_pos = 0;
	uint8_t len;
	uint8_t *data;
	uint8_t i;
	
	data = (uint8_t*)ldns_rdf_data(rdf);
	len = data[src_pos];
	
	if (ldns_rdf_size(rdf) > LDNS_MAX_DOMAINLEN) {
		/* too large, return */
		return nil;
	}
	
	res = [NSMutableString stringWithCapacity:len];
	
	/* special case: root label */
	if (1 == ldns_rdf_size(rdf)) {
		// keep the output empty
	} else {
		while ((len > 0) && src_pos < ldns_rdf_size(rdf)) {
			src_pos++;
			for(i = 0; i < len; i++) {
				/* paranoia check for various 'strange' 
				 characters in dnames
				 */
				if (data[src_pos] == '.' ||
				    data[src_pos] == '(' || data[src_pos] == ')') {
					[res appendFormat:@"\\%c", data[src_pos]];
				} else if (!isprint((int) data[src_pos])) {
					[res appendFormat:@"\\%03u", data[src_pos]];
				} else {
					[res appendFormat:@"%c", data[src_pos]];
				}
				src_pos++;
			}
			
			len = data[src_pos];
			// Don't append the last dot
			if (len > 0)
				[res appendString:@"."];
		}
	}	
	return res;
}

- (NSNumber *)numberFromIntRdf:(const ldns_rdf *)rdf {
	NSNumber *res = [NSNumber numberWithInt:ntohs(*(uint16_t *)(ldns_rdf_data(rdf)))];
	return res;
}

@end
