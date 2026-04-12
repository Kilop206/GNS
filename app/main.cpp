#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include <iostream>

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

    GLFWwindow* window = glfwCreateWindow(720, 1280, "KNS Simulator", nullptr, nullptr);
    
    if (window == nullptr) return 1;

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    while (!glfwWindowShouldClose(window)) {
        engine.processEvent();

        glfwPollEvents();
        
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        
        Stats stats = engine.getStats();

        ImGui::Begin("KNS");
        ImGui::Text("Packets sent: %d\nPackets Deliveried: %d\nPackets Lost: %d\nAverage Latency: %f", 
            stats.packets_sent, stats.packets_delivered, stats.packets_lost, (stats.total_latency / stats.packets_delivered));
        ImGui::End();
        
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