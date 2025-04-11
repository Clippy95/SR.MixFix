#ifndef INIREADER_H
#define INIREADER_H

#define MINI_CASE_SENSITIVE
#include "ini.h"
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <filesystem>

class CIniReader
{
private:
    std::filesystem::path m_szFileName;
    mINI::INIStructure m_ini;

public:
    CIniReader()
    {
        SetIniPath();
    }

    CIniReader(std::filesystem::path szFileName)
    {
        SetIniPath(szFileName);
    }

    bool operator==(CIniReader& ir)
    {
        auto& a = m_ini;
        auto& b = ir.m_ini;
        for (auto const& it : a)
        {
            auto const& section = std::get<0>(it);
            auto const& collection = std::get<1>(it);
            if (collection.size() != b[section].size()) {
                return false;
            }
            for (auto const& it2 : collection)
            {
                auto const& key = std::get<0>(it2);
                auto const& value = std::get<1>(it2);
                if (value != b[section][key]) {
                    return false;
                }
            }
        }
        return a.size() == b.size();
    }

    bool operator!=(CIniReader& ir)
    {
        return !(*this == ir);
    }

    bool CompareBySections(CIniReader& ir)
    {
        std::vector<std::string> sections1;
        std::vector<std::string> sections2;

        for (auto const& it : m_ini)
            sections1.emplace_back(std::get<0>(it));

        for (auto const& it : ir.m_ini)
            sections2.emplace_back(std::get<0>(it));

        return std::equal(sections1.begin(), sections1.end(), sections2.begin(), sections2.end());
    }

    bool CompareByValues(CIniReader& ir)
    {
        return *this == ir;
    }

    const std::filesystem::path& GetIniPath()
    {
        return m_szFileName;
    }

    void SetNewIniPathForSave(std::filesystem::path szFileName)
    {
        m_szFileName = szFileName;
    }

    void SetIniPath()
    {
        SetIniPath("");
    }

    void SetIniPath(std::filesystem::path szFileName)
    {
        static const auto lpModuleName = 1;
        WCHAR buffer[MAX_PATH];
        HMODULE hm = NULL;
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCWSTR)&lpModuleName, &hm);
        GetModuleFileNameW(hm, buffer, ARRAYSIZE(buffer));
        std::filesystem::path modulePath(buffer);

        if (szFileName.is_absolute())
        {
            m_szFileName = szFileName;
        }
        else if (szFileName.empty())
        {
            m_szFileName = modulePath.replace_extension(".ini");
        }
        else
        {
            m_szFileName = modulePath.parent_path() / szFileName;
        }

        mINI::INIFile file(m_szFileName);
        file.read(m_ini);
    }

    int ReadInteger(std::string_view szSection, std::string_view szKey, int iDefaultValue, bool createIfMissing = true)
    {
        try
        {
            // Check if section and key exist
            if (!m_ini.size() || !m_ini.has(szSection.data()) || !m_ini[szSection.data()].has(szKey.data()))
            {
                // Create default value if requested
                if (createIfMissing)
                {
                    m_ini[szSection.data()][szKey.data()] = std::to_string(iDefaultValue);
                    mINI::INIFile file(m_szFileName);
                    file.write(m_ini);
                }
                return iDefaultValue;
            }

            // Key exists, read its value
            auto& value = m_ini[szSection.data()][szKey.data()];
            return std::stoi(value, nullptr, (value.starts_with("0x") || value.starts_with("0X")) ? 16 : 10);
        }
        catch (...)
        {
            // If there's an exception (e.g., conversion error), set the default value if requested
            if (createIfMissing)
            {
                m_ini[szSection.data()][szKey.data()] = std::to_string(iDefaultValue);
                mINI::INIFile file(m_szFileName);
                file.write(m_ini);
            }
            return iDefaultValue;
        }
    }

    float ReadFloat(std::string_view szSection, std::string_view szKey, float fltDefaultValue, bool createIfMissing = true)
    {
        try
        {
            // Check if section and key exist
            if (!m_ini.size() || !m_ini.has(szSection.data()) || !m_ini[szSection.data()].has(szKey.data()))
            {
                // Create default value if requested
                if (createIfMissing)
                {
                    m_ini[szSection.data()][szKey.data()] = std::to_string(fltDefaultValue);
                    mINI::INIFile file(m_szFileName);
                    file.write(m_ini);
                }
                return fltDefaultValue;
            }

            // Key exists, read its value
            auto& value = m_ini[szSection.data()][szKey.data()];
            return static_cast<float>(std::atof(value.data()));
        }
        catch (...)
        {
            // If there's an exception, set the default value if requested
            if (createIfMissing)
            {
                m_ini[szSection.data()][szKey.data()] = std::to_string(fltDefaultValue);
                mINI::INIFile file(m_szFileName);
                file.write(m_ini);
            }
            return fltDefaultValue;
        }
    }
    
    bool ReadBoolean(std::string_view szSection, std::string_view szKey, bool bolDefaultValue, bool createIfMissing = true)
    {
        try
        {
            // Check if section and key exist
            if (!m_ini.size() || !m_ini.has(szSection.data()) || !m_ini[szSection.data()].has(szKey.data()))
            {
                // Create default value if requested
                if (createIfMissing)
                {
                    m_ini[szSection.data()][szKey.data()] = bolDefaultValue ? "true" : "false";
                    mINI::INIFile file(m_szFileName);
                    file.write(m_ini);
                }
                return bolDefaultValue;
            }

            // Key exists, read its value
            auto value = m_ini[szSection.data()][szKey.data()];
            
            // Handle single-character values (0/1)
            if (value.size() == 1)
                return value != "0";
                
            // Handle true/false strings
            std::string valueLower = value;
            std::transform(valueLower.begin(), valueLower.end(), valueLower.begin(), ::tolower);
            
            if (valueLower == "true" || valueLower == "1" || valueLower == "yes" || valueLower == "y")
                return true;
            else if (valueLower == "false" || valueLower == "0" || valueLower == "no" || valueLower == "n")
                return false;
                
            // If value can't be interpreted, reset it to default if requested
            if (createIfMissing)
            {
                m_ini[szSection.data()][szKey.data()] = bolDefaultValue ? "true" : "false";
                mINI::INIFile file(m_szFileName);
                file.write(m_ini);
            }
        }
        catch (...)
        {
            // If there's an exception, set the default value if requested
            if (createIfMissing)
            {
                m_ini[szSection.data()][szKey.data()] = bolDefaultValue ? "true" : "false";
                mINI::INIFile file(m_szFileName);
                file.write(m_ini);
            }
        }
        
        return bolDefaultValue;
    }
    
    std::string ReadString(std::string_view szSection, std::string_view szKey, std::string_view szDefaultValue, bool createIfMissing = true)
    {
        try
        {
            // Check if section and key exist
            if (!m_ini.size() || !m_ini.has(szSection.data()) || !m_ini[szSection.data()].has(szKey.data()))
            {
                // Create default value if requested
                if (createIfMissing)
                {
                    m_ini[szSection.data()][szKey.data()] = szDefaultValue.data();
                    mINI::INIFile file(m_szFileName);
                    file.write(m_ini);
                }
                return std::string(szDefaultValue);
            }

            // Key exists, read its value
            auto value = m_ini[szSection.data()][szKey.data()];
            if (!value.empty())
            {
                if (value.at(0) == '\"' || value.at(0) == '\'')
                    value.erase(0, 1);
                if (value.at(value.size() - 1) == '\"' || value.at(value.size() - 1) == '\'')
                    value.erase(value.size() - 1);
            }
            return value;
        }
        catch (...)
        {
            // If there's an exception, set the default value if requested
            if (createIfMissing)
            {
                m_ini[szSection.data()][szKey.data()] = szDefaultValue.data();
                mINI::INIFile file(m_szFileName);
                file.write(m_ini);
            }
            return std::string(szDefaultValue);
        }
    }
    
    void WriteInteger(std::string_view szSection, std::string_view szKey, int iValue, bool pretty = false)
    {
        try
        {
            mINI::INIFile file(m_szFileName);
            m_ini[szSection.data()][szKey.data()] = std::to_string(iValue);
            file.write(m_ini, pretty);
        }
        catch (...) {}
    }
    
    void WriteFloat(std::string_view szSection, std::string_view szKey, float fltValue, bool pretty = false)
    {
        try
        {
            mINI::INIFile file(m_szFileName);
            m_ini[szSection.data()][szKey.data()] = std::to_string(fltValue);
            file.write(m_ini, pretty);
        }
        catch (...) {}
    }
    
    void WriteBoolean(std::string_view szSection, std::string_view szKey, bool bolValue, bool pretty = false)
    {
        try
        {
            mINI::INIFile file(m_szFileName);
            m_ini[szSection.data()][szKey.data()] = bolValue ? "True" : "False";
            file.write(m_ini, pretty);
        }
        catch (...) {}
    }
    
    void WriteString(std::string_view szSection, std::string_view szKey, std::string_view szValue, bool pretty = false)
    {
        try
        {
            mINI::INIFile file(m_szFileName);
            m_ini[szSection.data()][szKey.data()] = szValue.data();
            file.write(m_ini, pretty);
        }
        catch (...) {}
    }
};

#endif //INIREADER_H