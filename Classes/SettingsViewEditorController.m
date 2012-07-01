    //
//  InstellingenViewEditorController.m
//  DomeinZoeker
//
//  Created by Maarten Wullink on 04-05-10.
//  Copyright 2010 SIDN. All rights reserved.
//

#import "SettingsViewEditorController.h"

@implementation SettingsViewEditorController


@synthesize txtField;
@synthesize txtText;
@synthesize txtPlaceHolder;
@synthesize delegate;



 // The designated initializer.  Override if you create the controller programmatically and want to perform customization that is not appropriate for viewDidLoad.
- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil fieldTitle:(NSString *)fieldTitle fieldText:(NSString *)fieldText {
    if ((self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil])) {
		self.txtPlaceHolder = fieldTitle;
		self.title = fieldTitle;
		self.txtText = fieldText;
    }
    return self;
}



// Implement loadView to create a view hierarchy programmatically, without using a nib.
- (void)loadView {
	[super loadView];
	txtField.placeholder = self.txtPlaceHolder;
	txtField.text = self.txtText;
	self.view.backgroundColor = [[UIColor alloc] initWithRed:251.0/255 green:251.0/255 blue:251.0/255 alpha:1.0];

	txtField.keyboardType = UIKeyboardTypeEmailAddress;		

	txtField.delegate = self;
}



// Implement viewDidLoad to do additional setup after loading the view, typically from a nib.
- (void)viewDidLoad {
    [super viewDidLoad];
	txtField.placeholder = self.txtPlaceHolder;
	txtField.text = self.txtText;
	self.view.backgroundColor = [[UIColor alloc] initWithRed:251.0/255 green:251.0/255 blue:251.0/255 alpha:1.0];
	
	txtField.keyboardType = UIKeyboardTypeEmailAddress;		
	
	txtField.delegate = self;
	
	[txtField becomeFirstResponder];
}


-(BOOL)textFieldShouldReturn:(UITextField *)textField{
	
	[txtField resignFirstResponder];
	

	if (delegate) {
		[delegate settingUpdated:textField.text];
	}
	
	
	[self.navigationController popViewControllerAnimated:YES];
	
	return YES;
}




- (void)dealloc {
    [super dealloc];
	[txtText release];
	[txtPlaceHolder release];
}


@end
