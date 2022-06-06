#pragma once

#include "ILog.h"
#include "Message.h"

namespace trview
{
    class Log final : public ILog
    {
    public:
        virtual ~Log() = default;
        virtual void log(Message::Status status, const std::string& topic, const std::string& activity, const std::string& text) override;
        virtual void log(Message::Status status, const std::string& topic, const std::vector<std::string>& activity, const std::string& text) override;
        virtual std::vector<Message> messages() const override;
        virtual std::vector<Message> messages(const std::string& topic, const std::string& activity) const override;
        virtual std::vector<std::string> topics() const override;
        virtual std::vector<std::string> activities(const std::string& topic) const override;
    private:
        std::vector<Message> _messages;
    };
}
