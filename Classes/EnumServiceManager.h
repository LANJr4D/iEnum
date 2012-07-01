//
//  EnumService.h
//  iEnum
//
//  Created by Maarten Wullink on 02-01-11.
//  Copyright 2011 SIDN. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "EnumService.h"
#import "LocationService.h"

@interface EnumServiceManager : NSObject {
}

+(EnumService *)createEnumServiceWithUrl:(NSString *)url andType:(int) type;
+(EnumService *)getEnumSericeForVoiceWithUrl:(NSString *)url andType:(int) type;
+(EnumService *)getEnumSericeForMailWithUrl:(NSString *)url andType:(int) type;
+(EnumService *)getEnumSericeForWeb:(NSString *)url;

@end
