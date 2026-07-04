import AppKit

let application = NSApplication.shared
let delegate = AppController()
application.delegate = delegate
application.setActivationPolicy(.accessory)
application.run()
