#pragma once

#include <erebus/log.hxx>
#include <erebus-srv/erebus-srv.hxx>

#include <grpcpp/grpcpp.h>

#include <chrono>
#include <shared_mutex>
#include <unordered_map>
#include <vector>

namespace Er
{

namespace Server
{

namespace Private
{

class AuthMetadataProcessor
    : public grpc::AuthMetadataProcessor
{
public:
    ~AuthMetadataProcessor()
    {
        ErLogDebug(m_log, ErLogInstance("AuthMetadataProcessor"), "~AuthMetadataProcessor()");
    }

    explicit AuthMetadataProcessor(Er::Log::ILog* log)
        : m_log(log)
    {
        ErLogDebug(m_log, ErLogInstance("AuthMetadataProcessor"), "AuthMetadataProcessor()");
    }

    grpc::Status Process(const InputMetadata& authMetadata, grpc::AuthContext* context, OutputMetadata* consumedMetadata, OutputMetadata* responseMetadata) override
    {
        std::shared_lock l(m_mutex);

        // determine intercepted method
        auto dispatch = authMetadata.find(":path");
        if (dispatch == authMetadata.end())
        {
            ErLogError(m_log, ErLogInstance("AuthMetadataProcessor"), "No method path in metadata");
            return grpc::Status(grpc::StatusCode::INTERNAL, "Internal Error");
        }

        // if token metadata not necessary, return early, avoid token checking
        auto dispatchValue = std::string(dispatch->second.data(), dispatch->second.length());
        if (std::find(m_noAuthMethods.begin(), m_noAuthMethods.end(), dispatchValue) != m_noAuthMethods.end())
        {
            ErLogDebug(m_log, ErLogInstance("AuthMetadataProcessor"), "No auth required for [%s]", dispatchValue.c_str());
            return grpc::Status::OK;
        }

        // determine availability of ticket metadata
        auto ticket = authMetadata.find("ticket");
        if (ticket == authMetadata.end())
        {
            ErLogError(m_log, ErLogInstance("AuthMetadataProcessor"), "No ticket in metadata");
            return grpc::Status(grpc::StatusCode::UNAUTHENTICATED, "Missing Ticket");
        }

        // determine validity of token metadata
        auto ticketValue = std::string(ticket->second.data(), ticket->second.length());
        auto it = m_tickets.find(ticketValue);
        if (it == m_tickets.end())
        {
            ErLogError(m_log, ErLogInstance("AuthMetadataProcessor"), "Invalid ticket");
            return grpc::Status(grpc::StatusCode::UNAUTHENTICATED, "Invalid Ticket");
        }

        it->second.touched = std::chrono::steady_clock::now();

        ErLogDebug(m_log, ErLogInstance("AuthMetadataProcessor"), "Found ticket [%s] -> [%s]", ticketValue.c_str(), it->second.user.c_str());

        // once verified, mark as consumed and store user for later retrieval
        consumedMetadata->insert(std::make_pair("ticket", ticketValue));     // required
        context->AddProperty("ticket", ticketValue);
        context->AddProperty("user", it->second.user);           // optional
        context->SetPeerIdentityPropertyName("user");                 // optional

        return grpc::Status::OK;
    }

    void addTicket(const std::string& user, const std::string& ticket)
    {
        ErLogDebug(m_log, ErLogInstance("AuthMetadataProcessor"), "Added ticket [%s] -> [%s]", ticket.c_str(), user.c_str());

        std::unique_lock l(m_mutex);

        dropStaleTickets();
        m_tickets.insert({ ticket, Ticket(ticket, user) });
    }

    void removeTicket(const std::string& ticket)
    {
        std::unique_lock l(m_mutex);
        
        auto it = m_tickets.find(ticket);
        if (it == m_tickets.end())
        {
            ErLogWarning(m_log, ErLogInstance("AuthMetadataProcessor"), "Ticket [%s] not found", ticket.c_str());
        }
        else
        {
            m_tickets.erase(it);
            ErLogDebug(m_log, ErLogInstance("AuthMetadataProcessor"), "Ticket [%s] removed", ticket.c_str());
        }
    }

    void addNoAuthMethod(std::string_view path)
    {
        std::unique_lock l(m_mutex);
        m_noAuthMethods.emplace_back(path);
    }

private:
    struct Ticket
    {
        std::string ticket;
        std::string user;
        std::chrono::steady_clock::time_point touched = std::chrono::steady_clock::now();

        explicit Ticket(const std::string& ticket, const std::string& user)
            : ticket(ticket)
            , user(user)
        {}
    };

    static constexpr unsigned kTicketDurationSeconds = 1;//60 * 60; // 1 hr

    void dropStaleTickets()
    {
        auto now = std::chrono::steady_clock::now();

        for (auto it = m_tickets.begin(); it != m_tickets.end();)
        {
            auto d = std::chrono::duration_cast<std::chrono::seconds>(now - it->second.touched);
            if (d.count() > kTicketDurationSeconds)
            {
                auto next = std::next(it);
                ErLogDebug(m_log, ErLogInstance("AuthMetadataProcessor"), "Stale ticket [%s] dropped", it->second.ticket.c_str());
                m_tickets.erase(it);
                it = next;
            }
            else
            {
                ++it;
            }
        }
    }

    Er::Log::ILog* m_log;
    std::shared_mutex m_mutex;
    std::vector<std::string> m_noAuthMethods;
    std::unordered_map<std::string, Ticket> m_tickets; // ticket -> user
};

} // namespace Private {}

} // namespace Server {}

} // namespace Er {}
