
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>
#include <clang-c/Index.h>


namespace
{
    struct ParseData
    {
        explicit ParseData(const std::string& path): source_path(path) {}

        std::string source_path;
        std::vector<std::string> scope;

        struct Member
        {
            std::string name;
            std::string type;
        };

        struct Class
        {
            explicit Class(const std::string& n): name(n) {}

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

    std::string generateLocation(const CXSourceLocation& location)
    {
        CXFile file;
        unsigned line, column;
        clang_getFileLocation(location, &file, &line, &column, nullptr);

        const char* filePath = clang_getCString(clang_getFileName(file));

        std::string locationMessage = filePath;
        locationMessage += ":" + std::to_string(line) + ":" + std::to_string(column);

        return locationMessage;
    }

    std::string generateMessage(const CXSourceLocation& location, std::string_view message)
    {
        const std::string fullMessage = generateLocation(location) + ": " + std::string(message);

        return fullMessage;
    }

    std::string generateMessage(CXCursor cursor, std::string_view message)
    {
        const CXSourceLocation location = clang_getCursorLocation(cursor);
        return generateMessage(location, message);
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

            const CX_CXXAccessSpecifier accessSpecifier = clang_getCXXAccessSpecifier(cursor);

            if (accessSpecifier != CX_CXXPublic)
                std::cerr << generateMessage(cursor, "warning: " + std::string(name) + " is not a public member. Unable to generate reflexion data for it." + "\n");

            if (name.empty() == false && accessSpecifier == CX_CXXPublic)
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

        data.classesList.push_back(ParseData::Class(generateScopedName(data.scope, name)));
        data.scope.push_back(std::string(name));

        clang_visitChildren(cursor, membersVisitor, &data.classesList.back().members);
        clang_visitChildren(cursor, visitor, &data);

        data.scope.pop_back();

        clang_disposeString(cxname);
    }

    void processNamespace(CXCursor cursor, ParseData& data)
    {
        const CXString cxname = clang_getCursorSpelling(cursor);
        const std::string_view name = clang_getCString(cxname);

        if (name.empty() == false)
            data.scope.push_back(std::string(name));

        clang_visitChildren(cursor, visitor, &data);

        if (name.empty() == false)
            data.scope.pop_back();

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
                const CXCursorKind definitionKind = clang_getCursorDefinition(cursor).kind;
                const CXType cursorType = clang_getCursorType(cursor);
                const int templateArgs = clang_Type_getNumTemplateArguments(cursorType);

                if ( (definitionKind == CXCursor_ClassDecl || definitionKind == CXCursor_StructDecl)
                    && templateArgs == -1)                                                                  // template specializations not supported yet
                {
                    processClass(cursor, *data);
                }
            }
            else if (cursorKind == CXCursor_Namespace)
                processNamespace(cursor, *data);
        }

        return CXChildVisit_Continue;
    }

    bool printProblems(const CXDiagnosticSet& diagnostics)
    {
        const auto count = clang_getNumDiagnosticsInSet(diagnostics);

        bool has_errors = false;
        for(unsigned i = 0; i < count; i++)
        {
            const auto diagnostic = clang_getDiagnosticInSet(diagnostics, i);
            const auto severity = clang_getDiagnosticSeverity(diagnostic);
            const CXString formattedMessage = clang_formatDiagnostic(diagnostic, clang_defaultDiagnosticDisplayOptions());
            const bool error = severity == CXDiagnostic_Error || severity == CXDiagnostic_Fatal;
            const auto diagnosticMessage = clang_getCString(formattedMessage);

            std::cerr << diagnosticMessage << std::endl;

            const auto childrenDiagnostics = clang_getChildDiagnostics(diagnostic);
            has_errors |= printProblems(childrenDiagnostics);

            clang_disposeDiagnostic(diagnostic);

            has_errors |= error;
        }

        return has_errors;
    }

    bool replace_first(std::string& str, const std::string_view& toReplace, const std::string_view& replaceWith)
    {
        const std::size_t pos = str.find(toReplace);
        if (pos == std::string::npos)
            return false;
        else
        {
            str.replace(pos, toReplace.length(), replaceWith);
            return true;
        }
    }

    std::string mangleName(const std::string_view& name)
    {
        std::string retVal(name);
        while(replace_first(retVal, "::", "_"));

        return retVal;
    }

    void generateOutput(const ParseData& data, std::ostream& output)
    {
        output << "namespace reflectpp                                \n";
        output << "{                                                \n\n";
        output << "template<typename T, typename R>\n";
        output << "void get_object_members(const T&, R);            \n\n";

        output << "template<typename T, typename R>                   \n";
        output << "void set_object_members(T&, R);                  \n\n";

        for (const auto& c: data.classesList)
        {
            const auto& name = c.name;
            const auto mangled_name = mangleName(name);

            //
            output << "namespace " << mangled_name << "_Meta                                \n";
            output << "{                                                                  \n\n";

            int i = 0;
            for (const auto& member: c.members)
            {
                const auto& member_name = member.name;
                const auto& member_type = member.type;

                output << "struct Member" << i++ << "_Meta                                      \n";
                output << "{                                                                    \n";
                output << "\tconstexpr static std::string_view name = \"" << member_name << "\";\n";
                output << "\tusing type = " << member_type << ";                                \n";
                output << "};                                                                 \n\n";
            }

            output << "}                                                                  \n\n";

            //

            output << "template<typename T>"                                            << "\n";
            output << "void get_object_members(const " << name << "& obj, T getter)"    << "\n";
            output << "{"                                                               << "\n";

            i = 0;
            for (const auto& member: c.members)
            {
                const auto& member_name = member.name;

                output << "\tgetter(" << mangled_name << "_Meta::Member" << i++ << "_Meta{}, obj." << member_name << ");\n";
            }

            output << "}"                                                               << "\n\n";

            //

            output << "template<typename T>"                                            << "\n";
            output << "void set_object_members(" << name << "& obj, T setter)"          << "\n";
            output << "{"                                                               << "\n";
            i = 0;
            for (const auto& member: c.members)
            {
                const auto& member_name = member.name;

                output << "\tobj." << member_name << " = setter(" << mangled_name << "_Meta::Member" << i++ << "_Meta{});\n";
            }

            output << "}"                                                               << "\n\n";

        }

        output << "}\n";
    }
}


int main(int argc, char* argv[])
{
    if (argc < 3)
    {
        std::cerr << "Usage: " << argv[0] << " <output file> <input file> [<clang options>]" << std::endl;
        return 1;
    }

    const char* source_file = argv[2];
    CXIndex index = clang_createIndex(0, 0);
    const auto source_file_absolute_path = std::filesystem::absolute(source_file);

    CXTranslationUnit translationUnit = clang_parseTranslationUnit(
        index, source_file_absolute_path.string().c_str(), &argv[3], argc - 3, nullptr, 0, CXTranslationUnit_None);

    if (!translationUnit)
    {
        std::cerr << "Failed to parse the translation unit." << std::endl;
        return 1;
    }

    // errors
    const auto diagnostics = clang_getDiagnosticSetFromTU(translationUnit);
    const bool has_errors = printProblems(diagnostics);

    if (has_errors)
    {
        // TODO: are errors fatal?
        // return 1;
    }

    // generation
    const char* output_file = argv[1];
    std::ofstream output(output_file, std::ofstream::out | std::ofstream::trunc);

    output << "\n";
    output << "#pragma once                                   \n";
    output << "#include <string_view>                         \n";

    // do not include source file if it is a c/cpp file
    if (source_file_absolute_path.extension().generic_string().starts_with(".c") == false)
        output << "#include " << source_file_absolute_path  << "\n";

    output << "\n";

    ParseData data(source_file_absolute_path.string());

    CXCursor cursor = clang_getTranslationUnitCursor(translationUnit);
    clang_visitChildren(cursor, visitor, &data);

    clang_disposeTranslationUnit(translationUnit);
    clang_disposeIndex(index);

    generateOutput(data, output);

    return 0;
}
