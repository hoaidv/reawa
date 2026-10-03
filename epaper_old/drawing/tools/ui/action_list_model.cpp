#include "action_list_model.hpp"

#include "../actions/action.hpp"
#include "../host_caps.hpp"

namespace epaper {
namespace tools {

ActionListModel::ActionListModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int ActionListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return int(m_rows.size());
}

QVariant ActionListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= int(m_rows.size()))
        return {};
    const Row &r = m_rows[size_t(index.row())];
    switch (role) {
    case ActionIdRole:
        return r.id;
    case IconRole:
        return r.icon;
    case LabelRole:
        return r.label;
    case EnabledRole:
        return r.enabled;
    default:
        return {};
    }
}

QHash<int, QByteArray> ActionListModel::roleNames() const
{
    return {
        {ActionIdRole, "actionId"},
        {IconRole, "icon"},
        {LabelRole, "label"},
        {EnabledRole, "enabled"},
    };
}

void ActionListModel::rebuild(const std::vector<ToolAction *> &actions, const HostCaps &caps)
{
    beginResetModel();
    m_rows.clear();
    for (ToolAction *a : actions) {
        if (!a || !a->visible(caps))
            continue;
        Row r;
        r.id = a->id();
        r.icon = a->icon();
        r.label = a->label(caps);
        r.enabled = a->enabled(caps);
        m_rows.push_back(std::move(r));
    }
    endResetModel();
    emit countChanged();
}

} // namespace tools
} // namespace epaper
