//
//  FileUtil.h
//  iEnum
//
//  Created by Maarten Wullink on 12-01-11.
//  Copyright 2011 SIDN. All rights reserved.
//
#import <Foundation/Foundation.h>

@interface FileUtil : NSObject {

}

+ (BOOL) writeApplicationData:(id) data toFile:(NSString *) filename;
+ (NSMutableArray *) applicationDataFromFile:(NSString *) filename;
@end
