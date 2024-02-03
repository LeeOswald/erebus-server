#pragma once

#include <erebus-srv/userdb.hxx>

#include <mutex>
#include <unordered_map>

namespace Er
{

namespace Private
{


class UserDb final
    : public Server::IUserDb
    , public boost::noncopyable
{
public:
    ~UserDb();
    explicit UserDb(const std::string& path);

    void add(const Server::User& u) override;
    void remove(const std::string& name) override;
    std::vector<Server::User> enumerate() const override;
    std::optional<Server::User> lookup(const std::string& name) const override;
    void save() override;

private:
    std::string m_path;
    mutable std::mutex m_mutex;
    std::unordered_map<std::string, Server::User> m_users;
    bool m_dirty = false;
};


} // namespace Private {}

} // namespace Er {}