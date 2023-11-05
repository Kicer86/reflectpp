
#pragma once

#include <string>

namespace Ext
{
    struct E
    {
        int a;
        float b;
    };
}

namespace Ext2
{
    struct F
    {
        std::string a;
        float b;
    };
}

using namespace Ext;

struct TestStructA
{
    using A = int;
    typedef float B;
    enum class C { a, b };
    struct D { A a; B b; C c; };

    A int_;
    B float_;
    C enum_;
    D struct_;
    E ext_struct_;
    Ext2::F ext_struct2_;
};

