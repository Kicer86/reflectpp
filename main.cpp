
#include <iostream>
#include <clang-c/Index.h>


CXChildVisitResult visitor(CXCursor cursor, CXCursor parent, CXClientData client_data)
{
    const auto cursorKind = clang_getCursorKind(cursor);

    if (cursorKind == CXCursor_FieldDecl)
    {
        const CXString cxname = clang_getCursorSpelling(cursor);
        const CXType cxtype = clang_getCursorType(cursor);
        const auto name = clang_getCString(cxname);
        const auto type = clang_getCString(clang_getTypeSpelling(cxtype));

        std::cout << "\taction(\"" << name << "\", obj." << name << ");\t// " << type << "\n";

        clang_disposeString(cxname);
    }
    else if (cursorKind == CXCursor_ClassDecl || cursorKind == CXCursor_StructDecl)
    {
        const CXString cxname = clang_getCursorSpelling(cursor);
        const auto name = clang_getCString(cxname);

        std::cout << "template<typename T>"                                     << "\n";
        std::cout << "void for_each_member_of(" << name << "& obj, T action)"   << "\n";
        std::cout << "{"                                                        << "\n";

        clang_disposeString(cxname);

        clang_visitChildren(cursor, visitor, nullptr);

        std::cout << "}"                                                        << "\n\n";
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
    CXTranslationUnit translationUnit = clang_parseTranslationUnit(
        index, cppFilePath, nullptr, 0, nullptr, 0, CXTranslationUnit_None);

    if (!translationUnit)
    {
        std::cerr << "Failed to parse the translation unit." << std::endl;
        return 1;
    }

    std::cout << "template<typename T>"         << "\n";
    std::cout << "void for_each_member_of();"   << "\n\n";

    CXCursor cursor = clang_getTranslationUnitCursor(translationUnit);
    clang_visitChildren(cursor, visitor, nullptr);

    clang_disposeTranslationUnit(translationUnit);
    clang_disposeIndex(index);

    return 0;
}
