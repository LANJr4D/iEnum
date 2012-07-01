    //
//  OverzichtViewController.m
//  DomeinZoeker
//
//  Created by Maarten Wullink on 23-04-10.
//  Copyright 2010 SIDN. All rights reserved.
//

#import "OverzichtViewController.h"


@implementation OverzichtViewController 

@synthesize lblOverzicht;





- (void)didReceiveMemoryWarning {
    // Releases the view if it doesn't have a superview.
    [super didReceiveMemoryWarning];
    
    // Release any cached data, images, etc that aren't in use.
}

- (void)viewDidUnload {
    [super viewDidUnload];
    // Release any retained subviews of the main view.
    // e.g. self.myOutlet = nil;
}

- (BOOL) shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)toInterfaceOrientation{
	return YES;	
	
}

- (void)dealloc {
    [super dealloc];
}


@end
