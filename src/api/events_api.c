#include "api/events_api.h"

#include <JavaScriptCore/JavaScript.h>

static const char *events_source = 
"function EventEmitter() {\n"
"  this._events = {};\n"
"  this._maxListeners = 10;\n"
"}\n"
"\n"
"EventEmitter.prototype.on = function(event, listener) {\n"
"  if (!this._events[event]) this._events[event] = [];\n"
"  this._events[event].push(listener);\n"
"  return this;\n"
"};\n"
"\n"
"EventEmitter.prototype.emit = function(event) {\n"
"  if (!this._events[event]) return false;\n"
"  var listeners = this._events[event];\n"
"  for (var i = 0; i < listeners.length; i++) {\n"
"    listeners[i]();\n"
"  }\n"
"  return true;\n"
"};\n"
"\n"
"this.EventEmitter = EventEmitter;\n";

void init_events_api(JSGlobalContextRef ctx) {
  JSStringRef source = JSStringCreateWithUTF8CString(events_source);
  JSEvaluateScript(ctx, source, NULL, NULL, 1, NULL);
  JSStringRelease(source);
}