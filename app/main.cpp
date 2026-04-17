#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include <iostream>
#include <cmath>
#include <vector>
#include <utility>
#include <numbers>
#include <stdexcept>
#include <algorithm>

#include "network/TopologyLoader.hpp"
#include "network/Topology.hpp"
#include "engine/core/SimulationEngine.hpp"
#include "engine/events/PacketGenerationEvent.hpp"
#include "engine/core/Stats.hpp"

#include "gui/include/MetricsPannel.hpp"
#include "gui/include/Window.hpp"
#include "engine/core/SimulationState.hpp"

using namespace kns;
using namespace interface;

// Helper Functions
void generatePackets(SimulationEngine& engine, const Topology& topo) {
    for (int i = 0; i < 100; i++) {
        engine.schedule(std::make_unique<PacketGenerationEvent>(
            i * 0.001,
            i % topo.size(),
            (i + 1) % topo.size()
        ));
    }
}

std::vector<std::pair<float, float>> generatePositions(const Topology& topo) {
    std::vector<std::pair<float, float>> positions;
    positions.reserve(topo.size());

    for (int i = 0; i < topo.size(); i++) {
        std::pair<float, float> pair = {
            640.0f + 120.0f * std::cos(2.0f * std::numbers::pi * i / topo.size()),
            260.0f + 120.0f * std::sin(2.0f * std::numbers::pi * i / topo.size())
        };
        positions.push_back(pair);
    }
    return positions;
}

// GUI & Rendering Functions
void generateWindow(SimulationEngine& engine, SimulationState& state, const Stats& stats, int& packetSize) {
    ImGui::Begin("Stats");

    if (state == SimulationState::Paused && ImGui::Button("Step")) {
        if (engine.hasEvents()) {
            engine.processEvent();
        }
    }

    MetricsPannel panel;
    panel.render(stats);

    if (ImGui::Button(state == SimulationState::Paused ? "Resume" : "Pause")) {
        state = (state == SimulationState::Paused)
            ? SimulationState::Running
            : SimulationState::Paused;
    }

    static float lossProb = 0.0f;
    if (ImGui::SliderFloat("Loss Probability", &lossProb, 0.0f, 1.0f)) {
        engine.setGlobalLossProb(lossProb);
    }

    // packetSize is now passed by reference, so the slider will correctly update it globally
    if (ImGui::SliderInt("Packet Size (bytes)", &packetSize, 100, 10'000)) {
        engine.setGlobalPacketSize(packetSize);
    }

    ImGui::Text("Current packet size: %d bytes", packetSize);

    ImGui::End();
}

void drawLinks(ImDrawList* draw_list, const Topology& topo, const std::vector<std::pair<float, float>>& positions) {
    for (int i = 0; i < topo.size(); i++) {
        const auto& links = topo.getLinksFromNode(i);
        for (const auto& link : links) {
            ImVec2 p1 = ImVec2(positions[link.from].first, positions[link.from].second);
            ImVec2 p2 = ImVec2(positions[link.to].first, positions[link.to].second);

            ImU32 color = IM_COL32(255, 255, 0, 255);
            float thickness = 2.0f;

            draw_list->AddLine(p1, p2, color, thickness);
        }
    }
}

void drawNodes(ImDrawList* draw_list, const Topology& topo, const std::vector<std::pair<float, float>>& positions) {
    for (int i = 0; i < topo.size(); i++) {
        draw_list->AddCircleFilled(
            ImVec2(positions[i].first, positions[i].second),
            10.0f,
            IM_COL32(100, 200, 100, 255)
        );
    }
}

void drawPackets(ImDrawList* draw_list, const std::vector<std::pair<float, float>>& positions, SimulationEngine& engine) {
    const auto& packets = engine.getPacketsInTransit();
    
    for (const auto& packet : packets) {
        double t = (engine.now() - packet.departure_time) / (packet.arrival_time - packet.departure_time);
        t = std::max(0.0, std::min(1.0, t));

        double x = positions[packet.from_node].first +
            (positions[packet.to_node].first - positions[packet.from_node].first) * t;

        double y = positions[packet.from_node].second +
            (positions[packet.to_node].second - positions[packet.from_node].second) * t;

        draw_list->AddCircleFilled(ImVec2(static_cast<float>(x), static_cast<float>(y)), 10.0f, IM_COL32(200, 100, 200, 255));
    }
}

void visualizeWindow(SimulationEngine& engine, 
                    const Topology& topo,
                    const std::vector<std::pair<float, float>>& positions, 
                    SimulationState& state, 
                    GLFWwindow* window, 
                    int& packetSize) {

    while (!glfwWindowShouldClose(window)) {

        if (state == SimulationState::Running && engine.hasEvents()) {
            engine.processEvent();
        }

        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        Stats stats = engine.getStats();

        // Pass by reference fixes apply here
        generateWindow(engine, state, stats, packetSize);

        ImDrawList* draw_list = ImGui::GetBackgroundDrawList();

        drawLinks(draw_list, topo, positions);
        drawNodes(draw_list, topo, positions);
        drawPackets(draw_list, positions, engine);

        ImGui::Render();
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }
}

void shutdownWindow(GLFWwindow* window) {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
}

// Main Execution
int main(int argc, char* argv[]) {
    if (argc < 2) {
        throw std::runtime_error("Usage: ./kns_app <topology_file>");
    }

    SimulationState state = SimulationState::Running;
    Topology topo = TopologyLoader::load_topology(argv[1]);
    SimulationEngine engine(topo);

    int packetSize = 1000;
    engine.setGlobalPacketSize(packetSize);

    generatePackets(engine, topo);

    std::vector<std::pair<float, float>> positions = generatePositions(topo);

    Window windowMethods;
    GLFWwindow* window = windowMethods.generate_window();

    if (!window) {
        throw std::runtime_error("Failed to create GLFW window");
    }

    visualizeWindow(engine, topo, positions, state, window, packetSize);

    shutdownWindow(window);

    return 0;
}