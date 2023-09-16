
#include <algorithm>
#include <cassert>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <set>
#include <vector>
#include <clang-c/Index.h>


namespace
{
    struct ParseData
    {
        ParseData(const std::string& path): source_path(path) {}

        std::string source_path;
        std::vector<std::string> scope;
        std::set<std::string> ignored_files;

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

        std::vector<Class> classesList;
    };

    std::string generateScopedName(const std::vector<std::string>& scope, const std::string_view& name)
    {
        std::string scopedName;

        for (const auto& n: scope)
        {
            scopedName += n;
            scopedName += "::";
        }

        scopedName += name;

        return scopedName;
    }

    CXChildVisitResult visitor(CXCursor cursor, CXCursor, CXClientData client_data);

    CXChildVisitResult membersVisitor(CXCursor cursor, CXCursor, CXClientData client_data)
    {
        const auto cursorKind = clang_getCursorKind(cursor);

        if (cursorKind == CXCursor_FieldDecl)
        {
            std::vector<ParseData::Member>* members = static_cast<std::vector<ParseData::Member>*>(client_data);

            const CXString cxname = clang_getCursorSpelling(cursor);
            const CXType cxtype = clang_getCursorType(cursor);
            const std::string_view name = clang_getCString(cxname);

            if (name.empty() == false)
            {
                const auto type = clang_getCString(clang_getTypeSpelling(cxtype));

                members->push_back({.name = std::string(name), .type = type});
            }

            clang_disposeString(cxname);
        }

        return CXChildVisit_Continue;
    }

    void processClass(CXCursor cursor, ParseData& data)
    {
        const CXString cxname = clang_getCursorSpelling(cursor);
        const std::string_view name = clang_getCString(cxname);

        if (name.starts_with('_') == false)
        {
            data.classesList.push_back(ParseData::Class(generateScopedName(data.scope, name)));
            data.scope.push_back(std::string(name));

            clang_visitChildren(cursor, membersVisitor, &data.classesList.back().members);
            clang_visitChildren(cursor, visitor, &data);

            data.scope.pop_back();
        }

        clang_disposeString(cxname);
    }

    void processNamespace(CXCursor cursor, ParseData& data)
    {
        const CXString cxname = clang_getCursorSpelling(cursor);
        const std::string_view name = clang_getCString(cxname);

        if (name != "std" && name.starts_with('_') == false)
        {
            if (name.empty() == false)
                data.scope.push_back(std::string(name));

            clang_visitChildren(cursor, visitor, &data);

            if (name.empty() == false)
                data.scope.pop_back();
        }

        clang_disposeString(cxname);
    }

    CXChildVisitResult visitor(CXCursor cursor, CXCursor, CXClientData client_data)
    {
        // process only main file's classes and structs, ignore data from includes
        CXSourceLocation location = clang_getCursorLocation(cursor);

        CXFile file;
        clang_getFileLocation(location, &file, nullptr, nullptr, nullptr);

        // Get the file's name
        CXString fileName = clang_getFileName(file);
        const std::string fileNameStr = clang_getCString(fileName);
        clang_disposeString(fileName);

        ParseData* data = static_cast<ParseData *>(client_data);
        if (fileNameStr == data->source_path)
        {
            const auto cursorKind = clang_getCursorKind(cursor);

            if (cursorKind == CXCursor_ClassDecl || cursorKind == CXCursor_StructDecl)
            {
                CXCursorKind definitionKind = clang_getCursorDefinition(cursor).kind;

                if (definitionKind == CXCursor_ClassDecl || definitionKind == CXCursor_StructDecl)
                    processClass(cursor, *data);
            }
            else if (cursorKind == CXCursor_Namespace)
                processNamespace(cursor, *data);
        }
        else
            data->ignored_files.insert(fileNameStr);

        return CXChildVisit_Continue;
    }

    std::vector<const char *> createClangArgs(char* lookup)
    {
        std::vector<const char *> args;

        long lookup_len = static_cast<long>(std::strlen(lookup));

        for(;;)
        {
            args.push_back(lookup);
            char* pos = std::find(lookup, lookup + lookup_len, ':');

            if (pos < lookup + lookup_len)
            {
                const auto entry_len = pos - lookup + 1;
                *pos = '\0';
                lookup = pos + 1;
                lookup_len -= entry_len;
                assert(lookup_len >= 0);
            }
            else
                break;
        }

        return args;
    }

}


int main(int argc, char* argv[])
{
    if (argc != 2 && argc != 3)
    {
        std::cerr << "Usage: " << argv[0] << " <file to parse> [<clang options to be included, separated with :>]" << std::endl;
        return 1;
    }

    const std::vector<const char *> args = argc == 3? createClangArgs(argv[2]): std::vector<const char *>();
    const char* source_file = argv[1];
    CXIndex index = clang_createIndex(0, 0);
    const auto source_file_absolute_path = std::filesystem::absolute(source_file);

    CXTranslationUnit translationUnit = clang_parseTranslationUnit(
        index, source_file_absolute_path.c_str(), args.data(), static_cast<int>(args.size()), nullptr, 0, CXTranslationUnit_None);

    if (!translationUnit)
    {
        std::cerr << "Failed to parse the translation unit." << std::endl;
        return 1;
    }

    std::cout << "\n";
    std::cout << "#pragma once\n";
    std::cout << "#include " << source_file_absolute_path << "\n\n";

    std::cout << "// Parsing file: " << source_file_absolute_path << "\n\n";

    std::cout << "template<typename T, typename R>"         << "\n";
    std::cout << "void for_each_member_of(const T&, R);"    << "\n\n";

    ParseData data(source_file_absolute_path);

    CXCursor cursor = clang_getTranslationUnitCursor(translationUnit);
    clang_visitChildren(cursor, visitor, &data);

    clang_disposeTranslationUnit(translationUnit);
    clang_disposeIndex(index);

    std::cout << "// Ignored content of files: \n";
    for (const auto& name: data.ignored_files)
        std::cout << "// " << name << "\n";

    std::cout << "\n";

    for (const auto& c: data.classesList)
    {
        const auto& name = c.name;

        std::cout << "template<typename T>"                                     << "\n";
        std::cout << "void for_each_member_of(const " << name << "& obj, T action)"   << "\n";
        std::cout << "{"                                                        << "\n";

        for (const auto& member: c.members)
        {
            const auto& member_name = member.name;
            const auto& member_type = member.type;

            std::cout << "\taction(\"" << member_name << "\", obj." << member_name << ");\t// " << member_type << "\n";
        }

        std::cout << "}"                                                        << "\n\n";
    }

    return 0;
}
