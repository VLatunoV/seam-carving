#pragma once

struct GLFWwindow;

class App {
public:
	/// Initializes the app.
	App();

	/// Does cleanup.
	~App();

	/// Run the app.
	void run();

private:
	union {
		struct {
			bool initialized : 1; ///< True if the all initialization was successful.
			bool glfwInitialized : 1; ///< True if glfw initialization returned success.
		};
		unsigned int allFlags = 0;
	};

	GLFWwindow* window = nullptr;

	/// Initialize the application.
	/// Creates the GL, GLFW contexts, creates the window.
	/// @return 0 if successful.
	int initialize();

	/// Destroys created contexts, windows, etc.
	void cleanup();
};
