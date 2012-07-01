//
//  BDHost.h
//  iEnum
//
//  Created by Maarten Wullink on 12-01-11.
//  Copyright 2011 SIDN. All rights reserved.
//

#import <Foundation/Foundation.h>


@interface BDHost : NSObject {

}

+ (NSString *)addressForHostname:(NSString *)hostname;
+ (NSArray *)addressesForHostname:(NSString *)hostname;
+ (NSArray *)hostnamesForAddress:(NSString *)address;
+ (NSString *)hostnameForAddress:(NSString *)address;
+ (NSArray *)ipAddresses;
+ (NSArray *)ethernetAddresses;

@end
