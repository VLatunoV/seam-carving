#include <assert.h>
#include <filesystem>
//#include <shlwapi.h>
#include <shobjidl_core.h> // For file browser dialog

#include "saveHandler.h"

static const COMDLG_FILTERSPEC fileTypeOptions[] = {
	{L"Any", L"*.*"},
	{L"Bitmap", L"*.bmp"},
	{L"JPEG", L"*.jpg;*.jpeg"},
	{L"PNG", L"*.png"},
};

static const LPWSTR defaultExt = L"png";


SaveImageHandler::SaveImageHandler() {
	HRESULT result = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	if (SUCCEEDED(result)) {
		initialized = true;
		result = CoCreateInstance(CLSID_FileSaveDialog, nullptr, CLSCTX_ALL, IID_IFileSaveDialog, reinterpret_cast<void**>(&pFileSave));
		if (SUCCEEDED(result)) {
			pFileSave->SetFileTypes(sizeof(fileTypeOptions)/sizeof(fileTypeOptions[0]), fileTypeOptions);
			pFileSave->SetDefaultExtension(L"png");
			pFileSave->SetFileTypeIndex(4); // Set the default file choice to PNG
		}
	}
}

SaveImageHandler::~SaveImageHandler() {
	if (initialized) {
		if (pFileSave) {
			pFileSave->Release();
		}
		CoUninitialize();
	}
}

void SaveImageHandler::setImageLoaded(const char* _path) {
	std::filesystem::path path = _path;
	filenameSuggestion = path.stem().wstring() + L"_seam";
	directorySuggestion = path.parent_path().wstring();
}

bool SaveImageHandler::getSavePath(std::string& savePath) {
	HRESULT result = 0;
	// Set the default save name.
	result = pFileSave->SetFileName(filenameSuggestion.c_str());
	assert(SUCCEEDED(result));

	// Set the default save directory.
	IShellItem* pItem = nullptr;
	result = SHCreateItemFromParsingName(directorySuggestion.c_str(), nullptr, IID_PPV_ARGS(&pItem));
	assert(SUCCEEDED(result));
	result = pFileSave->SetFolder(pItem);
	pItem->Release();
	assert(SUCCEEDED(result));

	// Show the Save dialog box.
	result = pFileSave->Show(nullptr);
	if (SUCCEEDED(result)) {
		// Get the file name from the dialog box.
		result = pFileSave->GetResult(&pItem);
		assert(SUCCEEDED(result));
		PWSTR pszFilePath;
		result = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);
		pItem->Release();
		assert(SUCCEEDED(result));

		// Set the result after converting from wchar to char
		std::filesystem::path resultPath = pszFilePath;
		savePath = resultPath.string();

		CoTaskMemFree(pszFilePath);
		return true;
	}

	return false;
}
