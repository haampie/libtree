#include <gtest/gtest.h>
#include <libtree/ld.hpp>
#include <algorithm>

std::string path;

TEST(example, add)
{    
    auto paths = parse_ld_conf(path);
    std::sort(paths.begin(), paths.end());

    ASSERT_EQ(paths.size(), 5);
    ASSERT_EQ(paths[0], "/and/another/path");
    ASSERT_EQ(paths[1], "/path/to/dir/a");
    ASSERT_EQ(paths[2], "/path/to/dir/b");
    ASSERT_EQ(paths[3], "/path/to/dir/c");
    ASSERT_EQ(paths[4], "/some/path");
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    // first arg == path to conf file.
    assert( argc >= 2 );
    path = argv[1];
    return RUN_ALL_TESTS();
}