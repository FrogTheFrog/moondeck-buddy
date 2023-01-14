// header file include
#include "registryfileparser.h"

// system/Qt includes
#include <QFile>
#include <QStringBuilder>

// local includes
#include "shared/loggingcategories.h"

//---------------------------------------------------------------------------------------------------------------------

namespace
{
bool isWhitespace(char byte)
{
    return byte == '\n' || byte == ' ' || byte == '\t' || byte == '\r';
}

//---------------------------------------------------------------------------------------------------------------------

bool isQuote(char byte)
{
    return byte == '"';
}

//---------------------------------------------------------------------------------------------------------------------

bool isGroupStart(char byte)
{
    return byte == '{';
}

//---------------------------------------------------------------------------------------------------------------------

bool isGroupEnd(char byte)
{
    return byte == '}';
}

//---------------------------------------------------------------------------------------------------------------------

bool isEscape(char byte)
{
    return byte == '\\';
}

//---------------------------------------------------------------------------------------------------------------------

bool isEscapable(char byte)
{
    return byte == '\n' || byte == '\t' || byte == '\r' || byte == '\\' || byte == '\"';
}

//---------------------------------------------------------------------------------------------------------------------

using Node = os::RegistryFileParser::Node;
QString toString(qsizetype indent_level, const Node& node);
QString toString(qsizetype indent_level, const Node::List& node_list);

//---------------------------------------------------------------------------------------------------------------------

QString indent(qsizetype level)
{
    return QString{level * 2, ' '};
}

//---------------------------------------------------------------------------------------------------------------------

// NOLINTNEXTLINE(*-no-recursion)
QString toString(qsizetype indent_level, const Node& node)
{
    QString result{indent(indent_level) % QStringLiteral("\"") % node.m_key % QStringLiteral("\"")};
    if (const auto* value = std::get_if<Node::List>(&node.m_value); value)
    {
        result += " {\n" % toString(indent_level + 1, *value) % indent(indent_level) % "}\n";
    }
    else if (const auto* value = std::get_if<QString>(&node.m_value); value)
    {
        result += " \"" % *value % "\"\n";
    }
    else if (const auto* value = std::get_if<qint64>(&node.m_value); value)
    {
        result += " " % QString::number(*value) % "\n";
    }
    else
    {
        assert(false);
    }
    return result;
}

//---------------------------------------------------------------------------------------------------------------------

// NOLINTNEXTLINE(*-no-recursion)
QString toString(qsizetype indent_level, const Node::List& node_list)
{
    QStringList entries;
    for (const auto& node : node_list)
    {
        entries.append(toString(indent_level, *node));
    }
    return entries.join("");
}
}  // namespace

//---------------------------------------------------------------------------------------------------------------------

namespace os
{
const Node::List& RegistryFileParser::getRoot() const
{
    return m_root;
}

//---------------------------------------------------------------------------------------------------------------------

bool RegistryFileParser::parse(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
    {
        qCWarning(lc::os) << "registry file" << path << "does not exist!";
        return false;
    }

    const qint64 file_buffer_size{4096};
    const qint64 value_buffer_size{512};
    const auto   cleanup{qScopeGuard([this]() { m_data = {}; })};

    m_root.clear();
    m_data.m_parents.push_back(&m_root);
    m_data.m_buffer.reserve(value_buffer_size);

    while (!file.atEnd())
    {
        QByteArray file_buffer{file.read(file_buffer_size)};
        for (const auto byte : file_buffer)
        {
            if (!processByte(byte))
            {
                return false;
            }
        }
    }

    //qCDebug(lc::os).noquote().nospace() << "Registry file:\n\n" << toString(0, m_root);
    return true;
}

//---------------------------------------------------------------------------------------------------------------------

bool RegistryFileParser::processByte(const char byte)
{
    if (m_data.m_next_byte_escaped)
    {
        if (!isEscapable(byte))
        {
            qCWarning(lc::os) << "expected escaped char, got" << byte << "instead!";
            return false;
        }

        m_data.m_next_byte_escaped = false;
        m_data.m_buffer.push_back(byte);
        return true;
    }

    if (isEscape(byte))
    {
        m_data.m_next_byte_escaped = true;
        return true;
    }

    return m_data.m_is_quoted ? processInQuotedMode(byte) : processInUnquotedMode(byte);
}

//---------------------------------------------------------------------------------------------------------------------

bool RegistryFileParser::processInQuotedMode(const char byte)
{
    if (isQuote(byte))
    {
        m_data.m_is_quoted = false;
        return saveBufferAndFlipSearch();
    }

    m_data.m_buffer.push_back(byte);
    return true;
}

//---------------------------------------------------------------------------------------------------------------------

bool RegistryFileParser::processInUnquotedMode(const char byte)
{
    const auto save_buffer_if_needed = [this]() { return m_data.m_buffer.isEmpty() || saveBufferAndFlipSearch(); };
    const bool is_quote{isQuote(byte)};
    if (isWhitespace(byte) || is_quote)
    {
        m_data.m_is_quoted = is_quote;
        return save_buffer_if_needed();
    }

    if (isGroupStart(byte))
    {
        return save_buffer_if_needed() && enterGroup();
    }

    if (isGroupEnd(byte))
    {
        return save_buffer_if_needed() && leaveGroup();
    }

    m_data.m_buffer.push_back(byte);
    return true;
}

//---------------------------------------------------------------------------------------------------------------------

bool RegistryFileParser::enterGroup()
{
    if (m_data.m_expecting_key_value)
    {
        qCWarning(lc::os) << "trying to enter group, but we are searching for a key!";
        return false;
    }

    if (m_data.m_parents.empty())
    {
        qCWarning(lc::os) << "trying to enter group, but no parents are available!";
        return false;
    }

    auto& current_parent{m_data.m_parents.back()};
    if (current_parent->empty())
    {
        qCWarning(lc::os) << "trying to enter group, but no nodes are available!";
        return false;
    }

    auto& last_node{current_parent->back()};
    last_node->m_value = Node::List{};

    m_data.m_parents.push_back(&std::get<Node::List>(last_node->m_value));
    m_data.m_expecting_key_value = true;
    return true;
}

//---------------------------------------------------------------------------------------------------------------------

bool RegistryFileParser::leaveGroup()
{
    if (!m_data.m_expecting_key_value)
    {
        qCWarning(lc::os) << "trying to leave group, but we are searching for a value!";
        return false;
    }

    if (m_data.m_parents.empty())
    {
        qCWarning(lc::os) << "trying to leave group, but no parents are available!";
        return false;
    }

    m_data.m_parents.pop_back();
    return true;
}

//---------------------------------------------------------------------------------------------------------------------

bool RegistryFileParser::saveBuffer()
{
    if (m_data.m_parents.empty())
    {
        qCWarning(lc::os) << "trying to save buffer, but no parents are available!";
        return false;
    }

    auto& current_parent{m_data.m_parents.back()};
    if (m_data.m_expecting_key_value)
    {
        current_parent->push_back(std::make_unique<Node>(Node{m_data.m_buffer, QString{}}));
    }
    else
    {
        if (current_parent->empty())
        {
            qCWarning(lc::os) << "trying to save buffer, but no nodes are available!";
            return false;
        }

        auto&         last_child_node{current_parent->back()};
        const QString value_maybe_number{m_data.m_buffer};

        bool         converted{false};
        const qint64 number{value_maybe_number.toInt(&converted)};
        if (converted)
        {
            last_child_node->m_value = number;
        }
        else
        {
            last_child_node->m_value = value_maybe_number;
        }
    }

    m_data.m_buffer.clear();
    return true;
}

//---------------------------------------------------------------------------------------------------------------------

bool RegistryFileParser::saveBufferAndFlipSearch()
{
    const bool result            = saveBuffer();
    m_data.m_expecting_key_value = !m_data.m_expecting_key_value;
    return result;
}
}  // namespace os
