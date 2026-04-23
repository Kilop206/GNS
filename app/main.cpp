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
#include <limits>

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

// ------------------------------------------------------------
// Helper Functions
// ------------------------------------------------------------

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

    constexpr float centerX = 640.0f;
    constexpr float centerY = 260.0f;
    constexpr float radius = 120.0f;

    for (int i = 0; i < topo.size(); i++) {
        std::pair<float, float> pair = {
            centerX + radius * std::cos(2.0f * std::numbers::pi_v<float> * i / topo.size()),
            centerY + radius * std::sin(2.0f * std::numbers::pi_v<float> * i / topo.size())
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
        if (packet.from_node < 0 || packet.to_node < 0 ||
            packet.from_node >= (int)positions.size() ||
            packet.to_node >= (int)positions.size()) {
            continue;
        }

        double denom = (packet.arrival_time - packet.departure_time);
        if (denom <= 0.0) {
            continue;
        }

        double t = (engine.now() - packet.departure_time) / denom;
        t = std::clamp(t, 0.0, 1.0);

        float x = positions[packet.from_node].first +
            (positions[packet.to_node].first - positions[packet.from_node].first) * static_cast<float>(t);

        float y = positions[packet.from_node].second +
            (positions[packet.to_node].second - positions[packet.from_node].second) * static_cast<float>(t);

        draw_list->AddCircleFilled(
            ImVec2(x, y),
            6.0f,
            IM_COL32(200, 100, 200, 255)
        );
    }
}

void renderSelectedNodePanel(
    int selected_node,
    const std::vector<Routing::RoutingEntry>& routingTable
) {
    ImGui::Begin("Node Details");

    if (selected_node < 0) {
        ImGui::Text("Nenhum nó selecionado.");
        ImGui::End();
        return;
    }

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

static void SetupDockingLayout()
{
    ImGuiID dockspace_id = ImGui::GetID("MainDockSpace");

    ImGui::DockBuilderRemoveNode(dockspace_id);
    ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
    ImGui::DockBuilderSetNodeSize(dockspace_id, ImGui::GetMainViewport()->Size);

    ImGuiID dock_main = dockspace_id;
    ImGuiID dock_left = 0, dock_right = 0, dock_bottom = 0;

    ImGui::DockBuilderSplitNode(dock_main, ImGuiDir_Left, 0.25f, &dock_left, &dock_main);
    ImGui::DockBuilderSplitNode(dock_main, ImGuiDir_Right, 0.28f, &dock_right, &dock_main);
    ImGui::DockBuilderSplitNode(dock_main, ImGuiDir_Down, 0.30f, &dock_bottom, &dock_main);

    ImGui::DockBuilderDockWindow("Stats", dock_left);
    ImGui::DockBuilderDockWindow("Configurações", dock_right);
    ImGui::DockBuilderDockWindow("Node Details", dock_right);
    ImGui::DockBuilderDockWindow("Rede", dock_main);

    ImGui::DockBuilderFinish(dockspace_id);
}

static void BeginDockSpaceHost(bool& dock_initialized)
{
    ImGuiWindowFlags dockspace_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;

    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);

    dockspace_flags |= ImGuiWindowFlags_NoTitleBar
        | ImGuiWindowFlags_NoCollapse
        | ImGuiWindowFlags_NoResize
        | ImGuiWindowFlags_NoMove
        | ImGuiWindowFlags_NoBringToFrontOnFocus
        | ImGuiWindowFlags_NoNavFocus;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::Begin("DockSpace Host", nullptr, dockspace_flags);
    ImGui::PopStyleVar(2);

    ImGuiID dockspace_id = ImGui::GetID("MainDockSpace");
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);

    if (!dock_initialized) {
        SetupDockingLayout();
        dock_initialized = true;
    }

    ImGui::End();
}

static void renderNetworkPanel(
    const Topology& topo,
    const std::vector<std::pair<float, float>>& positions,
    int selected_node,
    SimulationEngine& engine
) {
    ImGui::Begin("Rede");

    ImVec2 canvas_p0 = ImGui::GetCursorScreenPos();
    ImVec2 canvas_sz = ImGui::GetContentRegionAvail();

    if (canvas_sz.x < 50.0f) canvas_sz.x = 50.0f;
    if (canvas_sz.y < 50.0f) canvas_sz.y = 50.0f;

    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    draw_list->AddRectFilled(
        canvas_p0,
        ImVec2(canvas_p0.x + canvas_sz.x, canvas_p0.y + canvas_sz.y),
        IM_COL32(20, 20, 20, 255)
    );

    ImGui::InvisibleButton("network_canvas", canvas_sz);

    if (topo.size() > 0) {
        drawLinks(draw_list, topo, positions);
        drawNodes(draw_list, topo, positions, selected_node);
        drawPackets(draw_list, positions, engine);
    }

    ImGui::End();
}

// ------------------------------------------------------------
// Main Visualization Loop
// ------------------------------------------------------------

void visualizeWindow(
    SimulationEngine& engine,
    Topology& topo,
    std::vector<std::pair<float, float>>& positions,
    SimulationState& state,
    GLFWwindow* window,
    CircularBuffer& buffer,
    int& packetSize
) {
    int selected_node = -1;
    std::vector<Routing::RoutingEntry> routingTable;
    Routing routing;
    static bool firstFrame = true;
    static bool dock_initialized = false;

    while (!glfwWindowShouldClose(window)) {
        if (state == SimulationState::Running && engine.hasEvents()) {
            engine.processEvent();
        }

        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        BeginDockSpaceHost(dock_initialized);

        if (firstFrame && topo.size() == 0) {
            ImGuiFileDialog::Instance()->OpenDialog("TopologyKey", "Select Initial Topology", ".json");
            firstFrame = false;
        }

        generateWindow(engine, state, engine.getStats(), buffer, packetSize);

        renderNetworkPanel(topo, positions, selected_node, engine);

        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            selected_node = pickNodeAtMouse(positions, 10.0f);
            if (selected_node != -1) {
                routingTable = routing.buildRoutingTable(topo, selected_node);
            } else {
                routingTable.clear();
            }
        }

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
                    engine.setLatencyObserver([&buffer](double lat) {
                        buffer.addLatencyToBuffer(static_cast<float>(lat));
                    });

                    positions = generatePositions(topo);
                    generatePackets(engine, topo);

                    state = SimulationState::Paused;
                    selected_node = -1;
                    routingTable.clear();
                } catch (const std::exception& e) {
                    std::cerr << "Load error: " << e.what() << std::endl;
                }
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
    if (argc < 2) {
        return -1;
    }

    Topology topo;

    try {
        topo = TopologyLoader::load_topology(argv[1]);
    } catch (const std::exception& e) {
        return -1;
    }

    SimulationState state = SimulationState::Paused;

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

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    visualizeWindow(engine, topo, positions, state, window, buffer, packetSize);

    shutdownWindow(window);
    return 0;
}