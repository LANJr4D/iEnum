//
//  HomeViewController.m
//  DomeinZoeker
//
//  Created by Maarten Wullink on 27-04-10.
//  Copyright 2010 SIDN. All rights reserved.
//

#import "HomeViewController.h"


@implementation HomeViewController


#pragma mark -
#pragma mark Initialization


- (void) awakeFromNib{

	self.parentViewController.view.backgroundColor = [UIColor colorWithPatternImage:[UIImage imageNamed:@"home_background.png"]];
	
	[self.navigationController.navigationBar setTintColor:[[[UIColor alloc] initWithRed:15.0 / 255 green:107.0 / 255 blue:172.0 / 255 alpha:1.0] autorelease]];
	
	self.title = NSLocalizedString(@"HOME.TAB.TITLE", @"");
	self.navigationItem.title = NSLocalizedString(@"HOME.TAB.TITLE", @"");
	

	
}										   
		
-(void)viewDidLoad{
	[super viewDidLoad];
	NSLog(@"view did load in home view");
	//
	GradientButton *registerBtn = [GradientButton buttonWithType:UIButtonTypeCustom];
	[registerBtn useBlackStyle];
	registerBtn.frame = CGRectMake(20, 50, 280,44);
	[registerBtn setTitle:NSLocalizedString(@"HOME.BUTTON.REGISTER", @"") forState:UIControlStateNormal];
	[registerBtn addTarget:self action:@selector(registerButtonPressed:) forControlEvents:UIControlEventTouchUpInside];
	[self.view addSubview:registerBtn];
}

-(void)registerButtonPressed:(id)sender{
	NSLog(@"registerButtonPressed");
	
	//reuse the enumservice class to shwo the enum  registrar webpage on enum.nl
	EnumService *registarService = [[EnumService alloc] init];
	registarService.url = NSLocalizedString(@"HOME.BUTTON.REGISTER.REGISTRARS.URL", @"");
	registarService.title = NSLocalizedString(@"HOME.BUTTON.REGISTER.REGISTRARS.TITLE", @"");
	
	WebContentViewController *wcvc = [[[WebContentViewController alloc]initWithNibName:@"WebContentViewController" bundle:nil andService:registarService] autorelease];
	[self.navigationController pushViewController:wcvc animated:YES];
	
}


@end

