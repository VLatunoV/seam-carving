#pragma once
#include <string>

struct IFileSaveDialog;

/// Creates a windows file browser window that allows the user to select where to save the image.
class SaveImageHandler {
public:
	/// Constructor. Initializes the COM system and sets up the dialog window parameters.
	SaveImageHandler();
	/// Destructor. Destroys the COM system and window dialog.
	~SaveImageHandler();

	/// Called when an image is loaded. We save the image name and path, so that we can autofill
	/// a default file name for the saved image.
	void setImageLoaded(const char* path);

	/// Opens the dialog and waits for the user to select where to save. If they press "Save", the
	/// path is written back to the caller. Otherwise nothing is written.
	/// @param[out] savePath File path where the user selected to save the image.
	/// @return True if the dialog was accepted, false if it was canceled.
	bool getSavePath(std::string& savePath);

private:
	bool initialized = false; ///< Set to true if we successfully initialized the COM system.
	IFileSaveDialog* pFileSave = nullptr; ///< Handle to the save dialog.
	std::wstring filenameSuggestion; ///< Filename to suggest when saving the image.
	std::wstring directorySuggestion; ///< Directory to suggest when saving the image.
};
