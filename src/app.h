#pragma once

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
	void cleanup();

	bool initialized = false; ///< True if ImGUI was successfully initialized.
};

class App {
public:
	App(); ///< Initializes the app.
	~App(); ///< Does cleanup.

			/// Run the app.
	void run();

private:
	bool initialized = false; ///< True if everything was successfully initialized.
	ComponentGLFW glfw;
	ComponentImGui imgui;
};
