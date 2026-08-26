#pragma once

/**
 * DocContext — document model access + mutate-then-notify helpers.
 * @implements [SRS-EP-07]
 */

#include "document/device_document.hpp"

namespace epaper {
namespace tools {

class DocContext {
public:
    virtual ~DocContext() = default;
    virtual epaper::document::DeviceDocument &document() = 0;
    virtual const epaper::document::DeviceDocument &document() const = 0;
    virtual void notifyHistory() = 0;
    virtual void noteDocumentMutated() = 0;
    virtual void flushWire() = 0;
};

} // namespace tools
} // namespace epaper
