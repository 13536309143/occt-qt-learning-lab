#include "gui_experiments.h"
#include "cad_view.h"
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPushButton>
#include <QScreen>
#include <QSpinBox>
#include <QSysInfo>
#include <QTimer>
#include <QToolButton>
#include <QtTest/QTest>
#include <Standard_Version.hxx>
#include <iostream>
namespace {
void check(bool ok, const char *why) {
  if (!ok)
    throw std::runtime_error(why);
}
void wait() {
  QTest::qWait(180);
}
} // namespace
int runGuiExperiments(QMainWindow &window, CadView &widget, const QString &output) {
  QDir().mkpath(output);
  QJsonArray results;
  int failed = 0;
  auto test = [&](const char *name, std::function<void()> fn) {
    bool ok = true;
    QString error;
    try {
      fn();
    } catch (const Standard_Failure &e) {
      ok = false;
      error = e.GetMessageString();
    } catch (const std::exception &e) {
      ok = false;
      error = e.what();
    }
    if (!ok)
      ++failed;
    results.append(QJsonObject{{"name", name}, {"passed", ok}, {"error", error}});
    std::cout << (ok ? "PASS " : "FAIL ") << name << " " << error.toStdString() << std::endl;
  };
  auto snapshot = [&](const char *name) {
    wait();
    const QString path = QDir(output).filePath(QString(name) + ".png");
    widget.snapshot(path);
    QImage image(path);
    check(!image.isNull(), "PNG failed to decode");
    check(image.width() > 100 && image.height() > 100, "Viewport too small");
    int varied = 0;
    const auto bg = image.pixelColor(10, 10);
    for (int y = 0; y < image.height(); y += 10)
      for (int x = 0; x < image.width(); x += 10)
        if (image.pixelColor(x, y) != bg)
          ++varied;
    check(varied > 30, "Rendered image appears blank");
  };
  test("native_window_opengl_initialization", [&] {
    check(widget.initialized(), "Viewer not initialized");
    snapshot("01-box");
  });
  if (widget.initialized()) {
    test("toolbar_box_click", [&] {
      bool found = false;
      for (auto *b : window.findChildren<QToolButton *>())
        if (b->text() == "Box") {
          QTest::mouseClick(b, Qt::LeftButton);
          found = true;
          break;
        }
      check(found, "Box toolbar action not found");
      check(cad::stats(widget.currentShape()).faces == 6, "Box action failed");
    });
    test("face_selection_mouse_click", [&] {
      widget.box();
      widget.selectionMode(AIS_Shape::SelectionMode(TopAbs_FACE));
      auto p = widget.project(gp_Pnt(50, 40, 30));
      QTest::mouseClick(&widget, Qt::LeftButton, Qt::NoModifier, p);
      auto c = widget.occtContext();
      c->InitSelected();
      check(c->MoreSelected() && c->HasSelectedShape(), "No face selected");
      check(c->SelectedShape().ShapeType() == TopAbs_FACE, "Selection is not a face");
      snapshot("02-face-selection");
    });
    test("edge_selection_mouse_click", [&] {
      widget.selectionMode(AIS_Shape::SelectionMode(TopAbs_EDGE));
      auto p = widget.project(gp_Pnt(50, 0, 30));
      QTest::mouseClick(&widget, Qt::LeftButton, Qt::NoModifier, p);
      auto c = widget.occtContext();
      c->InitSelected();
      check(c->MoreSelected() && c->HasSelectedShape(), "No edge selected");
      check(c->SelectedShape().ShapeType() == TopAbs_EDGE, "Selection is not an edge");
      snapshot("03-edge-selection");
    });
    test("wheel_changes_camera_scale", [&] {
      auto v = widget.occtView();
      const auto before = v->Camera()->Scale();
      QPointF p(widget.width() / 2, widget.height() / 2);
      QWheelEvent event(p, widget.mapToGlobal(p.toPoint()), QPoint(), QPoint(0, 120), Qt::NoButton,
                        Qt::NoModifier, Qt::NoScrollPhase, false);
      QApplication::sendEvent(&widget, &event);
      check(v->Camera()->Scale() < before, "Wheel did not zoom in");
    });
    test("right_drag_rotates_camera", [&] {
      auto v = widget.occtView();
      auto before = v->Camera()->Direction();
      QPoint a(widget.width() / 2, widget.height() / 2), b = a + QPoint(100, 40);
      QTest::mousePress(&widget, Qt::RightButton, Qt::NoModifier, a);
      QMouseEvent move(QEvent::MouseMove, b, widget.mapToGlobal(b), Qt::NoButton, Qt::RightButton,
                       Qt::NoModifier);
      QApplication::sendEvent(&widget, &move);
      QTest::mouseRelease(&widget, Qt::RightButton, Qt::NoModifier, b);
      check(before.Dot(v->Camera()->Direction()) < 0.99999, "Drag did not rotate");
      snapshot("04-rotated");
    });
    test("middle_drag_pans_camera", [&] {
      auto v = widget.occtView();
      auto before = v->Camera()->Center();
      QPoint a(300, 300), b(360, 330);
      QTest::mousePress(&widget, Qt::MiddleButton, Qt::NoModifier, a);
      QMouseEvent move(QEvent::MouseMove, b, widget.mapToGlobal(b), Qt::NoButton, Qt::MiddleButton,
                       Qt::NoModifier);
      QApplication::sendEvent(&widget, &move);
      QTest::mouseRelease(&widget, Qt::MiddleButton, Qt::NoModifier, b);
      check(before.Distance(v->Camera()->Center()) > 0.1, "Pan did not move center");
    });
    test("resize_and_fit", [&] {
      window.resize(1000, 700);
      wait();
      widget.fitAll();
      snapshot("05-resized");
      window.resize(1280, 800);
      wait();
    });
    test("plate_render", [&] {
      widget.plate();
      snapshot("06-plate");
    });
    test("extrusion_render", [&] {
      widget.displayShape(cad::extrusion());
      snapshot("07-extrusion");
    });
    test("revolution_render", [&] {
      widget.displayShape(cad::revolved());
      snapshot("08-revolution");
    });
    test("loft_render", [&] {
      widget.displayShape(cad::loft());
      snapshot("09-loft");
    });
    test("fillet_render", [&] {
      widget.displayShape(cad::filleted());
      snapshot("10-fillet");
    });
    test("flange_render", [&] {
      widget.displayShape(cad::flange());
      snapshot("11-flange");
    });
    test("gui_step_roundtrip", [&] {
      QString p = QDir(output).filePath("gui-flange.step");
      double before = cad::stats(widget.currentShape()).volume;
      widget.exportStep(p);
      widget.box();
      widget.importStep(p);
      check(std::abs(cad::stats(widget.currentShape()).volume - before) < 1e-3,
            "STEP volume changed");
      snapshot("12-step-import");
    });
    test("invalid_model_preserves_previous", [&] {
      double before = cad::stats(widget.currentShape()).volume;
      bool rejected = false;
      try {
        widget.displayShape(TopoDS_Shape());
      } catch (const std::exception &) {
        rejected = true;
      }
      check(rejected, "Empty model accepted");
      check(std::abs(cad::stats(widget.currentShape()).volume - before) < 1e-6,
            "Previous model lost");
    });
    test("selection_after_resize", [&] {
      widget.box();
      widget.selectionMode(AIS_Shape::SelectionMode(TopAbs_FACE));
      auto p = widget.project(gp_Pnt(50, 40, 30));
      QTest::mouseClick(&widget, Qt::LeftButton, Qt::NoModifier, p);
      auto c = widget.occtContext();
      c->InitSelected();
      check(c->MoreSelected() && c->HasSelectedShape(), "Selection after resize failed");
    });
    test("parameter_dialog_six_holes", [&] {
      bool dialogSeen = false, inputSeen = false;
      QTimer::singleShot(300, &window, [&] {
        auto *dialog = qobject_cast<QDialog *>(QApplication::activeModalWidget());
        if (!dialog)
          return;
        dialogSeen = true;
        auto *count = dialog->findChild<QSpinBox *>();
        auto *buttons = dialog->findChild<QDialogButtonBox *>();
        if (count && buttons) {
          inputSeen = true;
          count->setValue(6);
          QTest::mouseClick(buttons->button(QDialogButtonBox::Ok), Qt::LeftButton);
        } else
          dialog->reject();
      });
      bool actionFound = false;
      for (auto *button : window.findChildren<QToolButton *>()) {
        if (button->text() == "Parameters") {
          actionFound = true;
          QTest::mouseClick(button, Qt::LeftButton);
          break;
        }
      }
      check(actionFound && dialogSeen && inputSeen, "Parameter dialog did not expose inputs");
      const double expected = std::acos(-1.0) * (2500 - 225 - 6 * 16) * 12;
      check(std::abs(cad::stats(widget.currentShape()).volume - expected) < 1e-3,
            "Parameter dialog did not rebuild the six-hole flange");
      snapshot("14-six-hole-parameters");
    });
    // Capture only this application's visible rectangle. Permission/OS restrictions
    // can prevent screen capture; viewport PNGs above remain the primary evidence.
    widget.displayShape(cad::flange());
    widget.statistics();
    wait();
    const QPoint origin = window.mapToGlobal(QPoint(0, 0));
    QPixmap screen =
        window.screen()->grabWindow(0, origin.x(), origin.y(), window.width(), window.height());
    if (!screen.isNull())
      screen.save(QDir(output).filePath("13-application-screen.png"));
  }
  QJsonObject root{{"platform", QSysInfo::prettyProductName()},
                   {"architecture", QSysInfo::currentCpuArchitecture()},
                   {"qt", qVersion()},
                   {"occt", OCC_VERSION_COMPLETE},
                   {"tests", results},
                   {"total", results.size()},
                   {"failed", failed},
                   {"device_pixel_ratio", widget.devicePixelRatioF()},
                   {"screenshot_note", "01-12 and 14 are actual OCCT viewport framebuffer dumps; "
                                       "13 is optional OS screen capture."}};
  QFile file(QDir(output).filePath("gui-results.json"));
  file.open(QIODevice::WriteOnly);
  file.write(QJsonDocument(root).toJson());
  std::cout << results.size() - failed << "/" << results.size() << " GUI experiments passed"
            << std::endl;
  return failed ? 1 : 0;
}
