//
//  NumberSelectionController.m
//  iEnum
//
//  Created by Maarten Wullink on 04-12-10.
//  Copyright 2010 SIDN. All rights reserved.
//

#import "NumberSelectionController.h"


@implementation NumberSelectionController

@synthesize contact;
@synthesize profile;


#pragma mark -
#pragma mark Initialization


- (id)initWithStyle:(UITableViewStyle)style {
	self = [super initWithStyle:style];
    if (self) {
       
		self.title = NSLocalizedString(@"CONTACT.NUMBER.SELECT.TITLE", @"");
		self.view.backgroundColor = [[UIColor alloc] initWithRed:251.0/255 green:251.0/255 blue:251.0/255 alpha:1.0];

    }
    return self;
}




#pragma mark -
#pragma mark Table view data source

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
    // Return the number of sections.
    return 1;
}


- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
    return [self.contact.phoneArray count];
}


// Customize the appearance of table view cells.
- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
    
    static NSString *CellIdentifier = @"Cell";
	//use a cell with subtitle
   
    UITableViewCell *cell = [tableView dequeueReusableCellWithIdentifier:CellIdentifier];
    if (cell == nil) {
        cell = [[[UITableViewCell alloc] initWithStyle:UITableViewCellStyleSubtitle reuseIdentifier:CellIdentifier] autorelease];
    }
    
    [cell setAccessoryType:UITableViewCellAccessoryDetailDisclosureButton];
	cell.editingAccessoryType = UITableViewCellAccessoryNone;
	
	NSArray *numbers = self.contact.phoneArray;
	NSArray *labels = self.contact.phoneLabels;
	
	NSString *number = [numbers objectAtIndex:indexPath.row];
	NSString *label = [labels objectAtIndex:indexPath.row];
	cell.textLabel.font = [UIFont fontWithName:@"Arial-BoldMT" size:16];
	cell.textLabel.text = [self displayPropertyName:label];
	cell.detailTextLabel.text = number;
	cell.detailTextLabel.textColor = [UIColor grayColor];
    [cell.detailTextLabel setFont:[UIFont fontWithName:@"Arial" size:12]];
    
    return cell;
}



- (NSString *) displayPropertyName:(NSString *) propConst{
    if ([propConst isEqualToString:@"_$!<Home>!$_"]) return NSLocalizedString(@"CONTACT.NUMBER.SELECT.HOME", @"");
    if ([propConst isEqualToString:@"_$!<HomeFAX>!$_"]) return NSLocalizedString(@"CONTACT.NUMBER.SELECT.HOME.FAX", @"");
    if ([propConst isEqualToString:@"_$!<Main>!$_"]) return NSLocalizedString(@"CONTACT.NUMBER.SELECT.MAIN", @"");
	if ([propConst isEqualToString:@"_$!<Mobile>!$_"]) return NSLocalizedString(@"CONTACT.NUMBER.SELECT.MOBILE", @"");
	if ([propConst isEqualToString:@"_$!<Pager>!$_"]) return NSLocalizedString(@"CONTACT.NUMBER.SELECT.PAGER", @"");
	if ([propConst isEqualToString:@"_$!<Work>!$_"]) return NSLocalizedString(@"CONTACT.NUMBER.SELECT.WORK", @"");
    if ([propConst isEqualToString:@"_$!<WorkFAX>!$_"]) return NSLocalizedString(@"CONTACT.NUMBER.SELECT.WORK.FAX", @"");
	if ([propConst isEqualToString:@"_$!<Other>!$_"]) return NSLocalizedString(@"CONTACT.NUMBER.SELECT.OTHER", @"");
	//if nog match then return the original label
    return propConst;
}



#pragma mark -
#pragma mark Table view delegate

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
	//check if there is a network connection available
	if (![DnsResolver connectedToNetwork]) {
		//not network, no lookup. show error mesg
		[ModalAlert alertWith:NSLocalizedString(@"error", nil) andMessage:NSLocalizedString(@"ERROR.NO.NETWORK", nil)];
		return;
	}
		
	NSString *number = [self.contact.phoneArray objectAtIndex:indexPath.row];
	
	//show the activity wheel
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
	
	//show the details page
	ContactDetailsViewController *cdv = [[ContactDetailsViewController alloc] initWithProfile:self.profile];
	[self.navigationController pushViewController:cdv animated:YES];
}



@end

