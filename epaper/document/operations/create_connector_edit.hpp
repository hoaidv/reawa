#pragma once
/**
 * @implements [SRS-EP-07] create_connector
 * @implements [SRS-EP-17] commit envelope
 */

#include "doc_edit.hpp"

namespace epaper {
namespace document {

class CreateConnectorEdit final : public DocEdit {
public:
    CreateConnectorEdit() = default;

    const char *kind() const override { return edit_kind::kCreateConnector; }
    ApplyResult doApply(DeviceDocument &doc) override;
    std::unique_ptr<DocEdit> generateUndo(const DeviceDocument &doc) const override;
    JsonValue serialize() const override;
    std::unique_ptr<DocEdit> clone() const override;
    std::vector<std::string> targets() const override { return {m_nodeId}; }

    void setNodeId(std::string id) { m_nodeId = std::move(id); }
    void setParentId(std::optional<std::string> p) { m_parentId = std::move(p); }
    void setFrom(ConnectorAnchor a) { m_from = std::move(a); }
    void setTo(ConnectorAnchor a) { m_to = std::move(a); }
    void setWarpStyle(std::string s) { m_warpStyle = std::move(s); }
    void setRestSpine(std::vector<ConnectorRestPt> s) { m_restSpine = std::move(s); }
    void setRestOffsets(std::vector<ConnectorRestOff> o) { m_restOffsets = std::move(o); }
    void setCaptureIds(std::vector<std::string> ids) { m_captureIds = std::move(ids); }
    void setFromPose(ConnectorEndPose p) { m_fromPose = p; }
    void setToPose(ConnectorEndPose p) { m_toPose = p; }
    const std::string &nodeId() const { return m_nodeId; }
    const std::vector<std::string> &captureIds() const { return m_captureIds; }

    static std::unique_ptr<CreateConnectorEdit> fromPayload(const JsonValue &envelope,
                                                            const JsonValue &payload);

private:
    std::string m_nodeId;
    std::optional<std::string> m_parentId;
    ConnectorAnchor m_from;
    ConnectorAnchor m_to;
    std::string m_warpStyle = "morph";
    std::vector<ConnectorRestPt> m_restSpine;
    std::vector<ConnectorRestOff> m_restOffsets;
    std::vector<std::string> m_captureIds;
    ConnectorEndPose m_fromPose;
    ConnectorEndPose m_toPose;
};

} // namespace document
} // namespace epaper
