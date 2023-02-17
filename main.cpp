#include <stdio.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#define GLFW_INCLUDE_ES3
#include <GLES3/gl3.h>
#include <GLFW/glfw3.h>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <iostream>
#include <queue>

GLFWwindow* g_window;
ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
bool show_demo_window = true;
bool show_another_window = false;
int g_width;
int g_height;

EM_JS(int, canvas_get_width, (), {
  return Module.canvas.width;
});

EM_JS(int, canvas_get_height, (), {
  return Module.canvas.height;
});

EM_JS(void, resizeCanvas, (), {
  js_resizeCanvas();
});

void on_size_changed()
{
  glfwSetWindowSize(g_window, g_width, g_height);

  ImGui::SetCurrentContext(ImGui::GetCurrentContext());
}

enum class NodeType
{
  WEIGHT = 0,
  ASSET,
  CONDITIONAL,
  COUNT
};
static constexpr const char * const node_types[] = {"WEIGHT", "ASSET", "CONDITIONAL"};

enum class FunctionType
{
  MovingAverage,
  ExponentialMovingAverage,
  COUNT
};

enum class WeightType
{
  Evenly,
  Specified,
  COUNT
};

enum class OperatorType
{
  Smaller,
  SmallerOrEqual,
  Equal,
  GreaterOrEqual,
  Greater,
  COUNT
};
struct Condition
{
  FunctionType left_function_type;
  std::string left_asset_name;
  OperatorType operator_type;
  FunctionType right_function_type;
  std::string right_asset_name;
  bool is_right_fixed;
  double fixed_value;
};

struct Conditions
{
  std::vector<Condition> conditions;
  // The size of |is_and| is the size of |conditions| - 1.
  std::vector<bool> is_and;
};
struct TreeNode {
    std::string asset = "+";
    WeightType weight_type;
    Conditions conditions;
    NodeType node_type;
    std::vector<std::shared_ptr<TreeNode>> children;
};

void dfs(std::shared_ptr<TreeNode> node)
{
  if (!node) {
    return;
  }
  if (node->node_type == NodeType::ASSET) {
    ImGui::Text("DefaultAsset");
    return;
  }
  // static uint64_t node_id = 0;
  if (ImGui::TreeNode(node->asset.c_str())) {
    ImGui::Text("Weight equal");
    if (ImGui::Button("Add a Block"))
    {
      ImGui::OpenPopup("Add Block");
    }
    static int selected_type = -1;
    
    std::shared_ptr<TreeNode> new_node = std::make_shared<TreeNode>();
    if (ImGui::BeginPopup("Add Block"))
    {
      ImGui::SeparatorText("Options");
      for (int i = 0; i < IM_ARRAYSIZE(node_types); i++)
      {
        if (ImGui::Selectable(node_types[i]))
        {
          selected_type = i;
          new_node->node_type = static_cast<NodeType>(i);
        }
      }
      ImGui::EndPopup();
    }
    if (selected_type != -1) {
      node->children.push_back(new_node);
      selected_type = -1;
    }

    for (auto& child: node->children) {
      dfs(child);
    }
    ImGui::TreePop();
  }
};

void loop()
{
  int width = canvas_get_width();
  int height = canvas_get_height();

  if (width != g_width || height != g_height)
  {
    g_width = width;
    g_height = height;
    on_size_changed();
  }

  glfwPollEvents();

  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();

  // Show a simple window.
  // Tip: if we don't call ImGui::Begin()/ImGui::End() the widgets
  // automatically appears in a window called "Debug".
  {
      ImGui::SetNextWindowPos(ImVec2(30, 30), ImGuiCond_FirstUseEver);
      ImGui::SetNextWindowSize(ImVec2(300, 400), ImGuiCond_FirstUseEver);
      ImGui::Begin("Dev window");
      ImGui::Text("Hello from a dev window!");
      static std::shared_ptr<TreeNode> root = std::make_shared<TreeNode>();
      dfs(root);
      ImGui::End();
  }

  // Show the ImGui demo window. 
  // Most of the sample code is in ImGui::ShowDemoWindow().
  // Read its code to learn more about Dear ImGui!
  if (show_demo_window)
  {
      ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiCond_FirstUseEver); // Normally user code doesn't need/want to call this because positions are saved in .ini file anyway. Here we just want to make the demo initial state a bit more friendly!
      ImGui::ShowDemoWindow(&show_demo_window);
  }

  ImGui::Render();

  int display_w, display_h;
  glfwMakeContextCurrent(g_window);
  glfwGetFramebufferSize(g_window, &display_w, &display_h);
  glViewport(0, 0, display_w, display_h);
  glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
  glClear(GL_COLOR_BUFFER_BIT);

  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
  glfwMakeContextCurrent(g_window);
}


int init_gl()
{
  if( !glfwInit() )
  {
      fprintf( stderr, "Failed to initialize GLFW\n" );
      return 1;
  }

  //glfwWindowHint(GLFW_SAMPLES, 4); // 4x antialiasing
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // We don't want the old OpenGL

  // Open a window and create its OpenGL context
  int canvasWidth = 800;
  int canvasHeight = 600;
  g_window = glfwCreateWindow( canvasWidth, canvasHeight, "WebGui Demo", NULL, NULL);
  if( g_window == NULL )
  {
      fprintf( stderr, "Failed to open GLFW window.\n" );
      glfwTerminate();
      return -1;
  }
  glfwMakeContextCurrent(g_window); // Initialize GLEW

  return 0;
}


int init_imgui()
{
  // Setup Dear ImGui binding
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui_ImplGlfw_InitForOpenGL(g_window, true);
  ImGui_ImplOpenGL3_Init();

  // Setup style
  ImGui::StyleColorsDark();

  ImGuiIO& io = ImGui::GetIO();

  // Load Fonts
  io.Fonts->AddFontFromFileTTF("data/xkcd-script.ttf", 23.0f);
  io.Fonts->AddFontFromFileTTF("data/xkcd-script.ttf", 18.0f);
  io.Fonts->AddFontFromFileTTF("data/xkcd-script.ttf", 26.0f);
  io.Fonts->AddFontFromFileTTF("data/xkcd-script.ttf", 32.0f);
  io.Fonts->AddFontDefault();

  resizeCanvas();

  return 0;
}


int init()
{
  init_gl();
  init_imgui();
  return 0;
}


void quit()
{
  glfwTerminate();
}


extern "C" int main(int argc, char** argv)
{
  if (init() != 0) return 1;

  #ifdef __EMSCRIPTEN__
  emscripten_set_main_loop(loop, 0, 1);
  #endif

  quit();

  return 0;
}
