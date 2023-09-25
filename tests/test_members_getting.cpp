
#include <gmock/gmock.h>

#include "struct_a_r++.hpp"

using testing::ElementsAre;

TEST(ReflectppTest, GetMembers)
{
    Namespace::TestStructA a;
    a.vector_of_strings_ = {"123", "abc"};
    a.string_ = "qwerty";
    a.float_ = 2.5;
    a.int_ = 78;

    using MemberValues = std::variant<std::vector<std::string>, std::string, float, int>;

    std::vector<std::pair<std::string, MemberValues>> members;

    reflectpp::get_object_members(a, [&members](auto member, const auto& value)
    {
        members.emplace_back(member.name, MemberValues(value));
    });

    EXPECT_EQ(members[0], std::make_pair(std::string("vector_of_strings_"), MemberValues(std::vector<std::string>({"123", "abc"}))));
    EXPECT_EQ(members[1], std::make_pair(std::string("string_"),            MemberValues(std::string("qwerty"))));
    EXPECT_EQ(members[2], std::make_pair(std::string("float_"),             MemberValues(2.5f)));
    EXPECT_EQ(members[3], std::make_pair(std::string("int_"),               MemberValues(78)));
}
