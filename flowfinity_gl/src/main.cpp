// Dear ImGui: standalone example application for SDL2 + OpenGL
// (SDL is a cross-platform general purpose library for handling windows,
// inputs, OpenGL/Vulkan/Metal graphics context creation, etc.)

// Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs (same as your local docs/
// folder).
// - Introduction, links and more at the top of imgui.cpp

#include "editor.h"

#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_sdl2.h"
#include <GL/glew.h>
#include <SDL.h>

#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <SDL_opengles2.h>
#else
#include <SDL_opengl.h>
#endif

// This example can also compile and run with Emscripten! See
// 'Makefile.emscripten' for details.
#ifdef __EMSCRIPTEN__
#include "../libs/emscripten/emscripten_mainloop_stub.h"
#endif

// Main code
int main(int, char **) {
  // Setup SDL
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) !=
      0) {
    printf("Error: %s\n", SDL_GetError());
    return -1;
  }

  // Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
  // GL ES 2.0 + GLSL 100
  const char *glsl_version = "#version 100";
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#elif defined(__APPLE__)
  // GL 3.2 Core + GLSL 150
  const char *glsl_version = "#version 150";
  SDL_GL_SetAttribute(
      SDL_GL_CONTEXT_FLAGS,
      SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG); // Always required on Mac
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
#else
  // GL 3.0 + GLSL 130
  const char *glsl_version = "#version 130";
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#endif

  // From 2.0.18: Enable native IME.
#ifdef SDL_HINT_IME_SHOW_UI
  SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");
#endif

  // Create window with graphics context
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
  SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
  SDL_WindowFlags window_flags =
      (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE |
                        SDL_WINDOW_ALLOW_HIGHDPI);
  SDL_Window *window = SDL_CreateWindow(
      "Dear ImGui SDL2+OpenGL3 example", SDL_WINDOWPOS_CENTERED,
      SDL_WINDOWPOS_CENTERED, 1280, 720, window_flags);
  if (window == nullptr) {
    printf("Error: SDL_CreateWindow(): %s\n", SDL_GetError());
    return -1;
  }

  SDL_GLContext gl_context = SDL_GL_CreateContext(window);
  SDL_GL_MakeCurrent(window, gl_context);
  SDL_GL_SetSwapInterval(1); // Enable vsync

  // Setup Dear ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  (void)io;
  io.ConfigFlags |=
      ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
  io.ConfigFlags |=
      ImGuiConfigFlags_NavEnableGamepad; // Enable Gamepad Controls

  // Setup Dear ImGui style
  ImGui::StyleColorsDark();
  // ImGui::StyleColorsLight();

  // Setup Platform/Renderer backends
  ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
  ImGui_ImplOpenGL3_Init(glsl_version);

  // Load Fonts
  // - If no fonts are loaded, dear imgui will use the default font. You can
  // also load multiple fonts and use ImGui::PushFont()/PopFont() to select
  // them.
  // - AddFontFromFileTTF() will return the ImFont* so you can store it if you
  // need to select the font among multiple.
  // - If the file cannot be loaded, the function will return a nullptr. Please
  // handle those errors in your application (e.g. use an assertion, or display
  // an error and quit).
  // - The fonts will be rasterized at a given size (w/ oversampling) and stored
  // into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which
  // ImGui_ImplXXXX_NewFrame below will call.
  // - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use Freetype
  // for higher quality font rendering.
  // - Read 'docs/FONTS.md' for more instructions and details.
  // - Remember that in C/C++ if you want to include a backslash \ in a string
  // literal you need to write a double backslash \\ !
  // - Our Emscripten build process allows embedding fonts to be accessible at
  // runtime from the "fonts/" folder. See Makefile.emscripten for details.
  // io.Fonts->AddFontDefault();
  // io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf", 18.0f);
  // io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
  // io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
  // io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
  // ImFont* font =
  // io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f,
  // nullptr, io.Fonts->GetGlyphRangesJapanese()); IM_ASSERT(font != nullptr);

  // Our state
  bool show_demo_window = false;
  bool show_another_window = false;
  ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

  Editor editor = Editor();
  int success = editor.initialize(window, gl_context);
  if (success != 0) {
    return success;
  }

  // Main loop
  bool done = false;
#ifdef __EMSCRIPTEN__
  // For an Emscripten build we are disabling file-system access, so let's not
  // attempt to do a fopen() of the imgui.ini file. You may manually call
  // LoadIniSettingsFromMemory() to load settings from your own storage.
  io.IniFilename = nullptr;
  EMSCRIPTEN_MAINLOOP_BEGIN
#else
  while (!done)
#endif
  {
    // Poll and handle events (inputs, window resize, etc.)
    // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to
    // tell if dear imgui wants to use your inputs.
    // - When io.WantCaptureMouse is true, do not dispatch mouse input data to
    // your main application, or clear/overwrite your copy of the mouse data.
    // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input
    // data to your main application, or clear/overwrite your copy of the
    // keyboard data. Generally you may always pass all inputs to dear imgui,
    // and hide them from your application based on those two flags.
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      ImGui_ImplSDL2_ProcessEvent(&event);

      // run event thru our own editor
      if (!io.WantCaptureMouse && !io.WantCaptureKeyboard)
        editor.processEvent(event);

      if (event.type == SDL_QUIT)
        done = true;
      if (event.type == SDL_WINDOWEVENT &&
          event.window.event == SDL_WINDOWEVENT_CLOSE &&
          event.window.windowID == SDL_GetWindowID(window))
        done = true;
    }

    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    // 2. Show a simple window that we create ourselves. We use a Begin/End pair
    // to create a named window.
    {
      static int instances = 2000;
      static float size = 0.04f;
      static float damping = 0.96f;
      static float spacing = 0.05f;
      static float density = 0.26f;
      static float targetDensity = 1.2f;
      static float pressureMultiplier = 19.5f;
      static float gravity = -9.8f;
      static float inputRadius = 1.0f;
      static float inputStrengthMultiplier = 6.0f;
      static float viscosity = 0.075f;
      static bool randomLocationGenerated = false;
      static bool randomLocation = false;
      static float bounds[2]{7.5f, 4.0f};
      static int numColors = 6;

      // Colors
      static float color1[3]{0.03f, 0.29f, 0.86f};
      static float color2[3]{0.26f, 0.75f, 0.87f};
      static float color3[3]{0.19f, 0.79f, 0.62f};
      static float color4[3]{0.6f, 0.98f, 0.49f};
      static float color5[3]{0.99f, 0.82f, 0.03f};
      static float color6[3]{0.68f, 0.12f, 0.07f};

      ImGui::Begin("Particle Fluid Sim!"); // Create a window called "Hello,
                                           // world!" and append into it.

      ImGui::Text(
          editor.getStarted()
              ? "Edit Simulation Parameters, then Start."
              : "Edit the Live Parameters, or Reset!"); // Display some text
                                                        // (you can use a format
                                                        // strings too)
      ImGui::SliderFloat("Particle Radius", &size, 0.00f,
                         0.1f); // Edit 1 float using a slider from 0.0f to 1.0f
      ImGui::SliderFloat("Gravity", &gravity, -10.0f, 10.0f);
      if (!editor.getStarted()) {
        ImGui::SliderInt("Number of Particles", &instances, 1, 4000);
        ImGui::SliderFloat("Particle Spacing", &spacing, 0.0f, 1.0f);
        ImGui::Checkbox("Random Location", &randomLocation);
      } else {
        ImGui::SliderFloat("Input Radius", &inputRadius, 0.0f, 5.0f);
        ImGui::SliderFloat("Input Strength Multiplier",
                           &inputStrengthMultiplier, 0.0f, 25.0f);
        ImGui::SliderFloat2("Bounds", bounds, 0.0f, 10.0f);
      }
      if (ImGui::CollapsingHeader("Colors")) {
        ImGui::Text("Choose the Velocity Color Gradient!");
        ImGui::ColorEdit3("Color 1", (float *)&color1);
        ImGui::ColorEdit3("Color 2", (float *)&color2);
        ImGui::ColorEdit3("Color 3", (float *)&color3);
        ImGui::ColorEdit3("Color 4", (float *)&color4);
        ImGui::ColorEdit3("Color 5", (float *)&color5);
        ImGui::ColorEdit3("Color 6", (float *)&color6);
      }
      if (ImGui::CollapsingHeader("Advanced Settings")) {
        ImGui::SliderFloat("Density Radius", &density, 0.0f, 2.0f);
        ImGui::SliderFloat("Target Density", &targetDensity, 0.0f, 10.0f);
        ImGui::SliderFloat("Particle Damping", &damping, -1.0f, 1.0f);
        ImGui::SliderFloat("Viscosity", &viscosity, 0.0f, 0.3f);
        ImGui::SliderFloat("Pressure Multiplier", &pressureMultiplier, 0.0f,
                           75.0f);
      }
      // ImGui::ColorEdit3(
      //     "clear color",
      //     (float *)&clear_color); // Edit 3 floats representing a color

      if (ImGui::Button(
              editor.getStarted()
                  ? "Reset Simulation"
                  : "Start Simulation")) // Buttons return true when clicked
                                         // (most widgets return true when
                                         // edited/activated)
      {
        if (!editor.getStarted()) {
          editor.startSimulation();
        } else {
          editor.resetSimulation();
        }
      }

      ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
                  1000.0f / io.Framerate, io.Framerate);
      ImGui::End();

      // Set Variables
      if (!editor.getStarted()) {
        editor.setNumInstances(instances);
        editor.setParticleSpacing(spacing);
        editor.setDensityRadius(density);

        // If the random location status has changed
        if (randomLocation != randomLocationGenerated) {
          // If the random location is being turned off, tell the editor to
          // regenerate
          if (!randomLocation && randomLocationGenerated) {
            editor.setRandomLocationGenerated(false);
          }
          // Tell the editor about the new status
          editor.setRandomLocation(randomLocation);
          // Save the previous status
          randomLocationGenerated = randomLocation;
        }
      }
      // Set these even when started
      editor.setParticleSize(size);
      editor.setParticleDamping(damping);
      editor.setTargetDensity(targetDensity);
      editor.setPressureMultiplier(pressureMultiplier);
      editor.setGravity(gravity);
      editor.setInputRadius(inputRadius);
      editor.setInputStrengthMultiplier(inputStrengthMultiplier);
      editor.setViscosityStrength(viscosity);
      editor.setBounds(glm::vec2(bounds[0], bounds[1]));

      // Set Colors
      if (editor.getStarted()) {
        std::vector<glm::vec3> colors;

        colors.push_back(glm::vec3(color1[0], color1[1], color1[2]));
        colors.push_back(glm::vec3(color2[0], color2[1], color2[2]));
        colors.push_back(glm::vec3(color3[0], color3[1], color3[2]));
        colors.push_back(glm::vec3(color4[0], color4[1], color4[2]));
        colors.push_back(glm::vec3(color5[0], color5[1], color5[2]));
        colors.push_back(glm::vec3(color6[0], color6[1], color6[2]));

        editor.setColors(colors);
      }
    }

    // Rendering
    ImGui::Render();
    glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);

    if (!editor.getStarted()) {
      editor.resetSimulation();
    }
    editor.paint();

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    SDL_GL_SwapWindow(window);
  }
#ifdef __EMSCRIPTEN__
  EMSCRIPTEN_MAINLOOP_END;
#endif

  // Cleanup
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplSDL2_Shutdown();
  ImGui::DestroyContext();

  SDL_GL_DeleteContext(gl_context);
  SDL_DestroyWindow(window);
  SDL_Quit();

  return 0;
}
