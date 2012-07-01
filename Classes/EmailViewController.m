    //
//  EmailViewController.m
//  iEnum
//
//  Created by Maarten Wullink on 06-01-11.
//  Copyright 2011 SIDN. All rights reserved.
//

#import "EmailViewController.h"

@implementation EmailViewController

@synthesize textviev;
@synthesize cryptoService;


- (id)initWithCrypto:(CryptoService *)crypto{
    self = [super init];
    if (self) {
        self.cryptoService = crypto;
    }
    return self;
}



// Implement loadView to create a view hierarchy programmatically, without using a nib.
- (void)loadView {
	[super loadView];
	self.title = NSLocalizedString(@"EMAIL.TITLE", @"");
	
	UIBarButtonItem *btnSend = [[[UIBarButtonItem alloc]
								 initWithTitle:NSLocalizedString(@"EMAIL.BUTTON.SEND",nil)
								   style:UIBarButtonItemStyleBordered 
								   target:self 
								 action:@selector(sendEmail:)] autorelease];
    
    self.navigationItem.rightBarButtonItem = btnSend;
	
	self.textviev = [[UITextView alloc] initWithFrame:CGRectMake(0, 0, 320, 480)];
	self.textviev.font = [UIFont fontWithName:@"Helvetica" size:14.0];
	[self.view addSubview:self.textviev];
}


-(void)sendEmail:(id)sender{
	//show progress indicator
	AppDelegate *appDelegate= (AppDelegate *) [[UIApplication sharedApplication] delegate];
	[appDelegate showActivityViewer];
	
	//encrypt user message
	self.cryptoService.delegate = self;
	[self.cryptoService startCryptoWithPlaintext:self.textviev.text];
}

- (void)cryptoReady:(NSString*)ciphertext{
	//crypto ready send email.
	NSString *pre = NSLocalizedString(@"EMAIL.MESSAGE.PRE", nil);
	NSString *post = NSLocalizedString(@"EMAIL.MESSAGE.POST", nil);
	NSString *footer = NSLocalizedString(@"EMAIL.MESSAGE.FOOTER", nil);

	//create the email body
	NSString *body = [NSString stringWithFormat:@"%@\n%@\n%@\n%@\n", pre, ciphertext, post, footer];

	
	MFMailComposeViewController *controller = [[MFMailComposeViewController alloc] init];
	UIColor *navBarColor = [[[UIColor alloc] initWithRed:15.0 / 255 green:107.0 / 255 blue:172.0 / 255 alpha:1.0] autorelease];

	[[controller navigationBar] setTintColor:navBarColor];

	// Set the view controller delegate
	controller.mailComposeDelegate = self;
	
	// Set message body, if you want
	[controller setMessageBody:body isHTML:NO];
	
	//hide activity indicator
	AppDelegate *appDelegate= (AppDelegate *) [[UIApplication sharedApplication] delegate];
	[appDelegate hideActivityViewer];
	
	// Present the view controller 
	[[self navigationController] presentModalViewController:controller animated:YES];
	
	// Memory management
	[controller release];
	
}


-(void)cryptoFailed{
	//hide the activity indicator
	AppDelegate *appDelegate= (AppDelegate *) [[UIApplication sharedApplication] delegate];
	[appDelegate hideActivityViewer];
	
	//cannot load the public key for encryption
	[ModalAlert alertWith:NSLocalizedString(@"error", nil) andMessage:NSLocalizedString(@"EMAIL.KEY.FAILED", nil)];

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



// Implement viewDidLoad to do additional setup after loading the view, typically from a nib.
- (void)viewDidLoad {
    [super viewDidLoad];
	
	self.textviev.keyboardType = UIKeyboardTypeDefault;
	
	[self.textviev becomeFirstResponder];
}

-(void)viewWillAppear:(BOOL)animated{
	[super viewWillAppear:animated];
	//show the keybord when the view is displayed
	[self.textviev becomeFirstResponder];
}





@end
