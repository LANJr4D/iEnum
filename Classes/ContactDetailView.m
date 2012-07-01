//
//  Test.m
//  iEnum
//
//  Created by Maarten Wullink on 03-01-11.
//  Copyright 2011 SIDN. All rights reserved.
//

#import "ContactDetailView.h"


@implementation ContactDetailView

@synthesize number;
@synthesize profile;



- (id)initWithFrame:(CGRect)rect andProfile:(ContactProfile *)aProfile {
    self = [super initWithFrame:rect];
    if (self) {
		self.profile = aProfile;
		[self load];
    }
    return self;
}


- (void)load {
	self.backgroundColor = [UIColor colorWithPatternImage:[UIImage imageNamed:@"contact_details_background_10.png"]];
	[self createGui];
	[self createBottomButtons];
}


 
 
-(void)createBottomButtons{
	//init scrolling area
	UIScrollView *scrollView = [[UIScrollView alloc] initWithFrame:CGRectMake(20, 283, 280, 75)];	
	UIEdgeInsets anEdgeInset = { 0, 0, 0, 0};
	scrollView.scrollIndicatorInsets = anEdgeInset;
	
	scrollView.bounces = YES;
	scrollView.backgroundColor = [UIColor clearColor];
	
	NSEnumerator *enumerator = [self.profile.enumServices objectEnumerator];
	EnumService *service;
	int count = 0;
	while(service = [enumerator nextObject])
    {
		// Do your thing with the object.
		UIButton *btn = [self createButtonWithX:10+count*40 andImage:service.icon ];
		//add a tag id to the button, this can be used to identify which button is pressed by the user
		btn.tag = 900+count;
		count++;
		
		[scrollView addSubview:btn];
    }
	
	scrollView.contentSize = CGSizeMake((count*40)+20, 75);
	[self addSubview:scrollView];
	[scrollView release];
}


-(UIButton *)createButtonWithX:(int)xPos andImage:(UIImage *) image{
	UIButton *btn = [[UIButton buttonWithType:UIButtonTypeCustom] retain];
	btn.frame = CGRectMake(xPos, 0, 32, 32);
	btn.bounds = CGRectMake(xPos, 0, 32.0, 32.0);
	[btn setImage:image forState:UIControlStateNormal];
	
	UIViewController *controller = [self firstAvailableUIViewController];
	[btn addTarget:controller action:@selector(serviceButtonPressed:) forControlEvents:UIControlEventTouchUpInside];
	return btn;
}



- (UIViewController *) firstAvailableUIViewController {
    // convenience function for casting and to "mask" the recursive function
    return (UIViewController *)[self traverseResponderChainForUIViewController];
}

- (id) traverseResponderChainForUIViewController {
    id nextResponder = [self nextResponder];
    if ([nextResponder isKindOfClass:[UIViewController class]]) {
        return nextResponder;
    } else if ([nextResponder isKindOfClass:[UIView class]]) {
        return [nextResponder traverseResponderChainForUIViewController];
    } else {
        return nil;
    }
}



-(UIImageView *)createServicesIcon:(BOOL)active andRect:(CGRect)rect{
	
	UIImage *img = nil;
	
	if(active){
		img =  [UIImage imageNamed:@"Star-32.png"];
	}else {
		img =  [UIImage imageNamed:@"Warning-32.png"];
	}
	
	UIImageView *iview = [[[UIImageView alloc] initWithFrame:rect] autorelease];
	iview.image = img;
	
	return iview;
}

-(void)createServicesPart{
	//services label
	UILabel *servicesLbl = [self createLabelWithRect:CGRectMake(130, 15, 100, 20) andText:NSLocalizedString(@"CONTACT.DETAILS.TAB.SERVICES", nil) andFont:[UIFont fontWithName:@"Arial-BoldMT" size:16]]; 
	[self addSubview:servicesLbl];
	[servicesLbl release];
	
	//web sites label
	UILabel *webLbl = [self createLabelWithRect:CGRectMake(130, 35, 100, 20) andText:NSLocalizedString(@"CONTACT.DETAILS.TAB.WEB.LOC", nil)  andFont:[UIFont fontWithName:@"Arial" size:12]]; 
	[self addSubview:webLbl];
	[webLbl release];
	
	BOOL webFound = [[RecordUtil getAllWeblocations:self.profile.naptrList] count] > 0;
	[self addSubview:[self createServicesIcon:webFound andRect:CGRectMake(265, 35, 20, 20)]];
	
	//secure email label
	UILabel *mailLbl = [self createLabelWithRect:CGRectMake(130, 55, 100, 20) andText:NSLocalizedString(@"CONTACT.DETAILS.TAB.SEC.EMAIL", nil)  andFont:[UIFont fontWithName:@"Arial" size:12]]; 
	[self addSubview:mailLbl];
	[mailLbl release];
	
	BOOL keyFound = [self.profile isKeyAvailable];
	[self addSubview:[self createServicesIcon:keyFound andRect:CGRectMake(265, 55, 20, 20)]];
	
	//address label
	UILabel *addrLbl = [self createLabelWithRect:CGRectMake(130, 75, 100, 20) andText:NSLocalizedString(@"CONTACT.DETAILS.TAB.ADDRESS", nil)  andFont:[UIFont fontWithName:@"Arial" size:12]]; 
	[self addSubview:addrLbl];
	[addrLbl release];
	
	//if vcard is present and status is yes (parsed ok) then vcard service is ok
	BOOL addressFound =  (self.profile.vcard != nil) && (self.profile.vcard.status == YES);
	[self addSubview:[self createServicesIcon:addressFound andRect:CGRectMake(265, 75, 20, 20)]];
	
	//map label
	UILabel *mapLbl = [self createLabelWithRect:CGRectMake(130, 95, 100, 20) andText:NSLocalizedString(@"CONTACT.DETAILS.TAB.MAP.LOC", nil)  andFont:[UIFont fontWithName:@"Arial" size:12]]; 
	[self addSubview:mapLbl];
	[mapLbl release];
	
	BOOL locationFound =  [self.profile isLocationAvailable];
	[self addSubview:[self createServicesIcon:locationFound andRect:CGRectMake(265, 95, 20, 20)]];
}


-(void)createAddressPart{
	
	BOOL vcardOk =  (self.profile.vcard != nil) && (self.profile.vcard.status == YES);
	if (!vcardOk) {
		UILabel *nameLbl = [self createLabelWithRect:CGRectMake(20, 150, 200, 20) andText:NSLocalizedString(@"CONTACT.DETAILS.ADDRESS.NOT.FOUND", nil) andFont:[UIFont fontWithName:@"Arial-BoldMT" size:16]]; 
		nameLbl.textColor = [UIColor whiteColor];
		nameLbl.tag = 101;
		[self addSubview:nameLbl];
		[nameLbl release];
	}else{
		UILabel *nameLbl = [self createLabelWithRect:CGRectMake(20, 150, 200, 20) andText:self.profile.vcard.fullname andFont:[UIFont fontWithName:@"Arial-BoldMT" size:16]]; 
		nameLbl.textColor = [UIColor whiteColor];
		nameLbl.tag = 101;
		[self addSubview:nameLbl];
		[nameLbl release];
		
		UILabel *addr1Lbl = [self createLabelWithRect:CGRectMake(20, 165, 200, 20) andText:self.profile.vcard.street andFont:[UIFont fontWithName:@"Arial" size:12]]; 
		addr1Lbl.tag = 102;
		[self addSubview:addr1Lbl];
		[addr1Lbl release];
		
		NSString *addr3 = [NSString stringWithFormat:@"%@ %@", self.profile.vcard.postalCode, self.profile.vcard.city];
		UILabel *addr2Lbl = [self createLabelWithRect:CGRectMake(20, 180, 200, 20) andText:addr3 andFont:[UIFont fontWithName:@"Arial" size:12]]; 
		addr2Lbl.tag = 103;
		[self addSubview:addr2Lbl];
		[addr2Lbl release];
		
		
		UILabel *addr3Lbl = [self createLabelWithRect:CGRectMake(20, 195, 200, 20) andText:self.profile.vcard.country andFont:[UIFont fontWithName:@"Arial" size:12]]; 
		addr3Lbl.tag = 104;
		[self addSubview:addr3Lbl];
		[addr3Lbl release];
	}

}


-(void)createGui{
	
	[self createServicesPart];
	
	[self createAddressPart];
	
	UIImage *img = nil;
	if(self.profile.vcard.photo){
		img = self.profile.vcard.photo;
	}else {
		img =  [UIImage imageNamed:@"person_placeholder.png"];
	}
	
	UIImageView *iview = [[[UIImageView alloc] initWithFrame:CGRectMake(20, 12, 90, 122)] autorelease];
	[iview.layer setBorderColor: [[UIColor blackColor] CGColor]];
	[iview.layer setBorderWidth: 2.0];
	iview.image = img;
	iview.layer.cornerRadius = 5.0;
	iview.layer.masksToBounds = YES;
	iview.layer.borderColor = [UIColor whiteColor].CGColor;
	[self addSubview:iview];
}

-(UILabel *) createLabelWithRect:(CGRect)rect andText:(NSString *)text andFont:(UIFont *)font{
	UILabel *lbl = [[UILabel alloc] initWithFrame:rect];
	lbl.text = text;
	lbl.textAlignment = UITextAlignmentLeft;
	lbl.textColor = [UIColor whiteColor];
	lbl.backgroundColor = [UIColor colorWithRed:0.0 green:0.0 blue:0.0 alpha:0.0];
	lbl.font = font;
	
	return lbl;
	
} 



@end
