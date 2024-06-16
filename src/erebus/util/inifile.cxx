#include <erebus/util/inifile.hxx>
#include <erebus/util/stringutil.hxx>

#include <boost/algorithm/string.hpp>

#include <vector>

namespace Er
{

namespace Util
{

namespace IniFile
{

namespace
{

std::string_view unquoteValue(std::string_view value)
{
    if (value.length() < 2)
        return value;

    // assume trimmed value here
    if ((value[0] == '\"') && (value[value.length() - 1] == '\"'))
        return value.substr(1, value.length() - 2);

    if ((value[0] == '\'') && (value[value.length() - 1] == '\''))
        return value.substr(1, value.length() - 2);

    return value;
}

std::string_view uncommentValue(std::string_view value)
{
    std::string_view::size_type pos = 0;
    std::optional<char> quote;
    while (pos < value.length())
    {
        if ((value[pos] == '\"') || (value[pos] == '\''))
        {
            if (quote && (*quote == value[pos]))
            {
                // close quotes
                quote = std::nullopt;
            }
            else if (!quote)
            {
                // open quotes
                quote = value[pos];
            }

            // this is a quoted quote
        }
        else if (value[pos] == '#')
        {
            if (!quote)
                return value.substr(0, pos);
                
            // this is quoted #
        }
        else if ((value[pos] == ';') && (pos < value.length() - 1) && (value[pos + 1] == ' '))
        {
            if (!quote)
                return value.substr(0, pos);

            // not a comment
        }

        ++pos;
    }

    return value;
}

std::pair<std::string_view, std::string_view> splitKeyAndValue(std::string_view line)
{
    auto eq = line.find('=');
    if (eq == line.npos)
        return std::make_pair(std::string_view(), std::string_view());

    auto key = line.substr(0, eq);
    key = Er::Util::trim(key);
    if (key.empty())
        return std::make_pair(std::string_view(), std::string_view());

    std::string_view value;
    if (eq + 1 < line.length())
    {
        value = line.substr(eq + 1);
        value = Er::Util::trim(value);
        value = uncommentValue(value);
        value = Er::Util::trim(value);
        value = unquoteValue(value);
        value = Er::Util::trim(value);
    }

    return std::make_pair(key, value);
}

std::string_view detectSection(std::string_view line)
{
    // assume trimmed line
    if (line[0] == '[')
    {
        std::string_view::size_type pos = 1;
        long brackets = 1;
        while (pos < line.length())
        {
            if (line[pos] == '[')
            {
               ++brackets;
            }
            else if (line[pos] == ']')
            {
                if (--brackets == 0)
                {
                    return line.substr(1, pos - 1);
                }
            }

            ++pos;
        }
    }

    return std::string_view();
}

} // namespace {}


EREBUS_EXPORT Sections parse(std::string_view raw)
{
    Sections sections;
    Section currentSection;
    std::string_view currentSectionName;

    Er::Util::split(
        raw, 
        std::string_view("\n\r"), 
        Er::Util::SplitSkipEmptyParts,
        [&sections, &currentSection, &currentSectionName](std::string_view line)
        {
            auto trimmed = Er::Util::trim(line);

            if (trimmed.empty())
                return;

            auto sectionName = detectSection(line);
            if (!sectionName.empty()) // section start
            {
                // complete previous section
                if (!currentSectionName.empty())
                {
                    sections.insert({ currentSectionName, currentSection });
                    currentSection.clear();
                }

                currentSectionName = sectionName;
                return;
            }
            
            auto kv = splitKeyAndValue(line);
            if (!kv.first.empty())
                currentSection.insert({ kv.first, kv.second });
        }
    );

    // complete the last section
    if (!currentSectionName.empty())
    {
        sections.insert({ currentSectionName, currentSection });
    }

    return sections;
}

EREBUS_EXPORT std::optional<std::string_view> lookup(const Sections& ini, std::string_view section, std::string_view key)
{
    auto secIt = ini.find(section);
    if (secIt == ini.end())
        return std::nullopt;

    auto valIt = secIt->second.find(key);
    if (valIt == secIt->second.end())
        return std::nullopt;

    return valIt->second;
}


} // namespace IniFile {}

} // namespace Util {}

} // namespace Er {]