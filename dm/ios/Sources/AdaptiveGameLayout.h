#pragma once
#import <UIKit/UIKit.h>
#import "AdaptiveLayoutMath.h"
// Hinge capability gates all Duo-only placement changes. Layout uses current view geometry.
void acnh_observe_display(UIView *view);
BOOL acnh_has_hinge(UIView *view);
int acnh_display_mode(void);
BOOL acnh_expanded_view_active(UIView *view);
CGRect acnh_game_viewport(UIView *view);
CGRect acnh_game_frame(UIView *view);
CGRect acnh_pad_frame(UIView *view);
BOOL acnh_tabletop_active(UIView *view);
CGPoint acnh_control_point(UIView *view,CGPoint point,CGFloat radius);
CGFloat acnh_view_display_scale(UIView *view);
