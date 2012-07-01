//
//  WebContentViewController.m
//  DomeinZoeker
//
//  Created by Maarten Wullink on 09-05-10.
//  Copyright 2010 SIDN. All rights reserved.
//

#import "WebContentViewController.h"



@implementation WebContentViewController

@synthesize enumService;


 // The designated initializer.  Override if you create the controller programmatically and want to perform customization that is not appropriate for viewDidLoad.
- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil andService:(EnumService *)aEnumService {
    if ((self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil])) {
        // Custom initialization
    }

	self.enumService = aEnumService;
	self.parentViewController.view.backgroundColor = [[[UIColor alloc] initWithRed:251.0/255 green:251.0/255 blue:251.0/255 alpha:1.0] autorelease];
	self.title = self.enumService.title;
    return self;
}



// Implement viewDidLoad to do additional setup after loading the view, typically from a nib.
- (void)viewDidLoad {
	[super viewDidLoad];
	
	UIWebView *wv = (UIWebView *) [self.view viewWithTag:101];
	wv.backgroundColor = [UIColor clearColor];
	wv.scalesPageToFit = YES;
	wv.delegate = self;
	
	NSURL *nsUrl = [NSURL URLWithString:enumService.url];
	NSURLRequest *requestObj = [NSURLRequest requestWithURL:nsUrl];
	
	//show network activity indicator
	UIApplication* app = [UIApplication sharedApplication];
	app.networkActivityIndicatorVisible = YES;
	
	[wv loadRequest:requestObj];
}

- (void)webViewDidFinishLoad:(UIWebView *)webView{
    //stop network activity indicator
	UIApplication* app = [UIApplication sharedApplication];
	app.networkActivityIndicatorVisible = NO;
}


-(void)viewWillDisappear:(BOOL)animated{
	[super viewWillDisappear:animated];
	//make sure the download indicator is removed if the user goed back to
	//the prfile before the web page is loaded fully
	UIApplication* app = [UIApplication sharedApplication];
	app.networkActivityIndicatorVisible = NO;
	
}


- (void)viewDidUnload {
    [super viewDidUnload];
    // Release any retained subviews of the main view.
    // e.g. self.myOutlet = nil;
}


- (void)dealloc {
    [super dealloc];
}


@end
