#import "MWMSearch.h"
#import <Crashlytics/Crashlytics.h>
#import "Common.h"
#import "MWMAlertViewController.h"
#import "MWMLocationManager.h"
#import "MWMSearchHotelsFilterViewController.h"
#import "ToastView.h"

#include "Framework.h"

#include "search/everywhere_search_params.hpp"
#include "search/hotels_classifier.hpp"
#include "search/query_saver.hpp"
#include "search/viewport_search_params.hpp"

namespace
{
using TObserver = id<MWMSearchObserver>;
using TObservers = NSHashTable<__kindof TObserver>;
}  // namespace

@interface MWMSearch ()

@property(nonatomic) NSUInteger suggestionsCount;
@property(nonatomic) BOOL searchOnMap;

@property(nonatomic) BOOL textChanged;

@property(nonatomic) TObservers * observers;

@property(nonatomic) NSTimeInterval lastSearchTimestamp;

@property(nonatomic) BOOL isHotelResults;
@property(nonatomic) MWMSearchFilterViewController * filter;

@end

@implementation MWMSearch
{
  search::EverywhereSearchParams m_everywhereParams;
  search::ViewportSearchParams m_viewportParams;
  search::Results m_results;
  string m_filterQuery;
}

#pragma mark - Instance

+ (MWMSearch *)manager
{
  static MWMSearch * manager;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    manager = [[super alloc] initManager];
  });
  return manager;
}

- (instancetype)initManager
{
  self = [super init];
  if (self)
    _observers = [TObservers weakObjectsHashTable];
  return self;
}

- (void)updateCallbacks
{
  NSTimeInterval const timestamp = [NSDate date].timeIntervalSince1970;
  self.lastSearchTimestamp = timestamp;
  {
    __weak auto weakSelf = self;
    m_everywhereParams.m_onResults = [weakSelf, timestamp](search::Results const & results) {
      __strong auto self = weakSelf;
      if (!self)
        return;
      if (timestamp != self.lastSearchTimestamp)
        return;
      if (results.IsEndMarker())
      {
        [self checkIsHotelResults:results];
        [self onSearchCompleted];
      }
      else
      {
        self->m_results = results;
        self.suggestionsCount = results.GetSuggestsCount();
        [self onSearchResultsUpdated];
      }
    };
  }
  {
    __weak auto weakSelf = self;
    m_viewportParams.m_onStarted = [weakSelf] {
      __strong auto self = weakSelf;
      if (!self)
        return;
      if (IPAD)
        GetFramework().SearchEverywhere(self->m_everywhereParams);
      [self onSearchStarted];
    };
  }
  {
    __weak auto weakSelf = self;
    m_viewportParams.m_onCompleted = [weakSelf](search::Results const & results) {
      __strong auto self = weakSelf;
      if (!self)
        return;
      if (results.IsEndedNormal())
      {
        [self checkIsHotelResults:results];
        if (results.GetCount() == 0)
          [[MWMAlertViewController activeAlertController] presentSearchNoResultsAlert];
      }
      [self onSearchCompleted];
    };
  }
}

- (void)checkIsHotelResults:(search::Results const &)results
{
  self.isHotelResults = search::HotelsClassifier::IsHotelResults(results);
  m_filterQuery = m_everywhereParams.m_query;
}

- (void)updateFilters
{
  shared_ptr<search::hotels_filter::Rule> hotelsRules;

  if (self.filter)
  {
    hotelsRules = [self.filter rules];
    if (!hotelsRules)
      self.filter = nil;
  }

  m_viewportParams.m_hotelsFilter = hotelsRules;
  m_everywhereParams.m_hotelsFilter = hotelsRules;
}

- (void)update
{
  [MWMSearch clear];
  if (m_everywhereParams.m_query.empty())
    return;
  [self updateCallbacks];
  [self updateFilters];
  auto & f = GetFramework();
  if (IPAD)
  {
    f.SearchEverywhere(m_everywhereParams);
    f.SearchInViewport(m_viewportParams);
  }
  else
  {
    if (self.searchOnMap)
      f.SearchInViewport(m_viewportParams);
    else
      f.SearchEverywhere(m_everywhereParams);
  }
  [self onSearchStarted];
}

#pragma mark - Add/Remove Observers

+ (void)addObserver:(id<MWMSearchObserver>)observer
{
  [[MWMSearch manager].observers addObject:observer];
}

+ (void)removeObserver:(id<MWMSearchObserver>)observer
{
  [[MWMSearch manager].observers removeObject:observer];
}

#pragma mark - Methods

+ (void)saveQuery:(NSString *)query forInputLocale:(NSString *)inputLocale
{
  if (!query || query.length == 0)
    return;
  CLS_LOG(@"Save search text: %@\nInputLocale: %@", query, inputLocale);
  string const locale = (!inputLocale || inputLocale.length == 0)
                            ? [MWMSearch manager]->m_everywhereParams.m_inputLocale
                            : inputLocale.UTF8String;
  string const text = query.precomposedStringWithCompatibilityMapping.UTF8String;
  GetFramework().SaveSearchQuery(make_pair(locale, text));
}

+ (void)searchQuery:(NSString *)query forInputLocale:(NSString *)inputLocale
{
  if (!query)
    return;
  CLS_LOG(@"Search text: %@\nInputLocale: %@", query, inputLocale);
  MWMSearch * manager = [MWMSearch manager];
  if (inputLocale.length != 0)
  {
    string const locale = inputLocale.UTF8String;
    manager->m_everywhereParams.m_inputLocale = locale;
    manager->m_viewportParams.m_inputLocale = locale;
  }
  string const text = query.precomposedStringWithCompatibilityMapping.UTF8String;
  manager->m_everywhereParams.m_query = text;
  manager->m_viewportParams.m_query = text;
  manager.textChanged = YES;
  [manager update];
}

+ (void)showResult:(search::Result const &)result { GetFramework().ShowSearchResult(result); }

+ (search::Result &)resultAtIndex:(NSUInteger)index
{
  return [MWMSearch manager]->m_results.GetResult(index);
}

+ (void)update { [[MWMSearch manager] update]; }

+ (void)clear
{
  GetFramework().CancelAllSearches();
  MWMSearch * manager = [MWMSearch manager];
  manager->m_results.Clear();
  if (manager->m_filterQuery != manager->m_everywhereParams.m_query)
    manager.isHotelResults = NO;
  manager.suggestionsCount = 0;
  [manager onSearchResultsUpdated];
}

+ (BOOL)isSearchOnMap { return [MWMSearch manager].searchOnMap; }

+ (void)setSearchOnMap:(BOOL)searchOnMap
{
  MWMSearch * manager = [MWMSearch manager];
  if (manager.searchOnMap == searchOnMap)
    return;
  manager.searchOnMap = searchOnMap;
  if (!IPAD)
    [manager update];
}

+ (NSUInteger)suggestionsCount { return [MWMSearch manager].suggestionsCount; }

+ (NSUInteger)resultsCount { return [MWMSearch manager]->m_results.GetCount(); }

+ (BOOL)isHotelResults { return [MWMSearch manager].isHotelResults; }

#pragma mark - Filters

+ (BOOL)hasFilter { return [[MWMSearch manager].filter rules] != nullptr; }

+ (MWMSearchFilterViewController *)getFilter
{
  MWMSearch * manager = [MWMSearch manager];
  if (!manager.filter && manager.isHotelResults)
    manager.filter = [MWMSearchHotelsFilterViewController controller];
  return manager.filter;
}

+ (void)clearFilter
{
  MWMSearch * manager = [MWMSearch manager];
  manager.filter = nil;
  [manager update];
}

#pragma mark - Notifications

- (void)onSearchStarted
{
  for (TObserver observer in self.observers)
  {
    if ([observer respondsToSelector:@selector(onSearchStarted)])
      [observer onSearchStarted];
  }
}

- (void)onSearchCompleted
{
  if (self.searchOnMap && m_results.GetCount() == 0)
    [[[ToastView alloc] initWithMessage:L(@"search_not_found_query")] show];
  for (TObserver observer in self.observers)
  {
    if ([observer respondsToSelector:@selector(onSearchCompleted)])
      [observer onSearchCompleted];
  }
}

- (void)onSearchResultsUpdated
{
  for (TObserver observer in self.observers)
  {
    if ([observer respondsToSelector:@selector(onSearchResultsUpdated)])
      [observer onSearchResultsUpdated];
  }
}

@end
