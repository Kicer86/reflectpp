
#include <gmock/gmock.h>

#include "struct_a_r++.hpp"

using testing::ElementsAre;

TEST(ReflectppTest, SetMembers)
{
    Namespace::TestStructA a;

    reflectpp::set_object_members(a, [](auto member)
    {
        using Member = decltype(member);

        if constexpr (std::is_same_v<typename Member::type, std::vector<std::string>>)
            return std::vector<std::string>({"123", "abc"});
        else if constexpr (std::is_same_v<typename Member::type, std::string>)
            return std::string("qwerty");
        else if constexpr (std::is_same_v<typename Member::type, float>)
            return 1.5f;
        else if constexpr (std::is_same_v<typename Member::type, int>)
            return 8;
    });

    EXPECT_THAT(a.vector_of_strings_, ElementsAre("123", "abc"));
    EXPECT_EQ(a.string_, "qwerty");
    EXPECT_EQ(a.float_, 1.5f);
    EXPECT_EQ(a.int_, 8);
}
