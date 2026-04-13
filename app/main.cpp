#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include <iostream>
#include <cmath>
#include <vector>
#include <utility>
#include <numbers>

#include "network/TopologyLoader.hpp"
#include "network/Topology.hpp"
#include "engine/core/SimulationEngine.hpp"
#include "engine/events/PacketGenerationEvent.hpp"
#include "engine/core/Stats.hpp"

using namespace kns;

int main(int argc, char* argv[]) {
    if (!glfwInit()) return 1;

    Topology topo = TopologyLoader::load_topology(argv[1]);

    SimulationEngine engine(topo);

    for (int i = 0; i < 10; i++) {
        engine.schedule(std::make_unique<PacketGenerationEvent>(
            i * 0.01,
            i % topo.size(),
            (i + 1) % topo.size(),
            i * 1000
        ));
    }

    GLFWwindow* window = glfwCreateWindow(1280, 720, "KNS", nullptr, nullptr);
    
    if (window == nullptr) return 1;

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    std::vector<std::pair<float, float>> positions;

    for (int i = 0; i < topo.size(); i++) {
        std::pair pair = {
            640 + 120 * std::cos(2 * std::numbers::pi * i / topo.size()),
            260 + 120 * std::sin(2 * std::numbers::pi * i / topo.size())};
        positions.push_back(pair);
    }

    while (!glfwWindowShouldClose(window)) {
        if (engine.hasEvents()) {
            engine.processEvent();
        }

        glfwPollEvents();
        
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        
        Stats stats = engine.getStats();

        ImDrawList* draw_list = ImGui::GetBackgroundDrawList();

        for (int i = 0; i < topo.size(); i ++) {
            for (int j = 0; j < topo.getLinksFromNode(i).size(); j++) {

                Link link = topo.getLinksFromNode(i)[j];

                ImVec2 p1 = ImVec2(positions[link.from].first, positions[link.from].second);
                ImVec2 p2 = ImVec2(positions[link.to].first, positions[link.to].second);
                
                ImU32 color = IM_COL32(255, 255, 0, 255);
                float thickness = 2.0f;

                draw_list->AddLine(p1, p2, color, thickness);
            }
        }

        for (int i = 0; i < topo.size(); i++) {
            draw_list->AddCircleFilled(ImVec2(positions[i].first, positions[i].second), 10.0f, IM_COL32(100, 200, 100, 255));
        }
        
        ImGui::Render();
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}