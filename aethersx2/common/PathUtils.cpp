/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2010  PCSX2 Dev Team
 *
 *  PCSX2 is free software: you can redistribute it and/or modify it under the terms
 *  of the GNU Lesser General Public License as published by the Free Software Found-
 *  ation, either version 3 of the License, or (at your option) any later version.
 *
 *  PCSX2 is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with PCSX2.
 *  If not, see <http://www.gnu.org/licenses/>.
 */

#include "common/Path.h"

#include <wx/file.h>
#include <wx/utils.h>

// ---------------------------------------------------------------------------------
//  wxDirName (implementations)
// ---------------------------------------------------------------------------------

wxFileName wxDirName::Combine(const wxFileName& right) const
{
	pxAssertMsg(IsDir(), L"Warning: Malformed directory name detected during wxDirName concatenation.");
	if (right.IsAbsolute())
		return right;

	// Append any directory parts from right, and then set the filename.
	// Except we can't do that because our m_members are private (argh!) and there is no API
	// for getting each component of the path.  So instead let's use Normalize:

	wxFileName result(right);
	result.Normalize(wxPATH_NORM_ENV_VARS | wxPATH_NORM_DOTS | wxPATH_NORM_ABSOLUTE, GetPath());
	return result;
}

wxDirName wxDirName::Combine(const wxDirName& right) const
{
	pxAssertMsg(IsDir() && right.IsDir(), L"Warning: Malformed directory name detected during wDirName concatenation.");

	wxDirName result(right);
	result.Normalize(wxPATH_NORM_ENV_VARS | wxPATH_NORM_DOTS | wxPATH_NORM_ABSOLUTE, GetPath());
	return result;
}

wxDirName& wxDirName::Normalize(int flags, const wxString& cwd)
{
	pxAssertMsg(IsDir(), L"Warning: Malformed directory name detected during wDirName normalization.");
	if (!wxFileName::Normalize(flags, cwd))
		throw Exception::ParseError().SetDiagMsg(L"wxDirName::Normalize operation failed.");
	return *this;
}

wxDirName& wxDirName::MakeRelativeTo(const wxString& pathBase)
{
	pxAssertMsg(IsDir(), L"Warning: Malformed directory name detected during wDirName normalization.");
	if (!wxFileName::MakeRelativeTo(pathBase))
		throw Exception::ParseError().SetDiagMsg(L"wxDirName::MakeRelativeTo operation failed.");
	return *this;
}

wxDirName& wxDirName::MakeAbsolute(const wxString& cwd)
{
	pxAssertMsg(IsDir(), L"Warning: Malformed directory name detected during wDirName normalization.");
	if (!wxFileName::MakeAbsolute(cwd))
		throw Exception::ParseError().SetDiagMsg(L"wxDirName::MakeAbsolute operation failed.");
	return *this;
}

void wxDirName::Rmdir() const
{
	if (!Exists())
		return;
	wxFileName::Rmdir();
	// TODO : Throw exception if operation failed?  Do we care?
}

bool wxDirName::Mkdir() const
{
// wxWidgets recurses directory creation for us.

// only exist in wx2.9 and above
#ifndef wxS_DIR_DEFAULT
#define wxS_DIR_DEFAULT 0777
#endif

	if (Exists())
		return true;
	return wxFileName::Mkdir(wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);
}


// ---------------------------------------------------------------------------------
//  Path namespace (wxFileName helpers)
// ---------------------------------------------------------------------------------


bool Path::IsRelative(const wxString& path)
{
	return wxDirName(path).IsRelative();
}

// Returns -1 if the file does not exist.
s64 Path::GetFileSize(const wxString& path)
{
	if (!wxFile::Exists(path.c_str()))
		return -1;
	return (s64)wxFileName::GetSize(path).GetValue();
}


wxString Path::Normalize(const wxString& src)
{
	wxFileName normalize(src);
	normalize.Normalize();
	return normalize.GetFullPath();
}

wxString Path::Normalize(const wxDirName& src)
{
	return wxDirName(src).Normalize().ToString();
}

wxString Path::MakeAbsolute(const wxString& src)
{
	wxFileName absolute(src);
	absolute.MakeAbsolute();
	return absolute.GetFullPath();
}

// Concatenates two pathnames together, inserting delimiters (backslash on win32)
// as needed! Assumes the 'dest' is allocated to at least g_MaxPath length.
//
wxString Path::Combine(const wxString& srcPath, const wxString& srcFile)
{
	return (wxDirName(srcPath) + srcFile).GetFullPath();
}

wxString Path::Combine(const wxDirName& srcPath, const wxFileName& srcFile)
{
	return (srcPath + srcFile).GetFullPath();
}

wxString Path::Combine(const wxString& srcPath, const wxDirName& srcFile)
{
	return (wxDirName(srcPath) + srcFile).ToString();
}

std::string Path::CombineStdString(const wxDirName& srcPath, const std::string_view& srcFile)
{
	const wxString wxResult((srcPath + wxString::FromUTF8(srcFile.data(), srcFile.length())).GetFullPath());
	const wxCharBuffer wxBuf(wxResult.ToUTF8());
	return std::string(wxBuf.data(), wxBuf.length());
}

std::string Path::CombineStdString(const std::string_view& srcPath, const std::string_view& srcFile)
{
	const wxDirName srcPathDir(wxString::FromUTF8(srcPath.data(), srcPath.length()));
	const wxString wxResult((srcPathDir + wxString::FromUTF8(srcFile.data(), srcFile.length())).GetFullPath());
	const wxCharBuffer wxBuf(wxResult.ToUTF8());
	return std::string(wxBuf.data(), wxBuf.length());
}

// Replaces the extension of the file with the one given.
// This function works for path names as well as file names.
wxString Path::ReplaceExtension(const wxString& src, const wxString& ext)
{
	wxFileName jojo(src);
	jojo.SetExt(ext);
	return jojo.GetFullPath();
}

wxString Path::ReplaceFilename(const wxString& src, const wxString& newfilename)
{
	wxFileName jojo(src);
	jojo.SetFullName(newfilename);
	return jojo.GetFullPath();
}

wxString Path::GetFilename(const wxString& src)
{
	return wxFileName(src).GetFullName();
}

wxString Path::GetFilenameWithoutExt(const wxString& src)
{
	return wxFileName(src).GetName();
}

wxString Path::GetDirectory(const wxString& src)
{
	return wxFileName(src).GetPath();
}


// returns the base/root directory of the given path.
// Example /this/that/something.txt -> dest == "/"
wxString Path::GetRootDirectory(const wxString& src)
{
	size_t pos = src.find_first_of(wxFileName::GetPathSeparators());
	if (pos == wxString::npos)
		return wxString();
	else
		return wxString(src.begin(), src.begin() + pos);
}
