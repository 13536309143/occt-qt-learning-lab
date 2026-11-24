#pragma once
#include "modeling.h"
#include "native_window.h"
#include <AIS_DisplayMode.hxx>
#include <AIS_InteractiveContext.hxx>
#include <AIS_Shape.hxx>
#include <Aspect_DisplayConnection.hxx>
#include <Aspect_TypeOfTriedronPosition.hxx>
#include <BRepAlgoAPI_Cut.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <BRepGProp.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>
#include <GProp_GProps.hxx>
#include <IFSelect_ReturnStatus.hxx>
#include <OpenGl_GraphicDriver.hxx>
#include <QAction>
#include <QApplication>
#include <QFileDialog>
#include <QMainWindow>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPaintEngine>
#include <QResizeEvent>
#include <QShowEvent>
#include <QStatusBar>
#include <QToolBar>
#include <QWheelEvent>
#include <Quantity_Color.hxx>
#include <STEPControl_Reader.hxx>
#include <STEPControl_Writer.hxx>
#include <Standard_Failure.hxx>
#include <TopAbs_ShapeEnum.hxx>
#include <TopExp.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopoDS_Shape.hxx>
#include <V3d_View.hxx>
#include <V3d_Viewer.hxx>
#include <cmath>
#include <functional>
#include <gp_Ax2.hxx>
#include <gp_Dir.hxx>
#include <gp_Pnt.hxx>
#include <stdexcept>

class CadView : public QWidget {
public:
  explicit CadView(QWidget *parent = nullptr) : QWidget(parent) {
    setAttribute(Qt::WA_NativeWindow);
    setAttribute(Qt::WA_PaintOnScreen);
    setAttribute(Qt::WA_NoSystemBackground);
    setMouseTracking(true);
    setMinimumSize(480, 360);
  }
  ~CadView() override {
    if (!context.IsNull())
      context->RemoveAll(false);
    if (!view.IsNull())
      view->Remove();
  }
  std::function<void(const QString &)> report;

  void displayShape(const TopoDS_Shape &candidate) {
    if (view.IsNull())
      throw std::runtime_error("Viewer has not initialized");
    if (candidate.IsNull())
      throw std::runtime_error("Empty shape");
    if (!BRepCheck_Analyzer(candidate).IsValid())
      throw std::runtime_error("Shape validation failed");
    // Validate before removing the previously displayed model.
    context->RemoveAll(false);
    shape = candidate;
    ais = new AIS_Shape(shape);
    ais->SetColor(Quantity_Color(0.25, 0.65, 0.88, Quantity_TOC_RGB));
    context->Display(ais, false);
    context->SetDisplayMode(ais, AIS_Shaded, false);
    selectionMode(0);
    fitAll();
  }
  void box() {
    displayShape(cad::box());
  }
  void plate() {
    displayShape(cad::plate());
  }
  bool initialized() const {
    return !view.IsNull() && !shape.IsNull();
  }
  Handle(V3d_View) occtView() const {
    return view;
  }
  Handle(AIS_InteractiveContext) occtContext() const {
    return context;
  }
  const TopoDS_Shape &currentShape() const {
    return shape;
  }
  void snapshot(const QString &path) {
    if (!initialized() || !view->Dump(path.toUtf8().constData()))
      throw std::runtime_error("Viewport snapshot failed");
  }
  QPoint project(const gp_Pnt &point) const {
    int x = 0, y = 0, w = 1, h = 1;
    view->Convert(point.X(), point.Y(), point.Z(), x, y);
    nativeWindow->Size(w, h);
    return QPoint(qRound(double(x) * width() / w), qRound(double(y) * height() / h));
  }
  void fitAll() {
    if (view.IsNull())
      return;
    view->FitAll();
    view->ZFitAll();
    view->Redraw();
  }
  void selectionMode(int mode) {
    if (ais.IsNull())
      return;
    context->ClearSelected(false);
    context->Deactivate(ais);
    context->Activate(ais, mode);
    context->UpdateCurrentViewer();
  }
  void importStep(const QString &path) {
    displayShape(cad::readStep(path.toUtf8().constData()));
  }
  void exportStep(const QString &path) {
    cad::writeStep(shape, path.toUtf8().constData());
  }
  void statistics() {
    auto s = cad::stats(shape);
    if (report)
      report(QString("Faces %1 | Edges %2 | Vertices %3 | Solids %4 | Volume %5 mm^3")
                 .arg(s.faces)
                 .arg(s.edges)
                 .arg(s.vertices)
                 .arg(s.solids)
                 .arg(s.volume, 0, 'f', 3));
  }

protected:
  QPaintEngine *paintEngine() const override {
    return nullptr;
  }
  void showEvent(QShowEvent *event) override {
    QWidget::showEvent(event);
    if (!view.IsNull())
      return;
    try {
      display = new Aspect_DisplayConnection;
      driver = new OpenGl_GraphicDriver(display);
      viewer = new V3d_Viewer(driver);
      viewer->SetDefaultLights();
      viewer->SetLightOn();
      context = new AIS_InteractiveContext(viewer);
      view = viewer->CreateView();
      nativeWindow = makeNativeWindow(winId());
      view->SetWindow(nativeWindow);
      if (!nativeWindow->IsMapped())
        nativeWindow->Map();
      view->SetBackgroundColor(Quantity_Color(0.12, 0.14, 0.18, Quantity_TOC_RGB));
      view->TriedronDisplay(Aspect_TOTP_LEFT_LOWER, Quantity_NOC_WHITE, 0.08, V3d_ZBUFFER);
      view->SetProj(V3d_XposYnegZpos);
      view->MustBeResized();
      box();
    } catch (const Standard_Failure &e) {
      if (!qApp->property("selfTest").toBool())
        QMessageBox::critical(this, "OCCT initialization", QString::fromUtf8(e.GetMessageString()));
    } catch (const std::exception &e) {
      if (!qApp->property("selfTest").toBool())
        QMessageBox::critical(this, "Initialization", QString::fromUtf8(e.what()));
    }
  }
  void paintEvent(QPaintEvent *) override {
    if (!view.IsNull()) {
      view->MustBeResized();
      view->Redraw();
    }
  }
  void resizeEvent(QResizeEvent *event) override {
    QWidget::resizeEvent(event);
    if (!view.IsNull()) {
      view->MustBeResized();
      update();
    }
  }
  // Convert Qt logical coordinates to the actual OCCT viewport dimensions.
  // This avoids assuming that macOS Cocoa uses the same scaling as Windows.
  QPoint pixels(const QPointF &point) const {
    int w = 1, h = 1;
    nativeWindow->Size(w, h);
    return QPoint(qRound(point.x() * w / qMax(1, width())),
                  qRound(point.y() * h / qMax(1, height())));
  }
  void mousePressEvent(QMouseEvent *event) override {
    if (view.IsNull())
      return;
    press = last = pixels(event->position());
    if (event->button() == Qt::RightButton)
      view->StartRotation(last.x(), last.y());
  }
  void mouseMoveEvent(QMouseEvent *event) override {
    if (view.IsNull())
      return;
    const QPoint p = pixels(event->position());
    if (event->buttons() & Qt::RightButton)
      view->Rotation(p.x(), p.y());
    else if (event->buttons() & Qt::MiddleButton)
      view->Pan(p.x() - last.x(), last.y() - p.y());
    else
      context->MoveTo(p.x(), p.y(), view, true);
    last = p;
  }
  void mouseReleaseEvent(QMouseEvent *event) override {
    if (view.IsNull() || event->button() != Qt::LeftButton)
      return;
    const QPoint p = pixels(event->position());
    if ((p - press).manhattanLength() > 5)
      return;
    context->MoveTo(p.x(), p.y(), view, false);
    context->SelectDetected(event->modifiers().testFlag(Qt::ShiftModifier)
                                ? AIS_SelectionScheme_XOR
                                : AIS_SelectionScheme_Replace);
    context->UpdateCurrentViewer();
    context->InitSelected();
    if (context->MoreSelected() && context->HasSelectedShape() && report)
      report(QString("Selected topology type: %1 (FACE=4, EDGE=6, VERTEX=7)")
                 .arg(static_cast<int>(context->SelectedShape().ShapeType())));
  }
  void wheelEvent(QWheelEvent *event) override {
    if (view.IsNull())
      return;
    double steps = event->angleDelta().y() / 120.0;
    if (steps == 0.0)
      steps = event->pixelDelta().y() / 60.0;
    if (steps == 0.0)
      return;
    view->SetZoom(std::pow(1.15, qBound(-10.0, steps, 10.0)));
    view->Redraw();
    event->accept();
  }

private:
  Handle(Aspect_DisplayConnection) display;
  Handle(OpenGl_GraphicDriver) driver;
  Handle(V3d_Viewer) viewer;
  Handle(AIS_InteractiveContext) context;
  Handle(V3d_View) view;
  Handle(Aspect_Window) nativeWindow;
  Handle(AIS_Shape) ais;
  TopoDS_Shape shape;
  QPoint press, last;
};
