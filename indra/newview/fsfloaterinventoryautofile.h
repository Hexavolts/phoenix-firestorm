/**
 * @file fsfloaterinventoryautofile.h
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

#ifndef FS_FLOATERINVENTORYAUTOFILE_H
#define FS_FLOATERINVENTORYAUTOFILE_H

#include "llfloater.h"
#include "lllineeditor.h"
#include "llscrolllistctrl.h"

#include "fsinventoryautofile.h"

class FSFloaterInventoryAutoFile : public LLFloater
{
public:
    FSFloaterInventoryAutoFile(const LLSD& key);

    bool postBuild() override;
    void onClose(bool app_quitting) override;

private:
    FSInventoryAutoFileSettings mSettings; ///< working copy, committed on Save Changes
    S32 mSelectedIndex; ///< index into mSettings' rule list, or -1 if none/adding new

    LLScrollListCtrl* mRulesList;
    LLLineEditor* mPattern;
    LLLineEditor* mFolder;

    void updateRulesList();
    void enableEntryEditor();
    void disableEntryEditor();

    void onSelectEntry();
    void onAddEntry();
    void onDeleteEntry();
    void onSaveEntry();

    void onSaveChanges();
    void onCancel();
};

#endif // FS_FLOATERINVENTORYAUTOFILE_H
