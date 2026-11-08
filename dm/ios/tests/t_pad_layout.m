#import "../Sources/PadLayoutMath.h"
#include <assert.h>
int main(void){@autoreleasepool{
 CGRect b=CGRectMake(0,0,932,430),safe=CGRectMake(59,0,814,409);CGPoint original=CGPointMake(110,327);
 assert(CGPointEqualToPoint(pad_point(@{},@"stick",original,b,safe,77),original));
 assert(!pad_has_position(nil));
 assert(!pad_has_position(@{@"scale":@1.1}));
 assert(!pad_has_position(@{@"x":@0.2,@"y":@(NAN),@"dx":@0,@"dy":@0}));
 assert(pad_has_position(@{@"x":@0,@"y":@0}));
 assert(pad_has_position(@{@"dx":@0.2,@"scale":@1.1}));
 assert(pad_has_position(@{@"dy":@(-0.1)}));
 NSMutableDictionary *draft=[@{} mutableCopy];draft[@"stick"]=@{@"dx":@.2,@"dy":@-.1,@"scale":@1.5};
 assert(pad_scale(draft,@"stick")==1.5);CGPoint p=pad_point(draft,@"stick",original,b,safe,77);assert(fabs(p.x-296.4)<.001 && fabs(p.y-284)<.001);
 assert(pad_scale(draft,@"button.0")==1);assert(CGPointEqualToPoint(pad_point(draft,@"button.0",original,b,safe,77),original));
 NSDictionary *saved=draft.copy;[draft removeAllObjects];assert(pad_scale(saved,@"stick")==1.5);assert(pad_scale(draft,@"stick")==1);assert(CGPointEqualToPoint(pad_point(draft,@"stick",original,b,safe,77),original));
 for(id bad in @[@"broken",@{@"scale":@100},@{@"scale":@(-100)},@{@"scale":@(NAN)},@{@"scale":@"invalid"}]){CGFloat n=pad_scale(@{@"stick":bad},@"stick");assert(n>=.5 && n<=2);}
 p=pad_point(@{@"stick":@{@"dx":@100,@"dy":@(-100)}},@"stick",original,b,safe,77);assert(p.x<=CGRectGetMaxX(safe)-79 && p.y>=79);
 NSString *suite=@"org.acnhmobile.layout-regression";NSUserDefaults *defaults=[[NSUserDefaults alloc]initWithSuiteName:suite];[defaults removePersistentDomainForName:suite];
 [defaults setObject:saved forKey:@"acnh"];[defaults setObject:@{} forKey:@"generic"];assert(pad_scale([defaults dictionaryForKey:@"acnh"],@"stick")==1.5);assert(pad_scale([defaults dictionaryForKey:@"generic"],@"stick")==1);[defaults removePersistentDomainForName:suite];
 // Changing orientation/template must not overwrite another saved layout.
 assert([pad_store_key(@"acnh",@"") isEqual:@"ACNH.CustomPad.v1.acnh"]);
 NSArray *variants=@[@"",@".portrait.ipad",@".portrait.duo"];
 for(NSString *base in @[@"acnh",@"generic"])for(NSUInteger i=0;i<variants.count;i++)
  [defaults setObject:@{@"stick":@{@"scale":@(0.5+i*.5)}} forKey:pad_store_key(base,variants[i])];
 [defaults setObject:@{} forKey:pad_store_key(@"generic",@".portrait.duo")];
 for(NSString *base in @[@"acnh",@"generic"])for(NSUInteger i=0;i<variants.count;i++){
  CGFloat expected=([base isEqual:@"generic"]&&i==2)?1:0.5+i*.5;
  assert(pad_scale([defaults dictionaryForKey:pad_store_key(base,variants[i])],@"stick")==expected);
 }
 // Drag far beyond a lower-screen edge; the full button stays in that screen.
 for(NSValue *v in @[[NSValue valueWithSize:NSMakeSize(768,520)],[NSValue valueWithSize:NSMakeSize(430,500)]]){
  NSSize size=v.sizeValue;CGRect lower=CGRectMake(0,0,size.width,size.height);
  for(int sign=-1;sign<=1;sign+=2){CGPoint q=pad_point(@{@"stick":@{@"dx":@(sign*100),@"dy":@(sign*100)}},@"stick",CGPointMake(100,100),lower,lower,60);
   assert(CGRectContainsRect(lower,CGRectMake(q.x-60,q.y-60,120,120)));
  }
 }
 // Portrait editing stores the visible destination. An old extreme offset
 // and a subsequent resize can clamp it, but the next inward drag still moves.
 CGRect lower=CGRectMake(0,0,834,529.94);
 CGFloat margin=100;
 CGPoint old=pad_point(@{@"stick":@{@"dx":@1,@"dy":@(-1)}},@"stick",original,lower,lower,margin);
 assert(old.x==CGRectGetMaxX(lower)-102 && old.y==102);
 CGPoint wanted=pad_clamp_center(CGPointMake(old.x-8,old.y+6),lower,margin);
 NSDictionary *moved=@{@"stick":@{@"dx":@1,@"dy":@(-1),@"x":@(wanted.x/lower.size.width),@"y":@(wanted.y/lower.size.height)}};
 CGPoint restored=pad_point(moved,@"stick",original,lower,lower,margin);
 assert(hypot(restored.x-wanted.x,restored.y-wanted.y)<.001);
 assert(CGRectContainsRect(lower,CGRectMake(restored.x-margin,restored.y-margin,2*margin,2*margin)));
 for(int edge=0;edge<=1;edge++){
  CGPoint p=pad_point(@{@"stick":@{@"x":@(edge),@"y":@(edge)}},@"stick",original,lower,lower,margin);
  assert(CGRectContainsRect(lower,CGRectMake(p.x-margin,p.y-margin,2*margin,2*margin)));
 }
 assert(CGPointEqualToPoint(pad_clamp_center(CGPointZero,CGRectMake(0,0,30,20),100),CGPointMake(15,10)));
 [defaults removePersistentDomainForName:suite];
 puts("layout: exact defaults/reset, normalized movement, independent controls, scale bounds, malformed values and persistence passed");
}}
