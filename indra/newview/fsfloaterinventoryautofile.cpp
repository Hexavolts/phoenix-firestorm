/**
 * @file fsfloaterinventoryautofile.cpp
 * @brief Settings floater for regex-based inventory auto-filing rules
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

#include "fsfloaterinventoryautofile.h"

#include "llbutton.h"

FSFloaterInventoryAutoFile::FSFloaterInventoryAutoFile(const LLSD& key)
 : LLFloater(key)
 , mSelectedIndex(-1)
 , mRulesList(NULL)
 , mPattern(NULL)
 , mFolder(NULL)
{
}

bool FSFloaterInventoryAutoFile::postBuild()
{
    mSettings = FSInventoryAutoFile::instance().getSettings();

    mRulesList = getChild<LLScrollListCtrl>("autofile_rules_list");
    mRulesList->setCommitCallback(boost::bind(&FSFloaterInventoryAutoFile::onSelectEntry, this));
    mRulesList->setCommitOnSelectionChange(true);

    mPattern = getChild<LLLineEditor>("autofile_pattern");
    mFolder = getChild<LLLineEditor>("autofile_folder");

    getChild<LLUICtrl>("autofile_add_entry")->setCommitCallback(boost::bind(&FSFloaterInventoryAutoFile::onAddEntry, this));
    getChild<LLUICtrl>("autofile_delete_entry")->setCommitCallback(boost::bind(&FSFloaterInventoryAutoFile::onDeleteEntry, this));
    getChild<LLUICtrl>("autofile_save_entry")->setCommitCallback(boost::bind(&FSFloaterInventoryAutoFile::onSaveEntry, this));
    getChild<LLUICtrl>("autofile_save_changes")->setCommitCallback(boost::bind(&FSFloaterInventoryAutoFile::onSaveChanges, this));
    getChild<LLUICtrl>("autofile_cancel")->setCommitCallback(boost::bind(&FSFloaterInventoryAutoFile::onCancel, this));

    center();

    updateRulesList();
    disableEntryEditor();

    return true;
}

void FSFloaterInventoryAutoFile::onClose(bool app_quitting)
{
}

void FSFloaterInventoryAutoFile::updateRulesList()
{
    mRulesList->deleteAllItems();

    const std::vector<FSInventoryAutoFileRule>& rules = mSettings.getRules();
    for (size_t i = 0; i < rules.size(); ++i)
    {
        LLSD row;
        row["value"] = (S32)i;
        row["columns"][0]["column"] = "pattern";
        row["columns"][0]["value"] = rules[i].mPattern;
        row["columns"][1]["column"] = "folder";
        row["columns"][1]["value"] = rules[i].mFolder;

        mRulesList->addElement(row, ADD_BOTTOM);
    }
}

void FSFloaterInventoryAutoFile::enableEntryEditor()
{
    mPattern->setEnabled(true);
    mFolder->setEnabled(true);
    getChild<LLButton>("autofile_save_entry")->setEnabled(true);
    getChild<LLButton>("autofile_delete_entry")->setEnabled(mSelectedIndex != -1);
}

void FSFloaterInventoryAutoFile::disableEntryEditor()
{
    mSelectedIndex = -1;
    mPattern->clear();
    mPattern->setEnabled(false);
    mFolder->clear();
    mFolder->setEnabled(false);
    getChild<LLButton>("autofile_save_entry")->setEnabled(false);
    getChild<LLButton>("autofile_delete_entry")->setEnabled(false);
}

void FSFloaterInventoryAutoFile::onSelectEntry()
{
    LLSD selected = mRulesList->getSelectedValue();
    if (selected.isDefined())
    {
        mSelectedIndex = selected.asInteger();
        const std::vector<FSInventoryAutoFileRule>& rules = mSettings.getRules();
        if (mSelectedIndex >= 0 && (size_t)mSelectedIndex < rules.size())
        {
            mPattern->setValue(rules[mSelectedIndex].mPattern);
            mFolder->setValue(rules[mSelectedIndex].mFolder);
            enableEntryEditor();
        }
    }
    else
    {
        disableEntryEditor();
    }
}

void FSFloaterInventoryAutoFile::onAddEntry()
{
    mRulesList->deselectAllItems(false /* don't call commit */);
    mSelectedIndex = -1;
    mPattern->clear();
    mFolder->clear();
    enableEntryEditor();
    mPattern->setFocus(true);
}

void FSFloaterInventoryAutoFile::onDeleteEntry()
{
    if (mSelectedIndex != -1)
    {
        mSettings.removeRule(mSelectedIndex);
        updateRulesList();
        disableEntryEditor();
    }
}

void FSFloaterInventoryAutoFile::onSaveEntry()
{
    std::string pattern = mPattern->getValue().asString();
    std::string folder = mFolder->getValue().asString();

    if (pattern.empty() || folder.empty())
    {
        return;
    }

    if (mSelectedIndex == -1)
    {
        mSettings.addRule(pattern, folder);
    }
    else
    {
        mSettings.updateRule(mSelectedIndex, pattern, folder);
    }

    // Create the destination folder now so it's guaranteed to exist by the
    // time any matching item is offered.
    FSInventoryAutoFile::instance().ensureFolderExists(folder);

    updateRulesList();
    disableEntryEditor();
}

void FSFloaterInventoryAutoFile::onSaveChanges()
{
    FSInventoryAutoFile::instance().setSettings(mSettings);
    closeFloater(false /* not quitting */);
}

void FSFloaterInventoryAutoFile::onCancel()
{
    closeFloater(false /* not quitting */);
}
