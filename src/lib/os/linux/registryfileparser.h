#pragma once

// system/Qt includes
#include <QObject>

//---------------------------------------------------------------------------------------------------------------------

namespace os
{
// Note: this is a simplified parser - does not support macros, comments and other stuff
class RegistryFileParser : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(RegistryFileParser)

public:
    struct Node
    {
        using List = std::vector<std::unique_ptr<Node>>;

        QString                             m_key{};
        std::variant<List, QString, qint64> m_value{QString{}};
    };

    explicit RegistryFileParser()  = default;
    ~RegistryFileParser() override = default;

    const Node::List& getRoot() const;
    bool              parse(const QString& path);

private:
    struct ParsingData
    {
        std::vector<Node::List*> m_parents{};
        QByteArray               m_buffer{};
        bool                     m_expecting_key_value{true};
        bool                     m_is_quoted{false};
        bool                     m_next_byte_escaped{false};
    };

    bool processByte(const char byte);
    bool processInQuotedMode(const char byte);
    bool processInUnquotedMode(const char byte);
    bool enterGroup();
    bool leaveGroup();
    bool saveBuffer();
    bool saveBufferAndFlipSearch();

    ParsingData m_data;
    Node::List  m_root;
};
}  // namespace os
