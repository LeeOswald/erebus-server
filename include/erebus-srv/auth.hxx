#pragma once

#include <erebus/log.hxx>
#include <erebus-srv/erebus-srv.hxx>

#include <grpcpp/grpcpp.h>

#include <mutex>
#include <unordered_map>
#include <vector>

namespace Er
{

namespace Server
{

class AuthMetadataProcessor
    : public grpc::AuthMetadataProcessor
{
public:
    explicit AuthMetadataProcessor(Er::Log::ILog* log)
        : m_log(log)
    {
    }

    grpc::Status Process(const InputMetadata& authMetadata, grpc::AuthContext* context, OutputMetadata* consumedMetadata, OutputMetadata* responseMetadata) override
    {
        std::lock_guard l(m_mutex);

        // determine intercepted method
        auto dispatch = authMetadata.find(":path");
        if (dispatch == authMetadata.end())
        {
            m_log->write(Er::Log::Level::Error, "No method path in metadata");
            return grpc::Status(grpc::StatusCode::INTERNAL, "Internal Error");
        }

        // if token metadata not necessary, return early, avoid token checking
        auto dispatchValue = std::string(dispatch->second.data(), dispatch->second.length());
        if (std::find(m_noAuthMethods.begin(), m_noAuthMethods.end(), dispatchValue) != m_noAuthMethods.end())
        {
            return grpc::Status::OK;
        }

        // determine availability of ticket metadata
        auto ticket = authMetadata.find("ticket");
        if (ticket == authMetadata.end())
        {
            m_log->write(Er::Log::Level::Error, "No ticket in metadata");
            return grpc::Status(grpc::StatusCode::UNAUTHENTICATED, "Missing Ticket");
        }

        // determine validity of token metadata
        auto ticketValue = std::string(ticket->second.data(), ticket->second.length());
        auto it = m_tickets.find(ticketValue);
        if (it == m_tickets.end())
        {
            m_log->write(Er::Log::Level::Error, "Invalid ticket");
            return grpc::Status(grpc::StatusCode::UNAUTHENTICATED, "Invalid Ticket");
        }

        // once verified, mark as consumed and store user for later retrieval
        consumedMetadata->insert(std::make_pair("ticket", ticketValue));     // required
        context->AddProperty("user", it->second);           // optional
        context->SetPeerIdentityPropertyName("user");                 // optional

        return grpc::Status::OK;
    }

    void addTicket(const std::string& user, const std::string& ticket)
    {
        std::lock_guard l(m_mutex);
        m_tickets.insert({ ticket, user });
    }

    void addNoAuthMethod(std::string_view path)
    {
        std::lock_guard l(m_mutex);
        m_noAuthMethods.emplace_back(path);
    }

private:
    Er::Log::ILog* m_log;
    std::mutex m_mutex;
    std::vector<std::string> m_noAuthMethods;
    std::unordered_map<std::string, std::string> m_tickets; // ticket -> user
};


} // namespace Server {}

} // namespace Er {}
