/**
 * @file fsinventoryautofile.cpp
 * @brief Regex-pattern based auto-filing of newly received inventory items
 *
 * $LicenseInfo:firstyear=2026&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2026, Firestorm Viewer Project
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * $/LicenseInfo$
 */

#include "llviewerprecompiledheaders.h"

#include "fsinventoryautofile.h"

#include "llinventorymodel.h"
#include "llregex.h"
#include "llsdserialize.h"

#include <boost/regex.hpp>

const char* FSInventoryAutoFile::SETTINGS_FILE_NAME = "fs_inventory_autofile.xml";

// ================================================================
// FSInventoryAutoFileSettings
// ================================================================

namespace
{
    const std::string RULE_PATTERN = "pattern";
    const std::string RULE_FOLDER  = "folder";
}

FSInventoryAutoFileSettings::FSInventoryAutoFileSettings()
{
}

bool FSInventoryAutoFileSettings::setFromLLSD(const LLSD& settings)
{
    mRules.clear();

    if (!settings.isArray())
    {
        return false;
    }

    for (LLSD::array_const_iterator it = settings.beginArray(), end = settings.endArray(); it != end; ++it)
    {
        const LLSD& entry = *it;
        if (entry.isMap() && entry.has(RULE_PATTERN) && entry.has(RULE_FOLDER))
        {
            FSInventoryAutoFileRule rule;
            rule.mPattern = entry[RULE_PATTERN].asString();
            rule.mFolder = entry[RULE_FOLDER].asString();
            mRules.push_back(rule);
        }
    }

    return true;
}

LLSD FSInventoryAutoFileSettings::asLLSD() const
{
    LLSD result = LLSD::emptyArray();
    for (const FSInventoryAutoFileRule& rule : mRules)
    {
        LLSD entry;
        entry[RULE_PATTERN] = rule.mPattern;
        entry[RULE_FOLDER] = rule.mFolder;
        result.append(entry);
    }
    return result;
}

void FSInventoryAutoFileSettings::addRule(const std::string& pattern, const std::string& folder)
{
    FSInventoryAutoFileRule rule;
    rule.mPattern = pattern;
    rule.mFolder = folder;
    mRules.push_back(rule);
}

void FSInventoryAutoFileSettings::removeRule(size_t index)
{
    if (index < mRules.size())
    {
        mRules.erase(mRules.begin() + index);
    }
}

void FSInventoryAutoFileSettings::updateRule(size_t index, const std::string& pattern, const std::string& folder)
{
    if (index < mRules.size())
    {
        mRules[index].mPattern = pattern;
        mRules[index].mFolder = folder;
    }
}

// ================================================================
// FSInventoryAutoFile
// ================================================================

FSInventoryAutoFile::FSInventoryAutoFile()
{
}

void FSInventoryAutoFile::initSingleton()
{
    loadFromSettings();
}

std::string FSInventoryAutoFile::getUserSettingsFileName()
{
    return gDirUtilp->getExpandedFilename(LL_PATH_PER_SL_ACCOUNT, SETTINGS_FILE_NAME);
}

void FSInventoryAutoFile::loadFromSettings()
{
    std::string filename = getUserSettingsFileName();
    if (filename.empty() || !gDirUtilp->fileExists(filename))
    {
        LL_INFOS("InventoryAutoFile") << "no user settings file found; starting with an empty rule set" << LL_ENDL;
        return;
    }

    LLSD userSettings;
    llifstream file;
    file.open(filename.c_str());
    if (file.is_open())
    {
        LLSDSerialize::fromXML(userSettings, file);
        file.close();
    }

    if (mSettings.setFromLLSD(userSettings))
    {
        LL_INFOS("InventoryAutoFile") << "settings loaded from '" << filename << "'" << LL_ENDL;
    }
    else
    {
        LL_WARNS("InventoryAutoFile") << "invalid settings found in '" << filename << "'" << LL_ENDL;
    }
}

void FSInventoryAutoFile::saveToUserSettings()
{
    std::string filename = getUserSettingsFileName();
    if (filename.empty())
    {
        LL_WARNS("InventoryAutoFile") << "no valid per-account settings directory; not saving" << LL_ENDL;
        return;
    }

    llofstream file;
    file.open(filename.c_str());
    LLSDSerialize::toPrettyXML(mSettings.asLLSD(), file);
    file.close();
    LL_INFOS("InventoryAutoFile") << "settings saved to '" << filename << "'" << LL_ENDL;
}

FSInventoryAutoFileSettings FSInventoryAutoFile::getSettings()
{
    return mSettings;
}

void FSInventoryAutoFile::setSettings(const FSInventoryAutoFileSettings& settings)
{
    mSettings = settings;
    saveToUserSettings();
}

// static
std::string FSInventoryAutoFile::extractItemNameFromDesc(const std::string& desc)
{
    // Server description format is: 'ItemName' ( Location )
    std::size_t start = desc.find_first_of('\'');
    std::size_t end = desc.find_last_of('\'');
    if (start != std::string::npos && end != std::string::npos && start != end)
    {
        return desc.substr(start + 1, end - start - 1);
    }
    return desc;
}

bool FSInventoryAutoFile::getDestinationFolderForDesc(const std::string& desc, LLUUID& out_folder_id) const
{
    return getDestinationFolder(extractItemNameFromDesc(desc), out_folder_id);
}

bool FSInventoryAutoFile::getDestinationFolder(const std::string& item_name, LLUUID& out_folder_id) const
{
    for (const FSInventoryAutoFileRule& rule : mSettings.getRules())
    {
        if (rule.mPattern.empty() || rule.mFolder.empty())
        {
            continue;
        }

        bool matched = false;
        try
        {
            boost::regex pattern(rule.mPattern, boost::regex::icase);
            matched = ll_regex_search(item_name, pattern);
        }
        catch (const boost::regex_error& e)
        {
            LL_WARNS("InventoryAutoFile") << "invalid regex pattern '" << rule.mPattern << "': " << e.what() << LL_ENDL;
            continue;
        }

        if (matched)
        {
            LLUUID folder_id = gInventory.findCategoryByName(rule.mFolder);
            if (folder_id.notNull())
            {
                out_folder_id = folder_id;
                return true;
            }
            LL_WARNS("InventoryAutoFile") << "pattern '" << rule.mPattern << "' matched '" << item_name
                << "' but destination folder '" << rule.mFolder << "' does not exist" << LL_ENDL;
            return false;
        }
    }

    return false;
}

void FSInventoryAutoFile::ensureFolderExists(const std::string& folder_name)
{
    if (folder_name.empty())
    {
        return;
    }

    if (gInventory.findCategoryByName(folder_name).isNull())
    {
        gInventory.createNewCategory(gInventory.getRootFolderID(), LLFolderType::FT_NONE, folder_name);
    }
}
