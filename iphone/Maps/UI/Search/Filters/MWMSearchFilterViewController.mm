#import "MWMSearchFilterViewController_Protected.h"

@implementation MWMSearchFilterViewController

+ (MWMSearchFilterViewController *)controller
{
  // Must be implemented in subclasses.
  [self doesNotRecognizeSelector:_cmd];
  return nil;
}

+ (MWMSearchFilterViewController *)controllerWithIdentifier:(NSString *)identifier
{
  auto storyboard = [UIStoryboard storyboardWithName:@"MWMSearchFilters" bundle:nil];
  return [storyboard instantiateViewControllerWithIdentifier:identifier];
}

- (void)mwm_refreshUI { [self.view mwm_refreshUI]; }
- (shared_ptr<search::hotels_filter::Rule>)rules { return nullptr; }
@end
