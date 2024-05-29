#include "app.h"
#include <stdio.h>

#include "GLFW/glfw3.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

static void glfw_errorCallback(int error, const char* description) {
	fprintf(stderr, "GLFW error [%d]: %s\n", error, description);
}

App::App() {
	initialized = (initialize() == 0);
}

App::~App() {
	cleanup();
}

void App::run() {
	if (!initialized) return;
}

int App::initialize() {
	// Initialize GLFW
	glfwSetErrorCallback(glfw_errorCallback);
	glfwInitialized = (glfwInit() != 0);
	if (glfwInitialized == 0) {
		return 1;
	}

	// Create window with graphics context
	window = glfwCreateWindow(1280, 720, "Dear ImGui GLFW+OpenGL3 example", nullptr, nullptr);
	if (window == nullptr)
		return 1;
	glfwMakeContextCurrent(window);
	glfwSwapInterval(1); // Enable vsync

	const char* gpu = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
	const char* version = reinterpret_cast<const char*>(glGetString(GL_VERSION));
	auto msgHelper = [](const char* msg) {
		return msg ? msg : "Missing";
	};
	printf("Device: %s\n", msgHelper(gpu));
	printf("OpenGL version: %s\n", msgHelper(version));
	printf("GLFW version: %s\n", msgHelper(glfwGetVersionString()));

	// Setup IMGUI
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
	ImGui::StyleColorsDark();

	// Setup Platform/Renderer backends
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init();

	return 0;
}

void App::cleanup() {
	if (glfwInitialized)
		glfwTerminate();
}
