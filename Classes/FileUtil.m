//
//  FileUtil.m
//  iEnum
//
//  Created by Maarten Wullink on 12-01-11.
//  Copyright 2011 SIDN. All rights reserved.
//

#import "FileUtil.h"


@implementation FileUtil


+ (BOOL)writeApplicationData:(id)data toFile:(NSString *)filename{
	
	NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    NSString *documentsDirectory = [paths objectAtIndex:0];
    if (!documentsDirectory) {
        NSLog(@"Documents directory not found!");
        return NO;
    }
    NSString *appFile = [documentsDirectory stringByAppendingPathComponent:filename];
	
	BOOL result = [NSKeyedArchiver archiveRootObject:data toFile:appFile];
	if(!result)
		NSLog(@"Application data not saved!");
	
	return result;
}




+ (id) applicationDataFromFile:(NSString *)filename {
	
    NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    NSString *documentsDirectory = [paths objectAtIndex:0];
    NSString *appFile = [documentsDirectory stringByAppendingPathComponent:filename];
 	
	id object = [NSKeyedUnarchiver unarchiveObjectWithFile:appFile];
	
    return object;
}

@end
