#pragma once

/**
 * QAbstractListModel over visible ToolAction rows for SelectionContextToolbar.
 * @implements [SRS-EP-12]
 */

#include <QAbstractListModel>
#include <QString>
#include <vector>

namespace epaper {
namespace tools {

class ToolAction;
struct HostCaps;

class ActionListModel final : public QAbstractListModel {
    Q_OBJECT
public:
    enum Roles {
        ActionIdRole = Qt::UserRole + 1,
        IconRole,
        LabelRole,
        EnabledRole,
    };

    Q_PROPERTY(int count READ count NOTIFY countChanged)

    explicit ActionListModel(QObject *parent = nullptr);

    int count() const { return int(m_rows.size()); }
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    void rebuild(const std::vector<ToolAction *> &actions, const HostCaps &caps);

signals:
    void countChanged();

private:
    struct Row {
        QString id;
        QString icon;
        QString label;
        bool enabled = false;
    };
    std::vector<Row> m_rows;
};

} // namespace tools
} // namespace epaper
