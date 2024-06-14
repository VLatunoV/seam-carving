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

static void glfw_cursorPositionFunction(GLFWwindow* window, double xpos, double ypos) {
	App* app = CallbackBridge::getInstance().getApp();
	MouseState& mouse = app->mouse;
	const int xOffset = int(xpos) - mouse.px;
	const int yOffset = int(ypos) - mouse.py;
	mouse.px += xOffset;
	mouse.py += yOffset;

	if (mouse.mmbPressed) {
		app->canvas.pan(xOffset, yOffset);
	}
}

static void glfw_mouseButtomFunction(GLFWwindow* window, int button, int action, int mods) {
	MouseState& mouse = CallbackBridge::getInstance().getApp()->mouse;
	if (button == GLFW_MOUSE_BUTTON_MIDDLE) {
		mouse.mmbPressed = (action == GLFW_PRESS);
	}
}

static void glfw_scrollFunction(GLFWwindow* window, double xoffset, double yoffset) {
	App* app = CallbackBridge::getInstance().getApp();
	MouseState& mouse = app->mouse;
	app->canvas.zoom(int(yoffset), mouse.px, mouse.py);
}

static void glfw_keyboardFunction(GLFWwindow* window, int key, int scancode, int action, int mods) {
	if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
		App* app = CallbackBridge::getInstance().getApp();
		app->canvas.resetTransform();
	}
	if (key == GLFW_KEY_E && action == GLFW_PRESS) {
		App* app = CallbackBridge::getInstance().getApp();
		app->canvas.toggleTexture();
	}
}

static void tooltip(const char* desc) {
	ImGui::SameLine();
	ImGui::TextDisabled("(?)");
	if (ImGui::BeginItemTooltip()) {
		ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
		ImGui::TextUnformatted(desc);
		ImGui::PopTextWrapPos();
		ImGui::EndTooltip();
	}
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
	glfwSetCursorPosCallback(window, glfw_cursorPositionFunction);
	glfwSetMouseButtonCallback(window, glfw_mouseButtomFunction);
	glfwSetScrollCallback(window, glfw_scrollFunction);
	glfwSetKeyCallback(window, glfw_keyboardFunction);

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
	initialized = true;
	if (initialized) initialized = glfw.initialize();
	if (initialized) initialized = imgui.initialize(glfw.window);

	CallbackBridge::getInstance().setApp(*this);
	imageManager.registerObserver(*this);
	imageManager.registerObserver(canvas);
}

App::~App() {
	imageManager.unregisterObserver(*this);
	imageManager.unregisterObserver(canvas);

	imgui.cleanup();
	glfw.cleanup();
}

void App::run() {
	if (!initialized) return;

	// Do some setup
	float clearColor[3] = {0.45f, 0.55f, 0.60f};
	bool show_demo_window = false;
	float emaDeltaTime = 1.0f / 60.0f;
	const float emaDecay = 0.95f;
	int displayWidth = 0;
	int displayHeight = 0;

	ImGuiIO& io = ImGui::GetIO();
	io.IniFilename = nullptr;

	// Main loop
	while (!glfwWindowShouldClose(glfw.window)) {
		// Poll and handle events (inputs, window resize, etc.)
		glfwPollEvents();

		// Update state based on user input
		glfwGetFramebufferSize(glfw.window, &displayWidth, &displayHeight);
		canvas.update(displayWidth, displayHeight);

		emaDeltaTime = emaDecay * emaDeltaTime + (1.0f-emaDecay) * io.DeltaTime;

		// Draw ImGui UI
		{
			ImGui_ImplOpenGL3_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();

			// 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
			if (show_demo_window)
				ImGui::ShowDemoWindow(&show_demo_window);

			ImGui::Begin("Seam carving", nullptr, ImGuiWindowFlags_NoResize);
			ImGui::SetWindowPos({0, 0}, ImGuiCond_Once);

			ImGui::SeparatorText("Controls");
			ImGui::Text("Drag and drop an image to load.");
			ImGui::Text("Use middle mouse button to move and scroll to zoom.");
			ImGui::Text("Press SPACE to reset.");
			ImGui::Text("Press E to show image energy.");

			ImGui::SeparatorText("Seam carving");
			ImGui::Text("Target size");
			tooltip("This is the final size after removing seams from the image.");
			ImGui::SliderInt("Width", &targetWidth, bool(imageWidth), imageWidth);
			ImGui::SliderInt("Height", &targetHeight, bool(imageHeight), imageHeight);
			if (ImGui::Button("Carve")) {
				imageManager.triggerSeam(targetWidth, targetHeight);
			}

			ImGui::SeparatorText("Settings");
			ImGui::SliderInt("Zoom speed", &canvas.zoomSpeed, 1, 9, nullptr, ImGuiSliderFlags_NoInput);
			ImGui::ColorEdit3("Background color", clearColor);

			ImGui::SeparatorText("Info");
			ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f * emaDeltaTime, 1.0f / emaDeltaTime);
			ImGui::Text("Window size: %d x %d", displayWidth, displayHeight);
			ImGui::Text("Image size: %d x %d", imageWidth, imageHeight);
			ImGui::Text("Zoom level: %.03f", canvas.calcScale(canvas.zoomValue));
			ImGui::Checkbox("Demo window", &show_demo_window);
			if (ImGui::Button("Pop")) {
				ImGui::SetWindowPos({0, 0});
			}
			ImGui::End();

			// Rendering
			ImGui::Render();
		}
		glClearColor(clearColor[0], clearColor[1], clearColor[2], 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		canvas.draw();

		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(glfw.window);
	}
}

void App::onImageChange() {
	Image& originalImage = imageManager.getOriginalImage();
	imageWidth = originalImage.getWidth();
	imageHeight = originalImage.getHeight();
	targetWidth = imageWidth;
	targetHeight = imageHeight;
}

void App::onImageSeamed() {
	// Empty
}
