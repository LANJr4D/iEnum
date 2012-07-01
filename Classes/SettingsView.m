    //
//  InstellingenView.m
//  DomeinZoeker
//
//  Created by Maarten Wullink on 01-05-10.
//  Copyright 2010 SIDN. All rights reserved.
//

#import "SettingsView.h"


@implementation SettingsView

@synthesize settings;

int selectedField = -1;

- (void) awakeFromNib{
	[self.navigationController.navigationBar setTintColor:[[[UIColor alloc] initWithRed:15.0 / 255 green:107.0 / 255 blue:172.0 / 255 alpha:1.0] autorelease]];
	self.title = NSLocalizedString(@"SETTINGS.TAB.TITLE", nil);
	
	UIView *subView = [self.tableView viewWithTag:101];
	subView.layer.cornerRadius = 10;
	subView.backgroundColor = [[UIColor alloc] initWithRed:202.0/255 green:225.0/255 blue:241.0/255 alpha:0.3];
	
	self.tableView.backgroundColor = [UIColor clearColor];
	self.parentViewController.view.backgroundColor = [UIColor colorWithPatternImage:[UIImage imageNamed:@"settings_background.png"]];
	
	self.title = NSLocalizedString(@"SETTINGS.TAB.TITLE", nil);
}


- (NSInteger)numberOfSectionsInTableView:(UITableView *)aTableView{
	return 2;
	
}

- (NSInteger)tableView:(UITableView *)aTableView numberOfRowsInSection:(NSInteger) section{
	
	if(section == 0){
		return 3;
	}
	
	return 1;
	
}



-(void)viewWillAppear:(BOOL)animated{
	if (!self.settings) {
		//copy settings from app delegate
		self.settings = [[[AppSettings alloc] init] autorelease];
		AppDelegate *appDelegate= (AppDelegate *) [[UIApplication sharedApplication] delegate];
		self.settings.server = appDelegate.settings.server;
		self.settings.suffix = appDelegate.settings.suffix;
		self.settings.countrycode = appDelegate.settings.countrycode;
	}
}


- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
	
 	if (indexPath.section == 1) {
		
		UITableViewCell *cell = [tableView dequeueReusableCellWithIdentifier:@"saveCell"];
		if (cell == nil) {
			cell = [[[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:@"saveCell"] autorelease];
			cell.textLabel.font = [UIFont fontWithName:@"Arial-BoldMT" size:16];
			//cell.backgroundColor = [[UIColor alloc] initWithRed:0.0 / 255 green:72.0 / 255 blue:105.0 / 255 alpha:1.0];
			cell.backgroundColor = [[UIColor alloc] initWithRed:15.0 / 255 green:107.0 / 255 blue:172.0 / 255 alpha:1.0];
			cell.textLabel.textColor = [UIColor whiteColor];
			
		}

		cell.textLabel.text = NSLocalizedString(@"SETTINGS.BUTTON.SAVE", nil);
		return cell;
	}

	UITableViewCell *cell = nil;
	
	if (cell == nil) {
        cell = [[[UITableViewCell alloc] initWithStyle:UITableViewCellStyleValue1 reuseIdentifier:@"settingCell"] autorelease];
		cell.textLabel.font = [UIFont fontWithName:@"Arial-BoldMT" size:14];
		cell.detailTextLabel.font = [UIFont fontWithName:@"Arial-BoldMT" size:13];
		
		[cell setAccessoryType:UITableViewCellAccessoryDisclosureIndicator];
		cell.editingAccessoryType = UITableViewCellAccessoryNone;
    }
	
	switch (indexPath.row) {
		case 0:
			cell.textLabel.text = NSLocalizedString(@"SETTINGS.DNS.SERVER", nil);
			if(self.settings.server){
				cell.detailTextLabel.text = self.settings.server;
			}else {
				cell.detailTextLabel.text = NSLocalizedString(@"SETTINGS.DNS.SERVER.ENTER", nil);
			}
			break;
		case 1:
			if(self.settings.suffix){
				cell.detailTextLabel.text = self.settings.suffix;
			}else {
				cell.detailTextLabel.text = NSLocalizedString(@"SETTINGS.SUFFIX.ENTER", nil);
			}
			cell.textLabel.text = NSLocalizedString(@"SETTINGS.SUFFIX", nil);
	
			
			break;
		case 2:
			if(self.settings.countrycode){
				cell.detailTextLabel.text = self.settings.countrycode;
			}else {
				cell.detailTextLabel.text = NSLocalizedString(@"SETTINGS.COUNTRY.ENTER", nil);
			}
			cell.textLabel.text = NSLocalizedString(@"SETTINGS.COUNTRY", nil);
			
			break;
			

		default:
			break;
	}
    
    return cell;
}


- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath{
	if(indexPath.section == 0){
	
		NSString *title = @"";
		NSString *fieldTextValue = @"";
		switch (indexPath.row) {
			case 0:
			 title =  NSLocalizedString(@"SETTINGS.DNS.SERVER.ENTER", nil);
				if(self.settings.server){
					fieldTextValue = self.settings.server;
				}
				selectedField = 0;
				break;
			case 1:
				title = NSLocalizedString(@"SETTINGS.SUFFIX.ENTER", nil);
				if(self.settings.suffix){
					fieldTextValue = self.settings.suffix;
				}
				selectedField = 1;
				break;
			case 2:
				title = NSLocalizedString(@"SETTINGS.COUNTRY.ENTER", nil);	
				if(self.settings.countrycode){
					fieldTextValue = self.settings.countrycode;
				}
				selectedField = 2;
				break;
			default:
				break;	
		}
		SettingsViewEditorController *sc = [[SettingsViewEditorController alloc] initWithNibName:@"SettingEditorView" bundle:nil fieldTitle:title fieldText:fieldTextValue];		
		sc.delegate = self;
		[self.navigationController pushViewController:sc animated:YES];

	}

	if(indexPath.section == 1){
		NSLog(@"saveSettings server:%@ country:%@ suffix:%@", self.settings.server, self.settings.countrycode, self.settings.suffix);
		//save the settings button pressed
		//copy settings to the appdelegate
		
		AppDelegate *appDelegate= (AppDelegate *) [[UIApplication sharedApplication] delegate];
		appDelegate.settings.server = self.settings.server;
		appDelegate.settings.suffix = self.settings.suffix;
		appDelegate.settings.countrycode = self.settings.countrycode;
		
		[appDelegate saveSettings];
		
	}

}

-(void)settingUpdated:(NSString *)settingValue{
	NSLog(@"settingUpdated %@ for field %d", settingValue, selectedField);
	if (selectedField == 0) {
		//server modified
		self.settings.server = [self getValue:settingValue];
	}else if (selectedField == 1) {
		//suffix modified
		self.settings.suffix = [self getValue:settingValue];
	}else if (selectedField == 2) {
		//country code modified
		self.settings.countrycode = [self getValue:settingValue];
	}else {
		NSLog(@"unknown setting");
	}

	
	[self.tableView reloadData];
}

-(NSString *)getValue:(NSString *)value{
	if (value) {
		if ([value length] > 0) {
			return value;
		}
	}
	
	return nil;
}


@end
