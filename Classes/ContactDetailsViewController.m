    //
//  ContactDetailsView.m
//  iEnum
//
//  Created by Maarten Wullink on 05-12-10.
//  Copyright 2010 SIDN. All rights reserved.
//

#import "ContactDetailsViewController.h"


@implementation ContactDetailsViewController

@synthesize number;
@synthesize profile;

- (id)initWithProfile:(ContactProfile *)aProfile {
    self = [super init];
    if (self) {
		self.profile = aProfile;
    }
    return self;
}

- (void)loadView {
	self.view = [[[ContactDetailView alloc] initWithFrame:CGRectMake(0, 0, 320, 480) andProfile:self.profile] autorelease];
	self.title = NSLocalizedString(@"CONTACTS.DETAILS.TITLE", @"");
	self.view.backgroundColor = [UIColor colorWithPatternImage:[UIImage imageNamed:@"contact_details_background_10.png"]];	
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
	}else if ([service getType] == SERVICE_VOICE) {
		NSLog(@"Make a call to: %@", service.url);
		UIApplication *app = [UIApplication sharedApplication];
		NSString *numberUrl = [NSString stringWithFormat:@"tel://%@", service.url];
		[app openURL:[NSURL URLWithString:numberUrl]];
		
	}else if ([service getType] == SERVICE_MAIL) {
		NSLog(@"send mail  to: %@", service.url);
		if (![MFMailComposeViewController canSendMail]) {
			//cannot send email with this device
			[ModalAlert alertWith:NSLocalizedString(@"error", nil) andMessage:NSLocalizedString(@"EMAIL.NOT.SUPPORTED", nil)];
			return;
		}
		[self sendEmailToAddress:service.url];
		
	}else {
		NSLog(@"user selected unimplemented service %@ with url %@", service.title, service.url);
	}

}


-(void)sendEmailToAddress:(NSString *)address{
	NSString *footer = NSLocalizedString(@"EMAIL.PLAIN.MESSAGE.FOOTER", nil);
	
	//create the email body
	NSString *body = [NSString stringWithFormat:@"\n\n\n%@", footer];
	
	
	MFMailComposeViewController *controller = [[MFMailComposeViewController alloc] init];
	UIColor *navBarColor = [[[UIColor alloc] initWithRed:15.0 / 255 green:107.0 / 255 blue:172.0 / 255 alpha:1.0] autorelease];
	
	[[controller navigationBar] setTintColor:navBarColor];
	
	// Set the view controller delegate
	controller.mailComposeDelegate = self;
	
	// Set message body, if you want
	[controller setMessageBody:body isHTML:NO];
	
	[controller setToRecipients:[NSArray arrayWithObject:address]];
	
	//hide activity indicator
	AppDelegate *appDelegate= (AppDelegate *) [[UIApplication sharedApplication] delegate];
	[appDelegate hideActivityViewer];
	
	// Present the view controller 
	[[self navigationController] presentModalViewController:controller animated:YES];
	
	// Memory management
	[controller release];
	
}

- (void)mailComposeController:(MFMailComposeViewController*)controller didFinishWithResult:(MFMailComposeResult)result error:(NSError*)error {
	//hide the email composer
	[self becomeFirstResponder];
	[self dismissModalViewControllerAnimated:NO];
	
    //check if the user cancelled mail sending, maybe to change the plaintext?	
	if (result != MFMailComposeResultCancelled) {
		//notify caller that email handling is done
		[[self navigationController] popViewControllerAnimated:YES];
	}
	
}

-(void)mailActionDone{
	//email has been sent, remove editor
	[self dismissModalViewControllerAnimated:YES];
	
}




@end
