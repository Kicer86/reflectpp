
#include <deque>
#include <filesystem>
#include <iostream>
#include <vector>
#include <clang-c/Index.h>


namespace
{
    struct ParseData
    {
        std::deque<std::string> scope;

        struct Member
        {
            std::string name;
            std::string type;
        };

        struct Class
        {
            Class(const std::string& n): name(n) {}

            std::string name;
            std::vector<Member> members;
        };

        std::deque<Class> classesList;
        Class* currentClass;
    };

    std::string generateScopedName(const std::deque<std::string>& scope, const std::string& name)
    {
        std::string scopedName;

        for (const auto& name: scope)
        {
            scopedName += name;

            scopedName += "::";
        }

        scopedName += name;

        return scopedName;
    }
}


CXChildVisitResult visitor(CXCursor cursor, CXCursor, CXClientData client_data)
{
    ParseData* data = static_cast<ParseData *>(client_data);

    const auto cursorKind = clang_getCursorKind(cursor);

    if (cursorKind == CXCursor_FieldDecl)
    {
        const CXString cxname = clang_getCursorSpelling(cursor);
        const CXType cxtype = clang_getCursorType(cursor);
        const auto name = clang_getCString(cxname);
        const auto type = clang_getCString(clang_getTypeSpelling(cxtype));

        data->currentClass->members.push_back({.name = name, .type = type});

        clang_disposeString(cxname);
    }
    else if (cursorKind == CXCursor_ClassDecl || cursorKind == CXCursor_StructDecl)
    {
        const CXString cxname = clang_getCursorSpelling(cursor);
        const auto name = clang_getCString(cxname);

        data->classesList.push_back(ParseData::Class(generateScopedName(data->scope, name)));

        ParseData::Class* currentClass = data->currentClass;
        data->currentClass = &data->classesList.back();             // define where CXCursor_FieldDecl should add themselves
        data->scope.push_back(name);

        clang_disposeString(cxname);
        clang_visitChildren(cursor, visitor, data);

        // restore previous pointer to current class so CXCursor_FieldDecl from this level of recursion will write to theirs owner
        data->scope.pop_back();
        data->currentClass = currentClass;
    }
    else if (cursorKind == CXCursor_Namespace)
    {
        const CXString cxname = clang_getCursorSpelling(cursor);
        const std::string name = clang_getCString(cxname);

        if (name.empty() == false)
            data->scope.push_back(name);

        clang_disposeString(cxname);

        clang_visitChildren(cursor, visitor, data);

        if (name.empty() == false)
            data->scope.pop_back();
    }

    return CXChildVisit_Continue;
}


int main(int argc, char* argv[])
{
    if (argc != 2)
    {
        std::cerr << "Usage: " << argv[0] << " <cpp_file.cpp>" << std::endl;
        return 1;
    }

    CXIndex index = clang_createIndex(0, 0);
    const char* cppFilePath = argv[1];

    const auto cppAbsoluteFilePath = std::filesystem::absolute(cppFilePath);

    CXTranslationUnit translationUnit = clang_parseTranslationUnit(
        index, cppFilePath, nullptr, 0, nullptr, 0, CXTranslationUnit_None);

    if (!translationUnit)
    {
        std::cerr << "Failed to parse the translation unit." << std::endl;
        return 1;
    }

    std::cout << "\n";
    std::cout << "#include " << cppAbsoluteFilePath << "\n\n";

    std::cout << "template<typename T>"         << "\n";
    std::cout << "void for_each_member_of();"   << "\n\n";

    ParseData data;

    CXCursor cursor = clang_getTranslationUnitCursor(translationUnit);
    clang_visitChildren(cursor, visitor, &data);

    clang_disposeTranslationUnit(translationUnit);
    clang_disposeIndex(index);

    for (const auto& c: data.classesList)
    {
        const auto& name = c.name;

        std::cout << "template<typename T>"                                     << "\n";
        std::cout << "void for_each_member_of(" << name << "& obj, T action)"   << "\n";
        std::cout << "{"                                                        << "\n";

        for (const auto& member: c.members)
        {
            const auto& member_name = member.name;
            const auto& member_type = member.type;

            std::cout << "\taction(\"" << name << "\", obj." << member_name << ");\t// " << member_type << "\n";
        }

        std::cout << "}"                                                        << "\n\n";
    }

    return 0;
}
