/*****************************************************************************
 * Plus42 -- an enhanced HP-42S calculator simulator
 * Copyright (C) 2004-2025  Thomas Okken
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see http://www.gnu.org/licenses/.
 *****************************************************************************/

#import <Cocoa/Cocoa.h>

@interface StateNameWindow : NSWindow {
    NSTextField *label;
    NSTextField *stateName;
    NSMutableArray *existingNames;
    BOOL confirmed;
}

@property (nonatomic, retain) IBOutlet NSTextField *label;
@property (nonatomic, retain) IBOutlet NSTextField *stateName;

- (void) setupWithLabel:(NSString *)label initialName:(NSString *)name existingNames:(NSMutableArray *)names;
- (NSString *) selectedName;
- (IBAction) ok:(id)sender;
- (IBAction) cancel:(id)sender;

@end
