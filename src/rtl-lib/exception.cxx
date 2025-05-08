#include <erebus/rtl/exception.hxx>

#include <sstream>


namespace Er
{

std::string const& Exception::message() const noexcept
{
    if (m_message.empty())
    {
        decode();

        const std::string* brief = nullptr;
        const std::string* message = nullptr;

        auto brief_ = findProperty(m_properties, ExceptionProperties::Brief::Name, Property::Type::String);
        if (brief_)
            brief = brief_->getString();

        auto message_ = findProperty(m_properties, ExceptionProperties::Message::Name, Property::Type::String);
        if (message_)
            message = message_->getString();

        std::ostringstream ss;

        if (brief)
        {
            ss << *brief << ": ";
        }

        ErAssert(m_category);
        ss << "[" << m_category->name() << " " << m_code << "]";

        if (message)
        {
            ss << " " << *message;
        }

        m_message = ss.str();
    }

    return m_message;
}

std::string const* Exception::decode() const
{
    auto msg = findProperty(m_properties, ExceptionProperties::Message::Name, Property::Type::String);
    if (msg)
        return msg->getString();

    if (m_code != Success)
    {
        ErAssert(m_category);
        auto message = m_category->message(m_code);
        if (!message.empty())
        {
            m_properties.push_back(ExceptionProperties::Message(std::move(message)));
            return m_properties.back().getString();
        }
    }

    return nullptr;
}


} // namespace Er {}