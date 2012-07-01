//
//  RecordUtil.h
//  iEnum
//
//  Created by Maarten Wullink on 06-12-10.
//  Copyright 2010 SIDN. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "RecordNaptr.h"

@interface RecordUtil : NSObject {

}

+(NSArray *)getAllWeblocations:(NSArray *)naptrList;
+(NSString *)getPublicKey:(NSArray *)naptrList;

@end
