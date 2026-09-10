#include "../include/TcpCongestionPanel.hpp"

#include <cstdint>
#include <string>

#include "../../../core/include/network/transport/tcp/TCPConnection.hpp"
#include "../../../core/include/network/transport/tcp/TCPSession.hpp"
#include "../../../core/include/network/transport/tcp/congestion/CongestionControlType.hpp"
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

    }

    void TcpCongestionPanel::render(
        const kns::SimulationEngine& engine
    )
    {
        ImGui::Begin("TCP Congestion Control");

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

        if (!sessions.contains(selected_session_id)) {
            selected_session_id =
                sessions.begin()->first;
        }

        const auto selected =
            sessions.find(selected_session_id);

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
            std::to_string(session_id).c_str()
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
            ) ? "Fast Recovery" : "Normal"
        );

        ImGui::End();
    }

}