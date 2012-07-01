//
//  Preferences.m
//  DomeinZoeker
//
//  Created by Maarten Wullink on 01-05-10.
//  Copyright 2010 SIDN. All rights reserved.
//

#import "Preferences.h"


@implementation Preferences


+ (NSString *)serverHostName
{
	// Does preference exist...
	//if ([[NSUserDefaults standardUserDefaults] stringForKey:@"hostname"] == 0)
		return [[NSUserDefaults standardUserDefaults] stringForKey:@"hostname"];
	//else

}


+ (BOOL) setPreferences:(NSString *)serverHostName
{
	// Set values
	[[NSUserDefaults standardUserDefaults] setBool:saveUsername forKey:@"hostname"];
	
	// Return the results of attempting to write preferences to system
	return [[NSUserDefaults standardUserDefaults] synchronize];
}

@end