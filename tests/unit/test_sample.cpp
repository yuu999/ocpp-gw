
#include <gtest/gtest.h>

// Sample test to verify Google Test setup
TEST(SampleTest, BasicAssertions) {
    // Expect equality
    EXPECT_EQ(2 + 2, 4);
    
    // Expect inequality
    EXPECT_NE(2 + 2, 5);
    
    // Expect true
    EXPECT_TRUE(1 == 1);
    
    // Expect false
    EXPECT_FALSE(1 == 2);
}
