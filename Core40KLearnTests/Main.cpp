#define BOOST_TEST_MODULE Core 40KLearn Tests


#include <fstream>
#include <iostream>
#include <boost/test/unit_test.hpp>
#include <boost/test/unit_test_monitor.hpp>
#include <boost/test/debug.hpp>


struct FileLoggingConfig
{
	FileLoggingConfig() : test_log("Unit test results.txt")
	{
		boost::unit_test::unit_test_log.set_stream(test_log);
	}
	~FileLoggingConfig()
	{
		boost::unit_test::unit_test_log.set_stream(std::cout);
	}

	std::ofstream test_log;
};


BOOST_GLOBAL_FIXTURE(FileLoggingConfig);