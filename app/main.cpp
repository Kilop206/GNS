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

#include "gui/include/MetricsPannel.hpp"
#include "gui/include/Window.hpp"

using namespace kns;
using namespace interface;

int main(int argc, char* argv[]) {
    if (!glfwInit()) return 1;

    Topology topo = TopologyLoader::load_topology(argv[1]);

    SimulationEngine engine(topo);

    for (int i = 0; i < 100; i++) {
        engine.schedule(std::make_unique<PacketGenerationEvent>(
            i * 0.001,
            i % topo.size(),
            (i + 1) % topo.size(),
            i * 1000
        ));
    }

    std::vector<std::pair<float, float>> positions;

    for (int i = 0; i < topo.size(); i++) {
        std::pair pair = {
            640 + 120 * std::cos(2 * std::numbers::pi * i / topo.size()),
            260 + 120 * std::sin(2 * std::numbers::pi * i / topo.size())};
        positions.push_back(pair);
    }

    Window windowMethods;

    GLFWwindow* window = windowMethods.generate_window();

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

        for (int i = 0; i < engine.getPacketsInTransit().size(); i++) {
            double t = (engine.now() - engine.getPacketsInTransit()[i].departure_time) / 
                (engine.getPacketsInTransit()[i].arrival_time - engine.getPacketsInTransit()[i].departure_time);

            t = std::max(0.0, std::min(1.0, t));
            
            double x = positions[engine.getPacketsInTransit()[i].from_node].first + 
                (positions[engine.getPacketsInTransit()[i].to_node].first - positions[engine.getPacketsInTransit()[i].from_node].first) * t;

            double y = positions[engine.getPacketsInTransit()[i].from_node].second + 
                (positions[engine.getPacketsInTransit()[i].to_node].second - positions[engine.getPacketsInTransit()[i].from_node].second) * t;

            draw_list->AddCircleFilled(ImVec2(x, y), 10.0f, IM_COL32(200, 100, 200, 255));
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