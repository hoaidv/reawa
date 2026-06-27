from __future__ import annotations

import objc
from AppKit import (
    NSControlTextDidChangeNotification,
    NSFont,
    NSMakeRect,
    NSScrollView,
    NSSearchField,
    NSTextField,
    NSTextView,
    NSView,
)
from Foundation import NSNotificationCenter
from PyObjCTools import AppHelper

from remarkable.services.app_log import app_log


class LogPanelController(objc.lookUpClass("NSObject")):
    def init(self):
        self = objc.super(LogPanelController, self).init()
        if self is None:
            return None
        self._panel_view: NSView | None = None
        self._search_field = None
        self._status_label = None
        self._text_view = None
        self._scroll_view = None
        self._search_query = ""
        self._stick_to_bottom = True
        app_log.add_listener(self._on_log_update)
        return self

    def build_view(self, frame) -> NSView:
        self._panel_view = NSView.alloc().initWithFrame_(frame)
        width = frame.size.width
        height = frame.size.height

        margin = 16
        search_h = 24
        status_h = 18
        top = height - margin - search_h

        self._search_field = NSSearchField.alloc().initWithFrame_(
            NSMakeRect(margin, top, width - margin * 2, search_h)
        )
        self._search_field.setPlaceholderString_("Search logs")
        self._search_field.setTarget_(self)
        self._search_field.setAction_("searchChanged:")
        NSNotificationCenter.defaultCenter().addObserver_selector_name_object_(
            self,
            "searchChanged:",
            NSControlTextDidChangeNotification,
            self._search_field,
        )
        self._panel_view.addSubview_(self._search_field)

        status_top = top - status_h - 6
        self._status_label = NSTextField.alloc().initWithFrame_(
            NSMakeRect(margin, status_top, width - margin * 2, status_h)
        )
        self._status_label.setEditable_(False)
        self._status_label.setBezeled_(False)
        self._status_label.setDrawsBackground_(False)
        self._status_label.setStringValue_("")
        self._panel_view.addSubview_(self._status_label)

        scroll_top = margin
        scroll_h = status_top - margin - 6
        self._scroll_view = NSScrollView.alloc().initWithFrame_(
            NSMakeRect(margin, scroll_top, width - margin * 2, scroll_h)
        )
        self._scroll_view.setHasVerticalScroller_(True)
        self._scroll_view.setHasHorizontalScroller_(False)
        self._scroll_view.setBorderType_(1)
        self._scroll_view.setAutohidesScrollers_(True)

        self._text_view = NSTextView.alloc().initWithFrame_(
            NSMakeRect(0, 0, width - margin * 2, scroll_h)
        )
        self._text_view.setEditable_(False)
        self._text_view.setSelectable_(True)
        self._text_view.setRichText_(False)
        self._text_view.setFont_(NSFont.userFixedPitchFontOfSize_(11))
        self._text_view.setVerticallyResizable_(True)
        self._text_view.setHorizontallyResizable_(False)
        self._text_view.textContainer().setWidthTracksTextView_(True)

        self._scroll_view.setDocumentView_(self._text_view)
        self._panel_view.addSubview_(self._scroll_view)

        self._refresh_display()
        return self._panel_view

    @objc.IBAction
    def searchChanged_(self, sender) -> None:
        self._search_query = self._search_field.stringValue().strip().lower()
        self._refresh_display()

    def _on_log_update(self) -> None:
        AppHelper.callAfter(self._refresh_display)

    def _refresh_display(self) -> None:
        if self._text_view is None:
            return

        entries = app_log.entries()
        query = self._search_query
        if query:
            filtered = [
                entry for entry in entries if query in entry.format_line().lower()
            ]
        else:
            filtered = entries

        text = "\n".join(entry.format_line() for entry in filtered)
        if not text and not entries:
            text = "No log entries yet."

        total = len(entries)
        shown = len(filtered)
        if query:
            self._status_label.setStringValue_(
                f"Showing {shown} of {total} entries matching “{self._search_field.stringValue().strip()}”"
            )
        else:
            self._status_label.setStringValue_(f"{total} entries")

        self._text_view.setString_(text)
        if self._stick_to_bottom and shown:
            length = len(text)
            self._text_view.scrollRangeToVisible_((length, 0))

    def note_visible(self) -> None:
        """Called when the Logs tab is shown."""
        self._refresh_display()
