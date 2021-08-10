#include <gtest/gtest.h>

#include <libtree/deps.hpp>
#include <libtree/ld.hpp>

TEST(example, add)
{    
    ASSERT_TRUE(is_lib({"liblib.so"}));
    ASSERT_TRUE(is_lib({"liblib.so.1"}));
    ASSERT_TRUE(is_lib({"liblib.so.1.2"}));
    ASSERT_TRUE(is_lib({"liblib.so.1.2.3"}));

    ASSERT_TRUE(is_lib({"path/to/libx.so"}));
    ASSERT_TRUE(is_lib({"path/to/libx.so.1"}));
    ASSERT_TRUE(is_lib({"path/to/libx.so.1.2"}));
    ASSERT_TRUE(is_lib({"path/to/libx.so.1.2.3"}));

    ASSERT_FALSE(is_lib({"/some/binary"}));
    ASSERT_FALSE(is_lib({"libtree"}));
    ASSERT_FALSE(is_lib({"doesnotstartwithlib.so"}));
    ASSERT_FALSE(is_lib({"libalmost.1.3.so"}));

    ASSERT_FALSE(is_lib({""}));
}