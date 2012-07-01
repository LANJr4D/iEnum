//
//  LookupViewController.m
//  DomeinZoeker
//
//  Created by Maarten Wullink on 28-04-10.
//  Copyright 2010 SIDN. All rights reserved.
//

#import "LookupViewController.h"


@implementation LookupViewController


@synthesize searchBar;
@synthesize profile;
@synthesize detailView;


#pragma mark -
#pragma mark Initialization


- (void) awakeFromNib{
	self.searchBar = [[[UISearchBar alloc] initWithFrame:CGRectMake(0.0f, 0.0f, 320.0f, 44.0f)] autorelease];
	self.searchBar.autocorrectionType = UITextAutocorrectionTypeNo;
	self.searchBar.autocapitalizationType = UITextAutocapitalizationTypeNone;
	self.searchBar.keyboardType = UIKeyboardTypeAlphabet;
	self.searchBar.delegate = self;
	self.searchBar.showsCancelButton = YES;
	self.searchBar.placeholder = NSLocalizedString(@"LOOKUP.TAB.DEFAULT.SEARCH.TEXT", @"");
	
	
	self.searchBar.tintColor = [[[UIColor alloc] initWithRed:15.0 / 255 green:107.0 / 255 blue:172.0 / 255 alpha:1.0] autorelease];
	self.navigationItem.titleView = self.searchBar;

	self.parentViewController.view.backgroundColor = [[[UIColor alloc] initWithRed:251.0/255 green:251.0/255 blue:251.0/255 alpha:1.0] autorelease];
	[self.navigationController.navigationBar setTintColor:[[[UIColor alloc] initWithRed:15.0 / 255 green:107.0 / 255 blue:172.0 / 255 alpha:1.0] autorelease]];
	
	self.searchBar.backgroundColor = [UIColor clearColor];
	
	self.title = NSLocalizedString(@"LOOKUP.TAB.TITLE", @"");
	
}



- (void)searchBarSearchButtonClicked:(UISearchBar *)searchBar{
		
	[self.searchBar resignFirstResponder];
	
	//check if there is a network connection available
	if (![DnsResolver connectedToNetwork]) {
		//not network, no lookup. show error mesg
		[ModalAlert alertWith:NSLocalizedString(@"error", nil) andMessage:NSLocalizedString(@"ERROR.NO.NETWORK", nil)];
		return;
	}
	
	[self doLookup:[self.searchBar text]];
}


- (void)searchBarCancelButtonClicked:(UISearchBar *)searchBar{
	self.searchBar.text = @"";
	[self.searchBar resignFirstResponder];
}



-(void)doLookup:(NSString *)number{
	AppDelegate *appDelegate= (AppDelegate *) [[UIApplication sharedApplication] delegate];
	[appDelegate showActivityViewer];
	
	//load data, this is done on seperate thead, when the thread is done
	//the thread will use the callback mehod for notification
	
	self.profile = [[ContactProfile alloc] initWithNumber:number];
	//set the delegate to this class
	self.profile.delegate = self;
	[self.profile loadProfile];
		
}

- (void) profileProcessed:(NSNumber *)count{	
	//profile is loaded, hide activity wheel
	AppDelegate *appDelegate= (AppDelegate *) [[UIApplication sharedApplication] delegate];
	[appDelegate hideActivityViewer];
	
	if ([count intValue] == 0) {
		//no supported enum naptr records found
		[ModalAlert alertWith:NSLocalizedString(@"STATUS", nil) andMessage:NSLocalizedString(@"ERROR.PROFILE.NORECORDS", nil)];
		return;
	}
	
	if( ![self.profile.status isEqual:@"OK"]){
		//loading profile failed
		[ModalAlert alertWith:NSLocalizedString(@"error", nil) andMessage:NSLocalizedString(@"error.loading.profile", nil)];
		
		return;
	}

	//remove the old details page, if present
	if (self.detailView ) {
		[self.detailView removeFromSuperview];
	}
	//show the details page
	self.detailView = [[[ContactDetailView alloc] initWithFrame:CGRectMake(0, 0, 320, 480) andProfile:self.profile] autorelease];
	
	
	CGContextRef context = UIGraphicsGetCurrentContext();
	[UIView beginAnimations:nil context:context];
	[UIView setAnimationCurve:UIViewAnimationCurveEaseInOut];
	[UIView setAnimationDuration:1.0];
	UIView *whiteBackdrop = [self.view viewWithTag:901];
	
	[whiteBackdrop insertSubview:self.detailView atIndex:0];
	
	[UIView setAnimationTransition: UIViewAnimationTransitionFlipFromLeft forView:whiteBackdrop cache:YES];
	[whiteBackdrop exchangeSubviewAtIndex:1 withSubviewAtIndex:0];
	[UIView setAnimationDelegate:self];
	[UIView commitAnimations];
}




-(IBAction)serviceButtonPressed:(id)sender{

	UIButton *btn = (UIButton *)sender;
	EnumService *service = [self.profile.enumServices objectAtIndex:btn.tag-900];

	if ([service getType] == SERVICE_WEB) {
		WebContentViewController *wcvc = [[[WebContentViewController alloc]initWithNibName:@"WebContentViewController" bundle:nil andService:service] autorelease];
		[self.navigationController pushViewController:wcvc animated:YES];
	}else if ([service getType] == SERVICE_LOC) {
		LocationService *locservice = (LocationService *)service;
		MapAnnotation *annotation = [[MapAnnotation alloc] initWithLatitude:[locservice.latitude doubleValue]
																  longitude:[locservice.longtitude doubleValue] title:self.profile.vcard.fullname subTitle:@"" ];
	
		
		MapViewController *mvc = [[[MapViewController alloc] initWithAnnotation:annotation] autorelease];
		[self.navigationController pushViewController:mvc animated:YES];
	}else if ([service getType] == SERVICE_KEY) {
		if (![MFMailComposeViewController canSendMail]) {
			//cannot send email with this device
			[ModalAlert alertWith:NSLocalizedString(@"error", nil) andMessage:NSLocalizedString(@"EMAIL.NOT.SUPPORTED", nil)];
			return;
		}
		//load crypto service
		CryptoService *cs = [[[CryptoService alloc] initWithUrl:service.url] autorelease];
		cs.keyUrl = service.url;
		EmailViewController *evc = [[[EmailViewController alloc] initWithCrypto:cs] autorelease];
		[self.navigationController pushViewController:evc animated:YES];
	}
	

}




@end

