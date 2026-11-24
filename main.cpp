#include "cad_view.h"
#include "gui_experiments.h"
#include <QComboBox>
#include <QCommandLineParser>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QSpinBox>
#include <QTimer>

int main(int argc, char **argv) {
  QApplication app(argc, argv);
  QCommandLineParser parser;
  parser.addHelpOption();
  parser.addOption({"self-test", "Run GUI experiments and exit"});
  parser.addOption({"output", "Experiment artifact directory", "directory", "gui-artifacts"});
  parser.process(app);
  app.setProperty("selfTest", parser.isSet("self-test"));
  QMainWindow window;
  window.setWindowTitle("Qt 6 + OCCT 7.9 learning project");
  auto *cad = new CadView;
  window.setCentralWidget(cad);
  auto *toolbar = window.addToolBar("CAD");
  cad->report = [&window](const QString &text) { window.statusBar()->showMessage(text); };
  const auto action = [&](const QString &text, std::function<void()> operation) {
    QAction *a = toolbar->addAction(text);
    QObject::connect(a, &QAction::triggered, &window, [&window, operation] {
      try {
        operation();
      } catch (const Standard_Failure &e) {
        QMessageBox::critical(&window, "OCCT", QString::fromUtf8(e.GetMessageString()));
      } catch (const std::exception &e) {
        QMessageBox::critical(&window, "Error", QString::fromUtf8(e.what()));
      }
    });
  };
  action("Box", [cad] { cad->box(); });
  action("Plate + hole", [cad] { cad->plate(); });
  auto *lessons = new QComboBox;
  lessons->addItems(
      {"More experiments...", "L extrusion", "Revolution", "Loft", "Fillet", "Flange"});
  toolbar->addWidget(lessons);
  QObject::connect(lessons, &QComboBox::activated, &window, [&window, cad](int index) {
    try {
      switch (index) {
      case 1:
        cad->displayShape(cad::extrusion());
        break;
      case 2:
        cad->displayShape(cad::revolved());
        break;
      case 3:
        cad->displayShape(cad::loft());
        break;
      case 4:
        cad->displayShape(cad::filleted());
        break;
      case 5:
        cad->displayShape(cad::flange());
        break;
      }
    } catch (const Standard_Failure &e) {
      QMessageBox::critical(&window, "OCCT", e.GetMessageString());
    } catch (const std::exception &e) {
      QMessageBox::critical(&window, "Model", e.what());
    }
  });
  action("Parameters", [&window, cad] {
    QDialog dialog(&window);
    dialog.setWindowTitle("Parametric flange (mm)");
    QFormLayout form(&dialog);
    QDoubleSpinBox outer, thickness, bore, hole, pitch;
    QSpinBox count;
    for (auto *spin : {&outer, &thickness, &bore, &hole, &pitch}) {
      spin->setRange(0.1, 10000);
      spin->setDecimals(2);
    }
    outer.setValue(100);
    thickness.setValue(12);
    bore.setValue(30);
    hole.setValue(8);
    pitch.setValue(70);
    count.setRange(1, 64);
    count.setValue(4);
    form.addRow("Outer diameter", &outer);
    form.addRow("Thickness", &thickness);
    form.addRow("Bore diameter", &bore);
    form.addRow("Hole count", &count);
    form.addRow("Hole diameter", &hole);
    form.addRow("Pitch diameter", &pitch);
    QDialogButtonBox buttons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    form.addRow(&buttons);
    QObject::connect(&buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    QObject::connect(&buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    if (dialog.exec() == QDialog::Accepted)
      cad->displayShape(cad::flange(outer.value(), thickness.value(), bore.value(), count.value(),
                                    hole.value(), pitch.value()));
  });
  action("Fit", [cad] { cad->fitAll(); });
  action("Object", [cad] { cad->selectionMode(0); });
  action("Face", [cad] { cad->selectionMode(AIS_Shape::SelectionMode(TopAbs_FACE)); });
  action("Edge", [cad] { cad->selectionMode(AIS_Shape::SelectionMode(TopAbs_EDGE)); });
  action("Info", [cad] { cad->statistics(); });
  action("Open STEP", [&window, cad] {
    const QString path =
        QFileDialog::getOpenFileName(&window, "Open STEP", {}, "STEP (*.step *.stp)");
    if (!path.isEmpty())
      cad->importStep(path);
  });
  action("Save STEP", [&window, cad] {
    QString path =
        QFileDialog::getSaveFileName(&window, "Save STEP", "model.step", "STEP (*.step *.stp)");
    if (path.isEmpty())
      return;
    if (!path.endsWith(".step", Qt::CaseInsensitive) && !path.endsWith(".stp", Qt::CaseInsensitive))
      path += ".step";
    cad->exportStep(path);
  });
  window.statusBar()->showMessage("Left: select | Shift+left: multi-select | Right drag: rotate | "
                                  "Middle drag: pan | Wheel: zoom");
  window.resize(1280, 800);
  window.show();
  if (parser.isSet("self-test")) {
    const QString output = parser.value("output");
    QTimer::singleShot(1200, &window, [&app, &window, cad, output] {
      app.exit(runGuiExperiments(window, *cad, output));
    });
  }
  return app.exec();
}
