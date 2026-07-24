/**
 * @file fsinventoryautofile.h
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

#ifndef FS_INVENTORYAUTOFILE_H
#define FS_INVENTORYAUTOFILE_H

#include "llsd.h"
#include "llsingleton.h"
#include "lluuid.h"

#include <string>
#include <vector>

/// One pattern -> destination folder rule
struct FSInventoryAutoFileRule
{
    std::string mPattern;
    std::string mFolder;
};

/// Holds the editable rule set. Kept as a separate value type so the settings
/// floater can operate on a working copy and discard it on Cancel, the same
/// way LLAutoReplaceSettings is used by LLFloaterAutoReplaceSettings.
class FSInventoryAutoFileSettings
{
public:
    FSInventoryAutoFileSettings();

    bool setFromLLSD(const LLSD& settings);
    LLSD asLLSD() const;

    const std::vector<FSInventoryAutoFileRule>& getRules() const { return mRules; }

    void addRule(const std::string& pattern, const std::string& folder);
    void removeRule(size_t index);
    void updateRule(size_t index, const std::string& pattern, const std::string& folder);

private:
    std::vector<FSInventoryAutoFileRule> mRules;
};

/// Manages the persisted rule set and performs the pattern -> folder lookup
/// used when filing newly received inventory items.
class FSInventoryAutoFile : public LLSingleton<FSInventoryAutoFile>
{
    LLSINGLETON(FSInventoryAutoFile);

public:
    /// Get a copy of the current settings, for the floater to edit
    FSInventoryAutoFileSettings getSettings();

    /// Commit new settings and persist them
    void setSettings(const FSInventoryAutoFileSettings& settings);

    /// Looks up the item name embedded in an inventory offer description
    /// (server format: "'ItemName' ( Location )") against the configured
    /// rules, in order. On the first pattern match whose destination folder
    /// currently exists, returns true and fills in out_folder_id.
    /// Never throws: a malformed regex in a rule is logged and skipped.
    bool getDestinationFolderForDesc(const std::string& desc, LLUUID& out_folder_id) const;

    /// Same lookup, but takes an already-extracted plain item name.
    bool getDestinationFolder(const std::string& item_name, LLUUID& out_folder_id) const;

    /// Creates the named top-level inventory folder if it doesn't already
    /// exist. Called eagerly when a rule is saved so that the folder is
    /// guaranteed to exist by the time a matching item is offered.
    void ensureFolderExists(const std::string& folder_name);

    /// Extracts the item name from an offer description formatted as
    /// "'ItemName' ( Location )", mirroring LLOfferInfo::getSanitizedDescription.
    static std::string extractItemNameFromDesc(const std::string& desc);

private:
    /*virtual*/ void initSingleton() override;

    FSInventoryAutoFileSettings mSettings;

    void loadFromSettings();
    void saveToUserSettings();
    std::string getUserSettingsFileName();

    static const char* SETTINGS_FILE_NAME;
};

#endif // FS_INVENTORYAUTOFILE_H
