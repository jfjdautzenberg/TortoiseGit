// TortoiseGitMerge - a Diff/Patch program

// Copyright (C) 2010-2011 - TortoiseGit
// Copyright (C) 2006-2010, 2013 - TortoiseSVN

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
#include "stdafx.h"
#include "registry.h"
#include "AppUtils.h"
#include "PathUtils.h"
#include "UnicodeUtils.h"
#include "SysProgressDlg.h"

#pragma warning(push)
#include "svn_pools.h"
#include "svn_io.h"
#include "svn_path.h"
#include "svn_diff.h"
#include "svn_string.h"
#include "svn_utf.h"
#pragma warning(pop)
#include "Git.h"
#include "CreateProcessHelper.h"
#include "FormatMessageWrapper.h"

CAppUtils::CAppUtils(void)
{
}

CAppUtils::~CAppUtils(void)
{
}

BOOL CAppUtils::GetVersionedFile(CString sPath, CString sVersion, CString sSavePath, CSysProgressDlg * progDlg, HWND hWnd /*=NULL*/)
{
	CString sSCMPath = CRegString(_T("Software\\TortoiseGitMerge\\SCMPath"), _T(""));
	if (sSCMPath.IsEmpty())
	{
		// no path set, so use TortoiseGit as default
		sSCMPath = CPathUtils::GetAppDirectory() + _T("TortoiseGitProc.exe");
		sSCMPath += _T(" /command:cat /path:\"%1\" /revision:%2 /savepath:\"%3\" /hwnd:%4");
	}
	CString sTemp;
	sTemp.Format(_T("%ld"), (INT_PTR)hWnd);
	sSCMPath.Replace(_T("%1"), sPath);
	sSCMPath.Replace(_T("%2"), sVersion);
	sSCMPath.Replace(_T("%3"), sSavePath);
	sSCMPath.Replace(_T("%4"), sTemp);
	// start the external SCM program to fetch the specific version of the file
	PROCESS_INFORMATION process;
	if (!CCreateProcessHelper::CreateProcess(NULL, (LPTSTR)(LPCTSTR)sSCMPath, &process))
	{
		CFormatMessageWrapper errorDetails;
		MessageBox(NULL, errorDetails, _T("TortoiseGitMerge"), MB_OK | MB_ICONERROR);
		return FALSE;
	}
	DWORD ret = 0;
	do
	{
		ret = WaitForSingleObject(process.hProcess, 100);
	} while ((ret == WAIT_TIMEOUT) && (!progDlg || !progDlg->HasUserCancelled()));
	CloseHandle(process.hThread);
	CloseHandle(process.hProcess);

	if (progDlg && progDlg->HasUserCancelled())
	{
		return FALSE;
	}
	if (!PathFileExists(sSavePath))
		return FALSE;
	return TRUE;
}

bool CAppUtils::CreateUnifiedDiff(const CString& orig, const CString& modified, const CString& output, bool bShowError)
{
	CString cmd;
	cmd.Format(_T("git.exe diff --no-index -- \"%s\" \"%s\""),orig, modified);

	if(g_Git.RunLogFile(cmd,(CString&)output) && bShowError)
	{
		MessageBox(NULL, _T("Fail Create Patch"), _T("TortoiseGit"), MB_OK | MB_ICONERROR);
		return false;
	}
	return true;
}

bool CAppUtils::HasClipboardFormat(UINT format)
{
	if (OpenClipboard(NULL))
	{
		UINT enumFormat = 0;
		do
		{
			if (enumFormat == format)
			{
				CloseClipboard();
				return true;
			}
		} while((enumFormat = EnumClipboardFormats(enumFormat))!=0);
		CloseClipboard();
	}
	return false;
}

COLORREF CAppUtils::IntenseColor(long scale, COLORREF col)
{
	// if the color is already dark (gray scale below 127),
	// then lighten the color by 'scale', otherwise darken it
	int Gray  = (((int)GetRValue(col)) + GetGValue(col) + GetBValue(col))/3;
	if (Gray > 127)
	{
		long red   = MulDiv(GetRValue(col),(255-scale),255);
		long green = MulDiv(GetGValue(col),(255-scale),255);
		long blue  = MulDiv(GetBValue(col),(255-scale),255);

		return RGB(red, green, blue);
	}
	long R = MulDiv(255-GetRValue(col),scale,255)+GetRValue(col);
	long G = MulDiv(255-GetGValue(col),scale,255)+GetGValue(col);
	long B = MulDiv(255-GetBValue(col),scale,255)+GetBValue(col);

	return RGB(R, G, B);
}
