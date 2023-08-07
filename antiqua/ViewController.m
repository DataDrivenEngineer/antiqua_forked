//

#import <AppKit/NSScreen.h>

#import "ViewController.h"
#import "CustomNSView.h"

@implementation ViewController

- (void)viewDidLoad {
	[super viewDidLoad];
	
	// Do any additional setup after loading the view.
}

- (void)viewWillDisappear {
	[(CustomNSView *)self.view stopDisplayLink];
}

- (void)viewWillAppear {
	[(CustomNSView *)self.view resumeDisplayLink];
}

- (void)setRepresentedObject:(id)representedObject {
	[super setRepresentedObject:representedObject];

	// Update the view, if already loaded.
}

@end
