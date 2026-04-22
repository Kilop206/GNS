#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>

#include "ImGuiFileDialog.h"

#include <iostream>
#include <cmath>
#include <vector>
#include <utility>
#include <numbers>
#include <stdexcept>
#include <algorithm>
#include <string>
#include <memory>

#include "network/TopologyLoader.hpp"
#include "network/Topology.hpp"
#include "network/Routing.hpp"
#include "engine/core/SimulationEngine.hpp"
#include "engine/events/PacketGenerationEvent.hpp"
#include "engine/core/Stats.hpp"
#include "gui/include/MetricsPannel.hpp"
#include "gui/include/Window.hpp"
#include "gui/include/LatencyChart.hpp"
#include "engine/core/SimulationState.hpp"

using namespace kns;
using namespace interface;

// Helper Functions
void generatePackets(SimulationEngine& engine, const Topology& topo) {
    if (topo.size() <= 0) {
        return;
    }

    for (int i = 0; i < 1000; i++) {
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

    if (topo.size() <= 0) {
        return positions;
    }

    for (int i = 0; i < topo.size(); i++) {
        std::pair<float, float> pair = {
            640.0f + 120.0f * std::cos(2.0f * std::numbers::pi_v<float> * i / topo.size()),
            260.0f + 120.0f * std::sin(2.0f * std::numbers::pi_v<float> * i / topo.size())
        };
        positions.push_back(pair);
    }
    return positions;
}

int pickNodeAtMouse(
    const std::vector<std::pair<float, float>>& positions,
    float radius
) {
    ImVec2 mouse_pos = ImGui::GetMousePos();

    for (int i = 0; i < static_cast<int>(positions.size()); i++) {
        float dx = mouse_pos.x - positions[i].first;
        float dy = mouse_pos.y - positions[i].second;
        float dist2 = dx * dx + dy * dy;

        if (dist2 <= radius * radius) {
            return i;
        }
    }

    return -1;
}

void generateWindow(
    SimulationEngine& engine,
    SimulationState& state,
    const Stats& stats,
    CircularBuffer& buffer,
    int& packetSize
) {
    ImGui::Begin("Stats");

    if (state == SimulationState::Paused && ImGui::Button("Step")) {
        if (engine.hasEvents()) {
            engine.processEvent();
        }
    }

    MetricsPannel panel;
    panel.render(stats, buffer);

    if (ImGui::Button(state == SimulationState::Paused ? "Resume" : "Pause")) {
        state = (state == SimulationState::Paused)
            ? SimulationState::Running
            : SimulationState::Paused;
    }

    static float lossProb = 0.0f;
    if (ImGui::SliderFloat("Loss Probability", &lossProb, 0.0f, 1.0f)) {
        engine.setGlobalLossProb(lossProb);
    }

    if (ImGui::SliderInt("Packet Size (bytes)", &packetSize, 100, 10'000)) {
        engine.setGlobalPacketSize(packetSize);
    }

    ImGui::Text("Current packet size: %d bytes", packetSize);

    ImGui::End();
}

void drawLinks(
    ImDrawList* draw_list,
    const Topology& topo,
    const std::vector<std::pair<float, float>>& positions
) {
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

void drawNodes(
    ImDrawList* draw_list,
    const Topology& topo,
    const std::vector<std::pair<float, float>>& positions,
    int selected_node
) {
    for (int i = 0; i < topo.size(); i++) {
        ImU32 color = (i == selected_node)
            ? IM_COL32(255, 255, 0, 255)
            : IM_COL32(100, 200, 100, 255);

        draw_list->AddCircleFilled(
            ImVec2(positions[i].first, positions[i].second),
            10.0f,
            color
        );

        draw_list->AddText(
            ImVec2(positions[i].first + 12.0f, positions[i].second - 6.0f),
            IM_COL32(255, 255, 255, 255),
            std::to_string(i).c_str()
        );
    }
}

void drawPackets(
    ImDrawList* draw_list,
    const std::vector<std::pair<float, float>>& positions,
    SimulationEngine& engine
) {
    const auto& packets = engine.getPacketsInTransit();

    for (const auto& packet : packets) {
        double denom = (packet.arrival_time - packet.departure_time);
        if (denom <= 0.0) {
            continue;
        }

        double t = (engine.now() - packet.departure_time) / denom;
        t = std::max(0.0, std::min(1.0, t));

        double x = positions[packet.from_node].first +
            (positions[packet.to_node].first - positions[packet.from_node].first) * t;

        double y = positions[packet.from_node].second +
            (positions[packet.to_node].second - positions[packet.from_node].second) * t;

        draw_list->AddCircleFilled(
            ImVec2(static_cast<float>(x), static_cast<float>(y)),
            10.0f,
            IM_COL32(200, 100, 200, 255)
        );
    }
}

void renderSelectedNodePanel(
    int selected_node,
    const std::vector<Routing::RoutingEntry>& routingTable
) {
    if (selected_node < 0) {
        return;
    }

    std::string title = "Node: " + std::to_string(selected_node);
    ImGui::Begin(title.c_str());

    ImGui::Text("Selected node: %d", selected_node);
    ImGui::Separator();

    if (routingTable.empty()) {
        ImGui::Text("Routing table is empty.");
    } else {
        for (const auto& entry : routingTable) {
            if (entry.distance == std::numeric_limits<double>::infinity()) {
                ImGui::Text("Dest: %d | Next: - | Dist: inf", entry.destination);
            } else {
                ImGui::Text(
                    "Dest: %d | Next: %d | Dist: %.2f",
                    entry.destination,
                    entry.next_hop,
                    entry.distance
                );
            }
        }
    }

    ImGui::End();
}

// --- Main Visualization Loop ---

void visualizeWindow(SimulationEngine& engine, Topology& topo, std::vector<std::pair<float, float>>& positions, 
                     SimulationState& state, GLFWwindow* window, CircularBuffer& buffer, int& packetSize) {
    int selected_node = -1;
    std::vector<Routing::RoutingEntry> routingTable;
    Routing routing;
    static bool firstFrame = true;

    while (!glfwWindowShouldClose(window)) {
        if (state == SimulationState::Running && engine.hasEvents()) engine.processEvent();
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        if (firstFrame && topo.size() == 0) {
            ImGuiFileDialog::Instance()->OpenDialog("TopologyKey", "Select Initial Topology", ".json");
            firstFrame = false;
        }

        generateWindow(engine, state, engine.getStats(), buffer, packetSize);

        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            selected_node = pickNodeAtMouse(positions, 10.0f);
            if (selected_node != -1) routingTable = routing.buildRoutingTable(topo, selected_node);
            else routingTable.clear();
        }

        ImDrawList* draw_list = ImGui::GetBackgroundDrawList();
        drawLinks(draw_list, topo, positions);
        drawNodes(draw_list, topo, positions, selected_node);
        drawPackets(draw_list, positions, engine);

        ImGui::Begin("Configurações");
        if (ImGui::Button("Load Topology")) {
             ImGuiFileDialog::Instance()->OpenDialog("TopologyKey", "Select File", ".json");
        }

        if (ImGuiFileDialog::Instance()->Display("TopologyKey", ImGuiWindowFlags_NoCollapse, ImVec2(400, 300))) {
            if (ImGuiFileDialog::Instance()->IsOk()) {
                std::string completePath = ImGuiFileDialog::Instance()->GetFilePathName();
                try {
                    topo = TopologyLoader::load_topology(completePath);
                    engine = SimulationEngine(topo); 
                    engine.setGlobalPacketSize(packetSize);
                    engine.setLatencyObserver([&buffer](double lat) { buffer.addLatencyToBuffer((float)lat); });
                    positions = generatePositions(topo);
                    generatePackets(engine, topo);
                    state = SimulationState::Paused;
                    selected_node = -1;
                    routingTable.clear();
                } catch (const std::exception& e) { std::cerr << "Load error: " << e.what() << std::endl; }
            }
            ImGuiFileDialog::Instance()->Close();
        }
        ImGui::End();

        renderSelectedNodePanel(selected_node, routingTable);
        ImGui::Render();
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
        firstFrame = false;
    }
}

void shutdownWindow(GLFWwindow* window) {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
}

int main(int argc, char* argv[]) {
    if (argc < 2 || argv[1] == nullptr) {
        std::cerr << "Usage: kns_app <topology_file.json>" << std::endl;
        return -1;
    }

    SimulationState state = SimulationState::Paused;

    Topology topo = TopologyLoader::load_topology(argv[1]);

    SimulationEngine engine(topo);
    CircularBuffer buffer;

    engine.setLatencyObserver([&buffer](double lat) {
        buffer.addLatencyToBuffer(static_cast<float>(lat));
    });

    int packetSize = 1000;
    engine.setGlobalPacketSize(packetSize);

    generatePackets(engine, topo);

    std::vector<std::pair<float, float>> positions = generatePositions(topo);

    Window windowMethods;
    GLFWwindow* window = windowMethods.generate_window();

    if (!window) {
        std::cerr << "Falha ao criar janela GLFW" << std::endl;
        return -1;
    }

    visualizeWindow(engine, topo, positions, state, window, buffer, packetSize);

    shutdownWindow(window);
    return 0;
}