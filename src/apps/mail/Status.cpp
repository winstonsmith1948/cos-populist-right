/*
Open Tracker License

Terms and Conditions

Copyright (c) 1991-2001, Be Incorporated. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice applies to all licensees
and shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF TITLE, MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
BE INCORPORATED BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF, OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of Be Incorporated shall not be
used in advertising or otherwise to promote the sale, use or other dealings in
this Software without prior written authorization from Be Incorporated.

BeMail(TM), Tracker(TM), Be(R), BeOS(R), and BeIA(TM) are trademarks or registered trademarks
of Be Incorporated in the United States and other countries. Other brand product
names are registered trademarks or trademarks of their respective holders.
All rights reserved.
*/
#include "Status.h"

#include <Button.h>
#include <Directory.h>
#include <FindDirectory.h>
#include <fs_index.h>
#include <LayoutBuilder.h>
#include <Node.h>
#include <NodeInfo.h>
#include <Path.h>
#include <Query.h>
#include <TextControl.h>
#include <Volume.h>
#include <VolumeRoster.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "MailApp.h"
#include "MailWindow.h"
#include "Messages.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Mail"


enum status_messages {
	STATUS = 128,
	OK,
	CANCEL
};


TStatusWindow::TStatusWindow(BRect frame, BMessenger target, const char* status)
	: BWindow(BRect(), "", B_MODAL_WINDOW, B_NOT_RESIZABLE | B_AUTO_UPDATE_SIZE_LIMITS),
	fTarget(target)
{
	fStatus = new BTextControl("status",  B_TRANSLATE("Status:"), status,
		new BMessage(STATUS));

	BButton *ok = new BButton("ok", B_TRANSLATE("OK"), new BMessage(OK));
	ok->MakeDefault(true);

	BButton *cancel = new BButton("cancel", B_TRANSLATE("Cancel"), new BMessage(CANCEL));

	BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
		.SetInsets(B_USE_DEFAULT_SPACING)
		.AddGroup(B_HORIZONTAL, 0)
			.Add(fStatus)
		.End()
		.AddStrut(B_USE_SMALL_SPACING)
		.AddGroup(B_HORIZONTAL, B_USE_SMALL_SPACING, 0)
			.AddStrut(B_USE_SMALL_SPACING)
			.Add(cancel)
			.Add(ok);

	fStatus->BTextControl::MakeFocus(true);
	CenterIn(frame);
	Show();
}


TStatusWindow::~TStatusWindow()
{
}


void
TStatusWindow::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case STATUS:
			break;

		case OK:
		{
			if (!_Exists(fStatus->Text())) {
				int32 index = 0;
				uint32 loop;
				status_t result;
				BDirectory dir;
				BEntry entry;
				BFile file;
				BNodeInfo* node;
				BPath path;

				find_directory(B_USER_SETTINGS_DIRECTORY, &path, true);
				dir.SetTo(path.Path());
				if (dir.FindEntry("Mail", &entry) == B_NO_ERROR)
					dir.SetTo(&entry);
				else
					dir.CreateDirectory("Mail", &dir);
				if (dir.InitCheck() != B_NO_ERROR)
					goto err_exit;
				if (dir.FindEntry("status", &entry) == B_NO_ERROR)
					dir.SetTo(&entry);
				else
					dir.CreateDirectory("status", &dir);
				if (dir.InitCheck() == B_NO_ERROR) {
					char name[B_FILE_NAME_LENGTH];
					char newName[B_FILE_NAME_LENGTH];

					sprintf(name, "%s", fStatus->Text());
					if (strlen(name) > B_FILE_NAME_LENGTH - 10)
						name[B_FILE_NAME_LENGTH - 10] = 0;
					for (loop = 0; loop < strlen(name); loop++) {
						if (name[loop] == '/')
							name[loop] = '\\';
					}
					strcpy(newName, name);
					while (1) {
						if ((result = dir.CreateFile(newName, &file, true)) == B_NO_ERROR)
							break;
						if (result != EEXIST)
							goto err_exit;
						snprintf(newName, B_FILE_NAME_LENGTH, "%s_%" B_PRId32,
							name, index++);
					}
					dir.FindEntry(newName, &entry);
					node = new BNodeInfo(&file);
					node->SetType("text/plain");
					delete node;
					file.Write(fStatus->Text(), strlen(fStatus->Text()) + 1);
					file.SetSize(file.Position());
					file.WriteAttr(INDEX_STATUS, B_STRING_TYPE, 0, fStatus->Text(),
						strlen(fStatus->Text()) + 1);
				}
			}
		err_exit:
			{
				BMessage close(M_CLOSE_CUSTOM);
				close.AddString("status", fStatus->Text());
				fTarget.SendMessage(&close);
				// will fall through
			}
		}
		case CANCEL:
			Quit();
			break;
	}
}


bool
TStatusWindow::_Exists(const char* status)
{
	BVolume volume;
	BVolumeRoster().GetBootVolume(&volume);

	BQuery query;
	query.SetVolume(&volume);
	query.PushAttr(INDEX_STATUS);
	query.PushString(status);
	query.PushOp(B_EQ);
	query.Fetch();

	BEntry entry;
	if (query.GetNextEntry(&entry) == B_NO_ERROR)
		return true;

	return false;
}
