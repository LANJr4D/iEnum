//
//  AppDelegate.m
//  iEnum
//
//  Created by Maarten Wullink on 12-01-11.
//  Copyright 2011 SIDN. All rights reserved.
//

#import "AppDelegate.h"
#import "FileUtil.h"
#import <QuartzCore/QuartzCore.h>
#import "ModalAlert.h"
#import "ContactsViewController.h"

@implementation AppDelegate

@synthesize window;
@synthesize tabBarController;
@synthesize settings;


- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions {    
   // Add the tab bar controller's current view as a subview of the window
    [window addSubview:tabBarController.view];
    [window makeKeyAndVisible];

	application.applicationSupportsShakeToEdit = YES;
	
	[self loadSettings];
	return YES;
}


-(void)hideActivityViewer
{
	if(activityViewShowing){
		[activityWheel stopAnimating];
		[activityView removeFromSuperview];
		activityView = nil;
		activityViewShowing = NO;
	}
	
}


-(void)showActivityViewer
{
	activityViewShowing = YES;
	[activityView release];
	activityView = [[UIView alloc] initWithFrame: CGRectMake(0, 0, window.bounds.size.width, window.bounds.size.height)];
	activityView.backgroundColor = [UIColor clearColor];
	
	UIView *centralView = [[UIView alloc] initWithFrame: CGRectMake(50, 165, 220, 150)];
	centralView.backgroundColor = [UIColor blackColor];
	centralView.alpha = 0.4;
	centralView.layer.cornerRadius = 10;
	
	if(!activityWheel){
		activityWheel = [[UIActivityIndicatorView alloc] initWithFrame: CGRectMake(centralView.bounds.size.width/2-15, centralView.bounds.size.height /2 - 15, 30, 30)];
		activityWheel.activityIndicatorViewStyle = UIActivityIndicatorViewStyleWhite;
		activityWheel.autoresizingMask = (UIViewAutoresizingFlexibleLeftMargin |
										  UIViewAutoresizingFlexibleRightMargin |
										  UIViewAutoresizingFlexibleTopMargin |
										  UIViewAutoresizingFlexibleBottomMargin);
	}
	
	
	UILabel *lblLaden = [[UILabel alloc] initWithFrame:CGRectMake(centralView.bounds.size.width/2-22, centralView.bounds.size.height /2 + 20, 80, 25)];
	lblLaden.text = NSLocalizedString(@"ACTIVITY.LOADING", nil);
	lblLaden.font = [UIFont fontWithName:@"Arial-BoldMT" size:14];
	lblLaden.textColor = [UIColor whiteColor];
	lblLaden.backgroundColor = [UIColor clearColor];
	
	
	[centralView addSubview:activityWheel];
	
	[centralView addSubview:lblLaden];
	
	[activityView addSubview:centralView];
	
	[window addSubview: activityView];
	
	[lblLaden release];
	[activityView release];
	[centralView release];
	[activityWheel startAnimating];
}

-(void) loadSettings{
	//get the documents directory:
	NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
	NSString *documentsDirectory = [paths objectAtIndex:0];
	
	//create filename to read settings from
	NSString *fileName = [NSString stringWithFormat:@"%@/settings.conf", documentsDirectory];
	
	if (![[NSFileManager defaultManager] fileExistsAtPath:fileName]) {
		//settings file does not exist
		return;
	}
	
	//read file contents
	NSString *content = [[NSString alloc] initWithContentsOfFile:fileName usedEncoding:nil error:nil];
	if (!content) {
		//no settings found
		return;
	}

	//convert to data object
	self.settings = [[[AppSettings alloc] init] autorelease];
	NSArray *chunks = [content componentsSeparatedByString:@"\n"];
	for (int i=0; i<[chunks count]; i++) {
		NSString *chunk = [chunks objectAtIndex:i];
	
		NSArray *lineParts = [chunk componentsSeparatedByString:@"="];
		NSString *key = [lineParts objectAtIndex:0];
		
		if([key rangeOfString:@"server"].location != NSNotFound) {
		   //hostname for nameserver found
			NSString *value = [lineParts objectAtIndex:1];
			settings.server = [value stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceAndNewlineCharacterSet]];
		}else if([key rangeOfString:@"countrycode"].location != NSNotFound) {
			//default country code found
			NSString *value = [lineParts objectAtIndex:1];
			settings.countrycode = [value stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceAndNewlineCharacterSet]];
			
		}else if([key rangeOfString:@"dnssuffix"].location != NSNotFound) {
			//default dns suffix found, use this instead of .e164.arpa
			NSString *value = [lineParts objectAtIndex:1];
			settings.suffix = [value stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceAndNewlineCharacterSet]];
			
		}
		
	}

	 NSLog(@"server:%@ country:%@ suffix:%@", settings.server, settings.countrycode, settings.suffix);
	
	//create a temp hosts file, the dns resolver can use
	[self createHostsFile];

	
}

-(void) saveSettings{
	//get the documents directory:
	NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
	NSString *documentsDirectory = [paths objectAtIndex:0];
	
	//make a file name to write the data to using the documents directory:
	NSString *fileName = [NSString stringWithFormat:@"%@/settings.conf", documentsDirectory];
	
	
	NSString *content = @"";
	if (self.settings.server && [self.settings.server length] > 0) {
		//save server settings
		NSString *server = [NSString stringWithFormat:@"server=%@\n", self.settings.server];
		content = [content stringByAppendingString:server];
	}
	if (self.settings.countrycode && [self.settings.countrycode length] > 0) {
		//save server settings
		NSString *cc = [NSString stringWithFormat:@"countrycode=%@\n", self.settings.countrycode];
		content = [content stringByAppendingString:cc];
	}
	
	if (self.settings.suffix && [self.settings.suffix length] > 0) {
		//save server settings
		NSString *suffix = [NSString stringWithFormat:@"dnssuffix=%@\n", self.settings.suffix];
		content = [content stringByAppendingString:suffix];
	}
	
	//save content to the documents directory
	[content writeToFile:fileName atomically:NO encoding:NSStringEncodingConversionAllowLossy error:nil];
	
	[self createHostsFile];
	
}

-(void)createHostsFile{
	//delete the old appHosts file first
	NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
	NSString *documentsDirectory = [paths objectAtIndex:0];
	NSString *fileName = [NSString stringWithFormat:@"%@/appHosts.conf", documentsDirectory];
	
	NSFileManager *fileManager = [NSFileManager defaultManager];
    NSError *error;
	
	[fileManager removeItemAtPath:fileName error:&error];
    if (error != nil)
    {
        NSLog(@"Error: %@", error);
        NSLog(@"Path to file: %@", fileName);
    }
	
	
	if (self.settings.server) {
		NSString *ip = nil;
		
		NSString *regexp = @"^(?:[0-9]{1,3}\\.){3}[0-9]{1,3}$";
		NSPredicate *regextest = [NSPredicate
								  predicateWithFormat:@"SELF MATCHES %@", regexp];
		
		if ([regextest evaluateWithObject:self.settings.server] == YES) {
			ip = self.settings.server;
		} 
		else {
			//invalid ip, assume it is a hostname, convert a hostname to a ip
			//ldns only works with an ip address
			ip = [BDHost addressForHostname:self.settings.server];
		}
			
		if (ip) {
			
			//save server settings
			NSString *ns = [NSString stringWithFormat:@"nameserver %@\n", ip];
			[ns writeToFile:fileName atomically:NO encoding:NSStringEncodingConversionAllowLossy error:nil];
			
		}
				
	}
}



- (void)dealloc {
    [tabBarController release];
    [window release];
    [super dealloc];
}

@end

