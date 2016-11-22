#include "testing/testing.hpp"

#include "generator/generator_tests_support/restriction_helpers.hpp"
#include "generator/generator_tests_support/test_feature.hpp"
#include "generator/generator_tests_support/test_mwm_builder.hpp"

#include "generator/restriction_collector.hpp"
#include "generator/restriction_generator.hpp"

#include "routing/restriction_loader.hpp"

#include "indexer/index.hpp"
#include "indexer/mwm_set.hpp"

#include "coding/file_name_utils.hpp"

#include "platform/platform_tests_support/scoped_dir.hpp"
#include "platform/platform_tests_support/scoped_file.hpp"

#include "platform/country_file.hpp"
#include "platform/platform.hpp"

#include "base/logging.hpp"
#include "base/scope_guard.hpp"

#include "std/string.hpp"

using namespace feature;
using namespace generator;
using namespace platform::tests_support;
using namespace platform;
using namespace routing;

namespace
{
// Directory name for creating test mwm and temporary files.
string const kTestDir = "restriction_generation_test";
// Temporary mwm name for testing.
string const kTestMwm = "test";
string const kRestrictionFileName = "restrictions_in_osm_ids.csv";
string const kOsmIdsToFeatureIdsName = "osm_ids_to_feature_ids" OSM2FEATURE_FILE_EXTENSION;

void BuildEmptyMwm(LocalCountryFile & country)
{
  generator::tests_support::TestMwmBuilder builder(country, feature::DataHeader::country);
}

/// \brief Generates a restriction section, adds it to an empty mwm,
/// loads the restriction section and test loaded restrictions.
/// \param restrictionContent comma separated text with restrictions in osm id terms.
/// \param mappingContent comma separated text with mapping from osm ids to feature ids.
void TestRestrictionBuilding(string const & restrictionContent, string const & mappingContent)
{
  Platform & platform = GetPlatform();
  string const writableDir = platform.WritableDir();

  // Building empty mwm.
  LocalCountryFile country(my::JoinFoldersToPath(writableDir, kTestDir), CountryFile(kTestMwm), 1);
  ScopedDir const scopedDir(kTestDir);
  string const mwmRelativePath = my::JoinFoldersToPath(kTestDir, kTestMwm + DATA_FILE_EXTENSION);
  ScopedFile const scopedMwm(mwmRelativePath);
  BuildEmptyMwm(country);

  // Creating a file with restrictions.
  string const restrictionRelativePath = my::JoinFoldersToPath(kTestDir, kRestrictionFileName);
  ScopedFile const restrictionScopedFile(restrictionRelativePath, restrictionContent);

  // Creating osm ids to feature ids mapping.
  string const mappingRelativePath = my::JoinFoldersToPath(kTestDir, kOsmIdsToFeatureIdsName);
  ScopedFile const mappingScopedFile(mappingRelativePath);
  string const mappingFullPath = my::JoinFoldersToPath(writableDir, mappingRelativePath);
  ReEncodeOsmIdsToFeatureIdsMapping(mappingContent, mappingFullPath);

  // Adding restriction section to mwm.
  string const restrictionFullPath = my::JoinFoldersToPath(writableDir, restrictionRelativePath);
  string const mwmFullPath = my::JoinFoldersToPath(writableDir, mwmRelativePath);
  BuildRoadRestrictions(mwmFullPath, restrictionFullPath, mappingFullPath);

  // Reading from mwm section and testing restrictions.
  Index index;
  auto const regResult = index.RegisterMap(country);
  TEST_EQUAL(regResult.second, MwmSet::RegResult::Success, ());

  MwmSet::MwmHandle mwmHandle = index.GetMwmHandleById(regResult.first);
  TEST(mwmHandle.IsAlive(), ());
  RestrictionLoader const restrictionLoader(*mwmHandle.GetValue<MwmValue>());
  RestrictionCollector const restrictionCollector(restrictionFullPath, mappingFullPath);

  TEST_EQUAL(restrictionLoader.GetRestrictions(), restrictionCollector.GetRestrictions(), ());
}

UNIT_TEST(RestrictionGenerationTest_NoRestriction)
{
  string const restrictionContent = "";
  string const osmIdsToFeatureIdsContent = "";
  TestRestrictionBuilding(restrictionContent, osmIdsToFeatureIdsContent);
}

UNIT_TEST(RestrictionGenerationTest_ZeroId)
{
  string const restrictionContent = R"(Only, 0, 0,)";
  string const osmIdsToFeatureIdsContent = R"(0, 0,)";
  TestRestrictionBuilding(restrictionContent, osmIdsToFeatureIdsContent);
}

UNIT_TEST(RestrictionGenerationTest_OneRestriction)
{
  string const restrictionContent = R"(No, 10, 10,)";
  string const osmIdsToFeatureIdsContent = R"(10, 1,)";
  TestRestrictionBuilding(restrictionContent, osmIdsToFeatureIdsContent);
}

UNIT_TEST(RestrictionGenerationTest_ThreeRestrictions)
{
  string const restrictionContent = R"(No, 10, 10,
                                       Only, 10, 20
                                       Only, 30, 40)";
  string const osmIdsToFeatureIdsContent = R"(10, 1,
                                              20, 2
                                              30, 3,
                                              40, 4)";
  TestRestrictionBuilding(restrictionContent, osmIdsToFeatureIdsContent);
}

UNIT_TEST(RestrictionGenerationTest_SevenRestrictions)
{
  string const restrictionContent = R"(No, 10, 10,
                                       No, 20, 20,
                                       Only, 10, 20,
                                       Only, 20, 30,
                                       No, 30, 30,
                                       No, 40, 40,
                                       Only, 30, 40,)";
  string const osmIdsToFeatureIdsContent = R"(10, 1,
                                              20, 2,
                                              30, 3,
                                              40, 4)";
  TestRestrictionBuilding(restrictionContent, osmIdsToFeatureIdsContent);
}

UNIT_TEST(RestrictionGenerationTest_ThereAndMoreLinkRestrictions)
{
  string const restrictionContent = R"(No, 10, 10,
                                       No, 20, 20,
                                       Only, 10, 20, 30, 40
                                       Only, 20, 30, 40)";
  string const osmIdsToFeatureIdsContent = R"(10, 1,
                                             20, 2,
                                             30, 3,
                                             40, 4)";
  TestRestrictionBuilding(restrictionContent, osmIdsToFeatureIdsContent);
}
}  // namespace
