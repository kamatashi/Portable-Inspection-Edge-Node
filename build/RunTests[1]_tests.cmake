add_test([=[EdgeProcessing.DetectsLineCrack]=]  /home/user/Portable-Inspection-Edge-Node/build/RunTests [==[--gtest_filter=EdgeProcessing.DetectsLineCrack]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[EdgeProcessing.DetectsLineCrack]=]  PROPERTIES WORKING_DIRECTORY /home/user/Portable-Inspection-Edge-Node/build SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==])
set(  RunTests_TESTS EdgeProcessing.DetectsLineCrack)
