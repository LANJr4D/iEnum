//
//  OverzichtViewController.h
//  DomeinZoeker
//
//  Created by Maarten Wullink on 23-04-10.
//  Copyright 2010 SIDN. All rights reserved.
//

#import <UIKit/UIKit.h>




@interface OverzichtViewController : UIViewController{
	IBOutlet UILabel *lblOverzicht;
	//IBOutlet OverzichtTableController *tblOverview;
}

@property(nonatomic,retain) IBOutlet UILabel *lblOverzicht;


- (IBAction)doTestButton:(id)sender;

- (IBAction)doPush:(id)sender;

- (void)doParseData:(NSData *)xmlData;

@end
