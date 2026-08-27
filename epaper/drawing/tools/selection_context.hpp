#pragma once

/**
 * SelectionContext — durable selection state machine host (ids + phase).
 * Ephemeral lasso/marquee geometry lives on Operations, not here.
 * @implements [SRS-EP-11]
 */

#include <string>
#include <vector>

namespace epaper {
namespace tools {

enum class SelectionPhase {
    Idle,
    Selecting,
    Selected,
    Transforming,
};

class SelectionContext {
public:
    virtual ~SelectionContext() = default;
    virtual SelectionPhase phase() const = 0;
    virtual const std::vector<std::string> &ids() const = 0;
    virtual const std::string &pickableId() const = 0;
    virtual void clear() = 0;
    virtual void setIds(const std::vector<std::string> &ids) = 0;
    virtual void setPhase(SelectionPhase phase) = 0;
};

} // namespace tools
} // namespace epaper
