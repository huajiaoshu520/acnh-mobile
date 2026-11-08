#import <UIKit/UIKit.h>
BOOL acnh_pad_editing(void);
void acnh_pad_use_geometry(UIView *view);
BOOL acnh_pad_custom(NSString *base);
CGFloat acnh_pad_scale(NSString *base, NSString *key);
CGPoint acnh_pad_point(NSString *base, NSString *key, CGPoint original, UIView *view, CGFloat margin);
CGPoint acnh_pad_point_avoiding(NSString *base, NSString *key, CGPoint original, UIView *view, CGFloat margin, CGRect peer);
NSDictionary *acnh_pad_control(NSString *key, NSString *name, CGPoint center, CGFloat radius);
@protocol ACNHEditablePad <NSObject>
- (NSArray<NSDictionary *> *)acnhEditableControls;
@end
UIView *dm_acnh_gamepad_preview(CGRect frame);
UIView *dm_generic_gamepad_view(CGRect frame);
