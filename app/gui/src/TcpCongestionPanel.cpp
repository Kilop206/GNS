#include "../include/TcpCongestionPanel.hpp"

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

#include "../../../core/include/network/transport/tcp/TCPConnection.hpp"
#include "../../../core/include/network/transport/tcp/TCPSession.hpp"
#include "../../../core/include/network/transport/tcp/congestion/CongestionControlType.hpp"
#include "../../../core/include/network/transport/tcp/congestion/CongestionControl.hpp"
#include "../../../core/include/network/transport/tcp/congestion/RenoCongestionControl.hpp"
#include "../../../core/include/network/transport/tcp/congestion/NewRenoCongestionControl.hpp"
#include "../../../core/include/network/transport/tcp/congestion/TahoeCongestionControl.hpp"
#include "../../../core/include/network/transport/tcp/congestion/CubicCongestionControl.hpp"

namespace gui {

    namespace {

        const char* congestionControlTypeToString(
            kns::CongestionControlType type
        ) noexcept
        {
            switch (type) {
                case kns::CongestionControlType::TAHOE:
                    return "Tahoe";

                case kns::CongestionControlType::RENO:
                    return "Reno";

                case kns::CongestionControlType::NEW_RENO:
                    return "NewReno";

                case kns::CongestionControlType::CUBIC:
                    return "CUBIC";
            }

            return "Unknown";
        }

        bool congestionControlInFastRecovery(
            const kns::CongestionControl& control
        ) noexcept
        {
            if (const auto* reno =
                dynamic_cast<const kns::RenoCongestionControl*>(
                    &control
                ))
            {
                return reno->inFastRecovery();
            }

            if (const auto* new_reno =
                dynamic_cast<const kns::NewRenoCongestionControl*>(
                    &control
                ))
            {
                return new_reno->inFastRecovery();
            }

            if (const auto* cubic =
                dynamic_cast<const kns::CubicCongestionControl*>(
                    &control
                ))
            {
                return cubic->inFastRecovery();
            }

            return false;
        }

        void renderCongestionChart(
            const kns::TCPConnection& connection
        )
        {
            const auto& history =
                connection.getCongestionHistory();

            if (history.empty()) {
                ImGui::TextDisabled(
                    "No congestion history available."
                );

                return;
            }

            std::vector<float> cwnd_values;
            std::vector<float> ssthresh_values;

            cwnd_values.reserve(history.size());
            ssthresh_values.reserve(history.size());

            for (const auto& sample : history) {
                cwnd_values.push_back(
                    static_cast<float>(sample.cwnd)
                );

                ssthresh_values.push_back(
                    static_cast<float>(sample.ssthresh)
                );
            }

            const auto max_cwnd =
                *std::max_element(
                    cwnd_values.begin(),
                    cwnd_values.end()
                );

            const auto max_ssthresh =
                *std::max_element(
                    ssthresh_values.begin(),
                    ssthresh_values.end()
                );

            const float max_value =
                std::max(
                    max_cwnd,
                    max_ssthresh
                );

            const float chart_max =
                max_value > 0.0f
                    ? max_value * 1.10f
                    : 1.0f;

            ImGui::Separator();

            ImGui::Text(
                "Congestion Window History"
            );

            ImGui::PlotLines(
                "cwnd",
                cwnd_values.data(),
                static_cast<int>(cwnd_values.size()),
                0,
                nullptr,
                0.0f,
                chart_max,
                ImVec2(
                    -1.0f,
                    180.0f
                )
            );

            ImGui::PlotLines(
                "ssthresh",
                ssthresh_values.data(),
                static_cast<int>(ssthresh_values.size()),
                0,
                nullptr,
                0.0f,
                chart_max,
                ImVec2(
                    -1.0f,
                    180.0f
                )
            );

            const auto& first =
                history.front();

            const auto& last =
                history.back();

            ImGui::Text(
                "Samples: %u",
                static_cast<unsigned>(
                    history.size()
                )
            );

            ImGui::Text(
                "First sample: t=%.3f s",
                first.timestamp
            );

            ImGui::Text(
                "Last sample: t=%.3f s",
                last.timestamp
            );
        }

    }

    void TcpCongestionPanel::render(
        const kns::SimulationEngine& engine
    )
    {
        ImGui::Begin(
            "TCP Congestion Control"
        );

        const auto& sessions =
            engine.getTCPSessions();

        if (sessions.empty()) {
            ImGui::TextDisabled(
                "No TCP sessions available."
            );

            ImGui::End();
            return;
        }

        static std::uint64_t selected_session_id = 0;

        if (!sessions.contains(
            selected_session_id
        )) {
            selected_session_id =
                sessions.begin()->first;
        }

        const auto selected =
            sessions.find(
                selected_session_id
            );

        if (selected == sessions.end()) {
            ImGui::TextDisabled(
                "Selected TCP session is unavailable."
            );

            ImGui::End();
            return;
        }

        const std::uint64_t session_id =
            selected->first;

        ImGui::Text(
            "Session: %llu",
            static_cast<unsigned long long>(
                session_id
            )
        );

        ImGui::SameLine();

        if (ImGui::BeginCombo(
            "##TCPCongestionSession",
            std::to_string(
                session_id
            ).c_str()
        ))
        {
            for (const auto& [id, session] : sessions)
            {
                const bool is_selected =
                    id == selected_session_id;

                const bool selectable =
                    ImGui::Selectable(
                        std::to_string(id).c_str(),
                        is_selected
                    );

                if (selectable) {
                    selected_session_id = id;
                }

                if (is_selected) {
                    ImGui::SetItemDefaultFocus();
                }

                static_cast<void>(session);
            }

            ImGui::EndCombo();
        }

        const auto& session =
            selected->second;

        const auto& connection =
            session.getClientConnection();

        const auto& congestion =
            connection.getCongestionControl();

        ImGui::Separator();

        ImGui::Text(
            "Algorithm: %s",
            congestionControlTypeToString(
                connection.getCongestionControlType()
            )
        );

        ImGui::Text(
            "MSS: %u bytes",
            congestion.getMss()
        );

        ImGui::Text(
            "cwnd: %u bytes",
            congestion.getCwnd()
        );

        ImGui::Text(
            "ssthresh: %u bytes",
            congestion.getSsthresh()
        );

        ImGui::Text(
            "Send Unacknowledged: %u",
            connection.getSendUnacknowledged()
        );

        ImGui::Text(
            "Send Next: %u",
            connection.getSendNext()
        );

        const std::uint32_t bytes_in_flight =
            connection.getSendNext() -
            connection.getSendUnacknowledged();

        ImGui::Text(
            "Bytes in flight: %u",
            bytes_in_flight
        );

        ImGui::Text(
            "Duplicate ACKs: %u",
            connection.getDuplicateAckCount()
        );

        ImGui::Text(
            "State: %s",
            congestionControlInFastRecovery(
                congestion
            )
                ? "Fast Recovery"
                : "Normal"
        );

        renderCongestionChart(
            connection
        );

        ImGui::End();
    }

}