#pragma once

#include <erebus-srv/erebus-srv.hxx>

#include <vector>


namespace Er
{

namespace Private
{

namespace Server
{

struct User
{
    std::string name;
    std::string pwdSalt;
    std::string pwdHash;

    explicit User(std::string_view name, std::string_view pwdSalt, std::string_view pwdHash)
        : name(name)
        , pwdSalt(pwdSalt)
        , pwdHash(pwdHash)
    {}
};


struct IUserDb
{
    virtual void add(const User& u) = 0;
    virtual void remove(const std::string& name) = 0;
    virtual std::vector<User> enumerate() const = 0;
    virtual std::optional<User> lookup(const std::string& name) const = 0;
    virtual void save() = 0;

protected:
    virtual ~IUserDb() {}
};


} // namespace Server {}

} // namespace Private {}

} // namespace Er {}