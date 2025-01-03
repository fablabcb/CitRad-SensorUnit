#ifndef LOGMODEL_HPP
#define LOGMODEL_HPP

#include <QAbstractListModel>
#include <QString>
#include <QTime>
#include <vector>

class LogModel : public QAbstractListModel
{
    Q_OBJECT

  public:
    enum Roles
    {
        TypeRole = Qt::UserRole + 1,
        TextRole,
        TimeRole
    };
    enum Type
    {
        Inbound = 0,
        Outbound = 1,
        Status = 2
    };

    LogModel(size_t maxSize, QObject* parent = nullptr);

  public:
    QHash<int, QByteArray> roleNames() const override;
    int rowCount(QModelIndex const&) const override;
    QVariant data(QModelIndex const& index, int role) const override;

    void add(Type type, std::string const& text);

  private:
    struct Item
    {
        Type type = Type::Inbound;
        QString text;
        QTime time;

        Item(Type type, QString const& text, QTime const& time);
    };

  private:
    size_t maxSize;
    size_t indexOffset = 0;
    std::vector<Item> items;
};

#endif // LOGMODEL_HPP
