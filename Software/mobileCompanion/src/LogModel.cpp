#include "LogModel.hpp"

LogModel::LogModel(size_t maxSize, QObject* parent)
    : QAbstractListModel(parent)
    , maxSize(maxSize)
{
    items.emplace_back(Type::Inbound, "This is an inbound message", QTime::currentTime());
    items.emplace_back(Type::Outbound, "This is an outbound message", QTime::currentTime());
    items.emplace_back(Type::Status, "This is a status message", QTime::currentTime());
}

QHash<int, QByteArray> LogModel::LogModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[TypeRole] = "type";
    roles[TextRole] = "text";
    roles[TimeRole] = "time";
    return roles;
}

int LogModel::rowCount(QModelIndex const&) const
{
    return items.size();
}

QVariant LogModel::data(QModelIndex const& index, int role) const
{
    if(index.row() < 0 || index.row() >= static_cast<int>(items.size()))
        return QVariant::Invalid;

    size_t rawRow = static_cast<size_t>(index.row());
    size_t row = (indexOffset + rawRow) % maxSize;

    switch(role)
    {
        case Roles::TypeRole:
            return items[row].type;
        case Roles::TextRole:
            return items[row].text;
        case Roles::TimeRole:
            return items[row].time.toString("hh:mm:ss:zzz");
        default:
            return QVariant::Invalid;
    }
}

void LogModel::add(Type type, std::string const& text)
{
    if(bool stillSpaceLeft = items.size() < maxSize; stillSpaceLeft)
    {
        beginInsertRows(QModelIndex(), items.size(), items.size());
        items.emplace_back(type, QString::fromStdString(text), QTime::currentTime());
        endInsertRows();
        return;
    }

    // we reached our limit
    // fake row removal
    beginRemoveRows(QModelIndex(), 0, 0);
    endRemoveRows();

    beginInsertRows(QModelIndex(), maxSize - 1, maxSize - 1);
    items[indexOffset] = Item{type, QString::fromStdString(text), QTime::currentTime()};
    indexOffset = (indexOffset + 1) % maxSize;
    endInsertRows();
}

LogModel::Item::Item(Type type, QString const& text, QTime const& time)
    : type(type)
    , text(text)
    , time(time)
{}
