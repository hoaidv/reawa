// Traceability (ADLC iter-000)
// @implements SRS-RW-01

import AppKit

let application = NSApplication.shared
let delegate = AppController()
application.delegate = delegate
application.setActivationPolicy(.accessory)
application.run()
