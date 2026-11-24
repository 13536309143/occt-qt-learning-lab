#include "native_window.h"
#if defined(_WIN32)
#include <WNT_Window.hxx>
Handle(Aspect_Window) makeNativeWindow(WId id) {
  return new WNT_Window(reinterpret_cast<Aspect_Handle>(id));
}
#elif defined(__APPLE__)
#include <Cocoa_Window.hxx>
Handle(Aspect_Window) makeNativeWindow(WId id) {
  // Cocoa_Window.hxx provides an opaque NSView declaration for plain C++.
  return new Cocoa_Window(reinterpret_cast<NSView *>(id));
}
#else
#error This tutorial targets Windows and macOS.
#endif
