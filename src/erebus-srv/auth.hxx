#pragma once

#include <erebus/log.hxx>
#include <erebus-srv/erebus-srv.hxx>

#include <grpcpp/grpcpp.h>

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
        LogDebug(m_log, LogInstance("AuthMetadataProcessor"), "~AuthMetadataProcessor()");
    }

    explicit AuthMetadataProcessor(Er::Log::ILog* log)
        : m_log(log)
    {
        LogDebug(m_log, LogInstance("AuthMetadataProcessor"), "AuthMetadataProcessor()");
    }

    grpc::Status Process(const InputMetadata& authMetadata, grpc::AuthContext* context, OutputMetadata* consumedMetadata, OutputMetadata* responseMetadata) override
    {
        std::shared_lock l(m_mutex);

        // determine intercepted method
        auto dispatch = authMetadata.find(":path");
        if (dispatch == authMetadata.end())
        {
            LogError(m_log, LogInstance("AuthMetadataProcessor"), "No method path in metadata");
            return grpc::Status(grpc::StatusCode::INTERNAL, "Internal Error");
        }

        // if token metadata not necessary, return early, avoid token checking
        auto dispatchValue = std::string(dispatch->second.data(), dispatch->second.length());
        if (std::find(m_noAuthMethods.begin(), m_noAuthMethods.end(), dispatchValue) != m_noAuthMethods.end())
        {
            LogDebug(m_log, LogInstance("AuthMetadataProcessor"), "No auth required for [%s]", dispatchValue.c_str());
            return grpc::Status::OK;
        }

        // determine availability of ticket metadata
        auto ticket = authMetadata.find("ticket");
        if (ticket == authMetadata.end())
        {
            LogError(m_log, LogInstance("AuthMetadataProcessor"), "No ticket in metadata");
            return grpc::Status(grpc::StatusCode::UNAUTHENTICATED, "Missing Ticket");
        }

        // determine validity of token metadata
        auto ticketValue = std::string(ticket->second.data(), ticket->second.length());
        auto it = m_tickets.find(ticketValue);
        if (it == m_tickets.end())
        {
            LogError(m_log, LogInstance("AuthMetadataProcessor"), "Invalid ticket");
            return grpc::Status(grpc::StatusCode::UNAUTHENTICATED, "Invalid Ticket");
        }

        LogDebug(m_log, LogInstance("AuthMetadataProcessor"), "Found ticket [%s] -> [%s]", ticketValue.c_str(), it->second.c_str());

        // once verified, mark as consumed and store user for later retrieval
        consumedMetadata->insert(std::make_pair("ticket", ticketValue));     // required
        context->AddProperty("ticket", ticketValue);
        context->AddProperty("user", it->second);           // optional
        context->SetPeerIdentityPropertyName("user");                 // optional

        return grpc::Status::OK;
    }

    void addTicket(const std::string& user, const std::string& ticket)
    {
        LogDebug(m_log, LogInstance("AuthMetadataProcessor"), "Added ticket [%s] -> [%s]", ticket.c_str(), user.c_str());

        std::lock_guard l(m_mutex);
        m_tickets.insert({ ticket, user });
    }

    void removeTicket(const std::string& ticket)
    {
        std::lock_guard l(m_mutex);
        auto it = m_tickets.find(ticket);
        if (it == m_tickets.end())
        {
            LogWarning(m_log, LogInstance("AuthMetadataProcessor"), "Ticket [%s] not found", ticket.c_str());
        }
        else
        {
            m_tickets.erase(it);
            LogDebug(m_log, LogInstance("AuthMetadataProcessor"), "Ticket [%s] removed", ticket.c_str());
        }
    }

    void addNoAuthMethod(std::string_view path)
    {
        std::lock_guard l(m_mutex);
        m_noAuthMethods.emplace_back(path);
    }

private:
    Er::Log::ILog* m_log;
    std::shared_mutex m_mutex;
    std::vector<std::string> m_noAuthMethods;
    std::unordered_map<std::string, std::string> m_tickets; // ticket -> user
};

} // namespace Private {}

} // namespace Server {}

} // namespace Er {}
