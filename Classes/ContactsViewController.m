//
//  ContactsViewController.m
//  DomeinZoeker
//
//  Created by Maarten Wullink on 24-04-10.
//  Copyright 2010 SIDN. All rights reserved.
//

#import "ContactsViewController.h"
#import "AppDelegate.h"


@implementation ContactsViewController

@synthesize contactList;
@synthesize profile;


-(void)awakeFromNib{

	self.parentViewController.view.backgroundColor = [[[UIColor alloc] initWithRed:251.0/255 green:251.0/255 blue:251.0/255 alpha:1.0] autorelease];
	[self.navigationController.navigationBar setTintColor:[[[UIColor alloc] initWithRed:15.0 / 255 green:107.0 / 255 blue:172.0 / 255 alpha:1.0] autorelease]];
	self.title = NSLocalizedString(@"CONTACTS.TAB.TITLE", @"");
	self.tableView.backgroundColor = [UIColor clearColor];	
}


- (void) viewWillAppear:(BOOL)animated{
	if (self.contactList == nil) {
		//load contacts
		ABAddressBookRef addressBook = ABAddressBookCreate();
		NSArray *addrList = (NSArray *) ABAddressBookCopyArrayOfAllPeople(addressBook);
		
		//create temp unsorted array
		NSMutableArray *unsortedArray = [NSMutableArray arrayWithCapacity:[addrList count]];
		for (id person in addrList) {
			[unsortedArray addObject:[ABContact contactWithRecord:(ABRecordRef)person]];
		}
		
		[addrList release];
		
		//sort data
		NSSortDescriptor *lastNameDescriptor = [[[NSSortDescriptor alloc] initWithKey:@"lastname" ascending:YES
							   selector:@selector(localizedCaseInsensitiveCompare:)] autorelease];
		
		NSSortDescriptor *firstNameDescriptor = [[[NSSortDescriptor alloc]
								initWithKey:@"firstname"
								ascending:YES
								selector:@selector(localizedCaseInsensitiveCompare:)] autorelease];
		NSArray *sortDescriptors = [NSArray arrayWithObjects:lastNameDescriptor,
						   firstNameDescriptor, nil];
		self.contactList = [unsortedArray sortedArrayUsingDescriptors:sortDescriptors];
		
		
	}
	
	[super viewWillAppear:animated];
	
}



- (NSInteger)numberOfSectionsInTableView:(UITableView *)aTableView{
	return 1;
	
}

- (NSInteger)tableView:(UITableView *)aTableView numberOfRowsInSection:(NSInteger) section{
	
	return [self.contactList count];
	
}

- (UITableViewCell *)tableView:(UITableView *)aTableView cellForRowAtIndexPath:(NSIndexPath *) indexPath{
	
	//use a cell with subtitle
	UITableViewCellStyle style = UITableViewCellStyleSubtitle;
	UITableViewCell *cell = [aTableView dequeueReusableCellWithIdentifier:@"myCell"];
	if(!cell){
		cell = [[[UITableViewCell alloc] initWithStyle:style reuseIdentifier:@"myCell"] autorelease];
	}
	
	
	[cell setAccessoryType:UITableViewCellAccessoryDisclosureIndicator];
	cell.editingAccessoryType = UITableViewCellAccessoryNone;
	
	cell.textLabel.font = [UIFont fontWithName:@"Arial-BoldMT" size:16];
	
	ABContact *contact = [self.contactList objectAtIndex:[indexPath row]];
	
	NSString *firstname = @"";
	NSString *lastname = @"";
	if (contact.firstname) {
		firstname = [NSString stringWithFormat:@"%@ ",contact.firstname];
	}
	if (contact.lastname) {
		lastname = [NSString stringWithString:contact.lastname];
	}
	
	cell.textLabel.text = [NSString stringWithFormat:@"%@%@", firstname, lastname]; 	
	[cell.detailTextLabel setFont:[UIFont fontWithName:@"Arial" size:12]];
	

	cell.detailTextLabel.text = contact.phonenumbers;

	
	cell.detailTextLabel.textColor = [UIColor grayColor];
	
	//draw contact image or placeholder
	
	UIImage *image;
	
	UIGraphicsBeginImageContext(CGSizeMake(45.0f, 45.0f));
	if(contact.image){
		//contact has an image
		image = contact.image;
		
	}else{
		//use placeholder image
		image = [UIImage imageNamed:@"person_placeholder.png"];
		
	}
	[image drawInRect:CGRectMake(0.0f, 0.0f, 45.0f, 45.0f)];
	
	UIImage *img = UIGraphicsGetImageFromCurrentImageContext();
	UIGraphicsEndImageContext();
	
	//add image to the cel
	cell.imageView.image = img;
	
	return cell;
}



- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
	
	//if contact has only one number the goto contact detail right away
    ABContact *contact = [self.contactList objectAtIndex:[indexPath row]];
	if ([[contact phoneArray] count] == 1 ) {
		//only one number found
		
		//check if there is a network connection available
		if (![DnsResolver connectedToNetwork]) {
			//not network, no lookup. show error mesg
			[ModalAlert alertWith:NSLocalizedString(@"error", nil) andMessage:NSLocalizedString(@"ERROR.NO.NETWORK", nil)];
			return;
		}
		
		//show the activity wheel
		AppDelegate *appDelegate= (AppDelegate *) [[UIApplication sharedApplication] delegate];
		[appDelegate showActivityViewer];
		
		//load data, this is done on seperate thead, when the thread is done
		//the thread will use the callback mehod for notification
		
		self.profile = [[ContactProfile alloc] initWithNumber:[contact phonenumbers]];
		self.profile.delegate = self;
		//start loading profile, will be done in separate thread, when loading is
		//done the profileProcessed: method is called.
		[self.profile loadProfile];
		
		
	}else {
		//contact has multiple numbers goto number selection view
		NumberSelectionController *nsc = [[NumberSelectionController alloc] initWithStyle:UITableViewStyleGrouped];
		nsc.contact = contact;
		
		[self.navigationController pushViewController:nsc animated:YES];
		[nsc release];
	}
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
