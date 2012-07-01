//
//  EnumService.m
//  iEnum
//
//  Created by Maarten Wullink on 04-01-11.
//  Copyright 2011 SIDN. All rights reserved.
//

#import "EnumService.h"


@implementation EnumService

@synthesize icon;
@synthesize url;
@synthesize title;


- (id)init{
    
    self = [super init];
    if (self) {
        // Initialization code.
    }
    return self;
}

-(int)getType{
	return type;
}
-(void)setType:(int)pType{
	type = pType;
}

- (void)dealloc {
    [super dealloc];
}


@end
