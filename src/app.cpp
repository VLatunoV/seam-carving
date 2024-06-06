#include <stdio.h>

#include "app.h"
#include "GLFW/glfw3.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

static void glfw_errorCallback(int error, const char* description) {
	fprintf(stderr, "GLFW error [%d]: %s\n", error, description);
}

static void glfw_dropFuncCapture(GLFWwindow* window, int path_count, const char* paths[]) {
	ImageManager& imgMan = CallbackBridge::getInstance().getApp()->imageManager;
	int fileAccepted = -1;
	for (int i=0; i<path_count; ++i) {
		if (imgMan.accepts(paths[i])) {
			fileAccepted = i;
			break;
		}
	}

	if (fileAccepted == -1) {
		if (path_count == 1) {
			printf("Cannot load file \"%s\"\n", paths[0]);
		} else {
			printf("Cannot load any file of:\n");
			for (int i=0; i<path_count; ++i) {
				printf("  %s\n", paths[i]);
			}
		}
		return;
	}

	printf("Loading image \"%s\"\n", paths[fileAccepted]);
	imgMan.triggerLoad(paths[fileAccepted]);
}

// ################################################################################################################################
// # ComponentGLFW
// ################################################################################################################################

bool ComponentGLFW::initialize() {
	glfwSetErrorCallback(glfw_errorCallback);
	initialized = (glfwInit() != 0);
	if (!initialized)
		return false;

	// Create window with graphics context
	window = glfwCreateWindow(1280, 720, "Dear ImGui GLFW+OpenGL3 example", nullptr, nullptr);
	if (window == nullptr)
		return false;
	glfwMakeContextCurrent(window);
	glfwSwapInterval(1); // Enable vsync

	glfwSetDropCallback(window, glfw_dropFuncCapture);

	printVersion();
	return initialized;
}

void ComponentGLFW::cleanup() {
	if (window) {
		glfwDestroyWindow(window);
		window = nullptr;
	}
	if (initialized) {
		glfwTerminate();
	}
}

void ComponentGLFW::printVersion() {
	const char* gpu = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
	const char* version = reinterpret_cast<const char*>(glGetString(GL_VERSION));
	auto msgHelper = [](const char* msg) {
		return msg ? msg : "Missing";
	};
	printf("Device: %s\n", msgHelper(gpu));
	printf("OpenGL version: %s\n", msgHelper(version));
	printf("GLFW version: %s\n", msgHelper(glfwGetVersionString()));
}

// ################################################################################################################################
// # ComponentImGui
// ################################################################################################################################

bool ComponentImGui::initialize(GLFWwindow* window) {
	// Keeps track of which initialization functions are successful in case of partial initialization.
	enum Rollback: uint32_t {
		Rollback_Context = 1<<0,
		Rollback_GLFW_Init = 1<<1,
		Rollback_OpenGL_Init = 1<<2,
	};
	uint32_t rollback = 0;
	initialized = true;

	IMGUI_CHECKVERSION();
	{
		// Initialize the context
		ImGui::CreateContext();
		rollback |= Rollback_Context;
	}
	ImGui::StyleColorsDark();

	// Setup Platform/Renderer backends
	if (initialized) {
		initialized = ImGui_ImplGlfw_InitForOpenGL(window, true);
		if (initialized) rollback |= Rollback_GLFW_Init;
	}
	if (initialized) {
		initialized = ImGui_ImplOpenGL3_Init();
		if (initialized) rollback |= Rollback_OpenGL_Init;
	}

	// Do rollback of partial initialization
	if (!initialized) {
		if (rollback & Rollback_OpenGL_Init) {
			ImGui_ImplOpenGL3_Shutdown();
		}
		if (rollback & Rollback_GLFW_Init) {
			ImGui_ImplGlfw_Shutdown();
		}
		if (rollback & Rollback_Context) {
			ImGui::DestroyContext();
		}
	}

	return initialized;
}

void ComponentImGui::cleanup() {
	if (!initialized) return;

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}

// ################################################################################################################################
// # CallbackBridge
// ################################################################################################################################

CallbackBridge CallbackBridge::instance;

// ################################################################################################################################
// # App
// ################################################################################################################################

App::App()
	: canvas(imageManager)
{
	CallbackBridge::getInstance().setApp(*this);

	initialized = true;
	if (initialized) initialized = glfw.initialize();
	if (initialized) initialized = imgui.initialize(glfw.window);
}

App::~App() {
	imgui.cleanup();
	glfw.cleanup();
}

void App::run() {
	if (!initialized) return;

	// Do some setup
	setup();
	bool show_demo_window = true;
	bool show_another_window = false;
	ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
	ImGuiIO& io = ImGui::GetIO();

	// Main loop
	while (!glfwWindowShouldClose(glfw.window)) {
		// Poll and handle events (inputs, window resize, etc.)
		glfwPollEvents();

		// Update state based on user input
		updateState();

		// Start the Dear ImGui frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		// 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
		if (show_demo_window)
			ImGui::ShowDemoWindow(&show_demo_window);

		// 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named window.
		{
			static float f = 0.0f;
			static int counter = 0;

			ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

			ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
			ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
			ImGui::Checkbox("Another Window", &show_another_window);

			ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
			ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

			if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
				counter++;
			ImGui::SameLine();
			ImGui::Text("counter = %d", counter);

			ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
			ImGui::Text("Window size: %d x %d", displayWidth, displayHeight);
			ImGui::End();
		}

		// 3. Show another simple window.
		if (show_another_window)
		{
			ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
			ImGui::Text("Hello from another window!");
			if (ImGui::Button("Close Me"))
				show_another_window = false;
			ImGui::End();
		}

		// Rendering
		ImGui::Render();
		glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
		glClear(GL_COLOR_BUFFER_BIT);

		canvas.draw();

		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(glfw.window);
	}
}

void App::setup() {
	ImGuiIO& io = ImGui::GetIO();
	io.IniFilename = nullptr;
}

void App::updateState() {
	glfwGetFramebufferSize(glfw.window, &displayWidth, &displayHeight);
	canvas.updateCanvasSize(displayWidth, displayHeight);

	if (imageManager.hasNewImage()) {
		canvas.makeTexture();
	}
}
