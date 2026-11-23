#include <gtest/gtest.h>
#include "support/leak_check.hpp"

int main(int argument_count, char** arguments)
{
	testing::InitGoogleTest(&argument_count, arguments);
	testing::UnitTest::GetInstance()->listeners().Append(new ds::test::leak_check);
	return RUN_ALL_TESTS();
}
