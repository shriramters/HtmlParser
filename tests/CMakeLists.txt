add_executable(RunTests ParserTest.cpp DOMTest.cpp DOMStrictTest.cpp QueryTest.cpp DOMToHtmlTest.cpp QueryAdvancedTest.cpp WhitespaceTest.cpp HeadElementsTest.cpp ParserStateTest.cpp)
target_link_libraries(RunTests HtmlParser gtest gtest_main)

add_test(NAME ParserTest COMMAND RunTests)
