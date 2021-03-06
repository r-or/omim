project(base_tests)

add_definitions(-DOMIM_UNIT_TEST_DISABLE_PLATFORM_INIT)

set(
  SRC
  assert_test.cpp
  bits_test.cpp
  buffer_vector_test.cpp
  cache_test.cpp
  collection_cast_test.cpp
  condition_test.cpp
  const_helper.cpp
  containers_test.cpp
  levenshtein_dfa_test.cpp
  logging_test.cpp
  math_test.cpp
  matrix_test.cpp
  mem_trie_test.cpp
  newtype_test.cpp
  observer_list_test.cpp
  range_iterator_test.cpp
  ref_counted_tests.cpp
  regexp_test.cpp
  rolling_hash_test.cpp
  scope_guard_test.cpp
  stl_add_test.cpp
  stl_helpers_test.cpp
  string_format_test.cpp
  string_utils_test.cpp
  sunrise_sunset_test.cpp
  thread_pool_tests.cpp
  threaded_list_test.cpp
  threads_test.cpp
  timegm_test.cpp
  timer_test.cpp
  uni_string_dfa_test.cpp
  worker_thread_test.cpp
)

omim_add_test(${PROJECT_NAME} ${SRC})
omim_link_libraries(${PROJECT_NAME} base)
