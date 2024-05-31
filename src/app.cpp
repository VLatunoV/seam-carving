#include "app.h"
#include <stdio.h>

#include "GLFW/glfw3.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

static void glfw_errorCallback(int error, const char* description) {
	fprintf(stderr, "GLFW error [%d]: %s\n", error, description);
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
// # App
// ################################################################################################################################

App::App() {
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
}
