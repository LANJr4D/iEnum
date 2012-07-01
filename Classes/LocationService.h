//
//  LocationService.h
//  iEnum
//
//  Created by Maarten Wullink on 08-01-11.
//  Copyright 2011 SIDN. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "EnumService.h"

@interface LocationService : EnumService {

	NSString *longtitude;
	NSString *latitude;
}

@property(nonatomic, retain) NSString *longtitude;
@property(nonatomic, retain) NSString *latitude;


-(void)parse:(NSString *)data;
@end
