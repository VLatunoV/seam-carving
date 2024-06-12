#pragma once
#include "canvas.h"
#include "image.h"

struct GLFWwindow;

/// Handles the GLFW part of the app.
class ComponentGLFW {
	friend class App;

	/// Initializes GLFW context.
	/// @return True if successful.
	bool initialize();

	/// Does cleanup.
	void cleanup();

	/// Print OpenGL and GLFW version information.
	void printVersion();

	bool initialized = false; ///< True if GLFW was successfully initialized.
	GLFWwindow* window = nullptr; ///< GLFW window handle.
};

/// Handles the ImGUI part of the app.
class ComponentImGui {
	friend class App;

	/// Initializes ImGUI state.
	/// @param[in] window The glfw window handle.
	/// @return True if successful.
	bool initialize(GLFWwindow* window);

	///< Does cleanup.
	void cleanup();

	bool initialized = false; ///< True if ImGUI was successfully initialized.
};

/// Makes the connection between callbacks and the Application. Singleton class.
/// This is used because GLFW callbacks pass only an opaque window pointer, so it is not possible to get
/// any reference to any object using them. So this class will create one static global instance, which
/// creates the bridge between the callback and the application.
class CallbackBridge {
	friend class App;

public:
	/// Return the singleton instance.
	static CallbackBridge& getInstance() {
		return instance;
	}

	/// Return the app instance. Should always be non-null.
	App* getApp() {
		return app;
	}

private:
	static CallbackBridge instance; ///< The singleton instance.
	App* app = nullptr; ///< App reference.

	/// Set the app pointer. Must be called before any callbacks are registered.
	void setApp(App& app) {
		this->app = &app;
	}
};

/// The mouse state on the previous frame.
struct MouseState {
	int px = 0; ///< Current mouse position.
	int py = 0; ///< Current mouse position.
	bool mmbPressed = false; ///< Middle mouse buttom pressed?
};

class App
	: public ImageManagerObserver
{
public:
	ImageManager imageManager;
	Canvas canvas;

	MouseState mouse;

	App(); ///< Initializes the app.
	~App(); ///< Does cleanup.

	void run(); ///< Run the app.

private:
	bool initialized = false; ///< True if everything was successfully initialized.
	ComponentGLFW glfw;
	ComponentImGui imgui;

	int imageWidth = 0; ///< current image's width.
	int imageHeight = 0; ///< Current image's height.
	int targetWidth = 0; ///< Final width after removing seams.
	int targetHeight = 0; ///< Final height after removing seams.

	// From ImageManagerObserver
	virtual void onImageChange() override;
};
