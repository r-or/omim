#import "MWMNetworkPolicy.h"
#import "MWMAlertViewController.h"

#include "platform/platform.hpp"

using np = platform::NetworkPolicy;

namespace
{
NSString * const kNetworkingPolicyTimeStamp = @"NetworkingPolicyTimeStamp";
NSTimeInterval const kSessionDurationSeconds = 24 * 60 * 60;
}  // namespace

namespace network_policy
{
void CallPartnersApi(platform::PartnersApiFn fn, bool force)
{
  auto const connectionType = GetPlatform().ConnectionStatus();
  if (connectionType == Platform::EConnectionType::CONNECTION_NONE)
  {
    fn(false);
    return;
  }
  if (force || connectionType == Platform::EConnectionType::CONNECTION_WIFI)
  {
    fn(true);
    return;
  }

  auto checkAndApply = ^bool {
    NSUserDefaults * ud = [NSUserDefaults standardUserDefaults];
    NSDate * policyDate = [ud objectForKey:kNetworkingPolicyTimeStamp];
    if ([policyDate compare:[NSDate date]] == NSOrderedDescending)
    {
      fn(true);
      return true;
    }
    if ([policyDate isEqualToDate:NSDate.distantPast])
    {
      fn(false);
      return true;
    }
    return false;
  };

  if (checkAndApply())
    return;

  MWMAlertViewController * alertController = [MWMAlertViewController activeAlertController];
  [alertController presentMobileInternetAlertWithBlock:^{
    if (!checkAndApply())
      fn(false);
  }];
}

void SetStage(np::Stage state)
{
  NSUserDefaults * ud = [NSUserDefaults standardUserDefaults];
  NSDate * policyDate = nil;
  switch (state)
  {
  case np::Stage::Always: policyDate = NSDate.distantFuture; break;
  case np::Stage::Session:
    policyDate = [NSDate dateWithTimeIntervalSinceNow:kSessionDurationSeconds];
    break;
  case np::Stage::Never: policyDate = NSDate.distantPast; break;
  }
  [ud setObject:policyDate forKey:kNetworkingPolicyTimeStamp];
}

np::Stage GetStage()
{
  NSUserDefaults * ud = [NSUserDefaults standardUserDefaults];
  NSDate * policyDate = [ud objectForKey:kNetworkingPolicyTimeStamp];
  if ([policyDate isEqualToDate:NSDate.distantFuture])
    return np::Stage::Always;
  if ([policyDate isEqualToDate:NSDate.distantPast])
    return np::Stage::Never;
  return np::Stage::Session;
}
}  // namespace network_policy
