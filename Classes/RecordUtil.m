//
//  RecordUtil.m
//  iEnum
//
//  Created by Maarten Wullink on 06-12-10.
//  Copyright 2010 SIDN. All rights reserved.
//

#import "RecordUtil.h"


@implementation RecordUtil


+(NSArray *)getAllWeblocations:(NSArray *)naptrList{
	
	NSMutableArray *list = [NSMutableArray arrayWithCapacity:10];
	for (RecordNaptr *rec in naptrList) {
		
		if ([[rec serviceDescription] isEqualToString:@"web:http"]  ) {
			[list addObject:rec];
		}
	}
	
	return list;
	
}


+(NSString *)getPublicKey:(NSArray *)naptrList{

	for (RecordNaptr *rec in naptrList) {
		
		if ([[rec serviceDescription] isEqualToString:@"key:http"]  ) {
			return rec.uriContent;
		}
	}
	
	return @"";
}


@end
