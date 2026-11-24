#include "modeling.h"
#include <BRepAlgoAPI_Cut.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakePolygon.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <BRepFilletAPI_MakeFillet.hxx>
#include <BRepGProp.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <BRepOffsetAPI_ThruSections.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>
#include <BRepPrimAPI_MakePrism.hxx>
#include <BRepPrimAPI_MakeRevol.hxx>
#include <BRepTools.hxx>
#include <BRep_Builder.hxx>
#include <GProp_GProps.hxx>
#include <Interface_Static.hxx>
#include <STEPControl_Reader.hxx>
#include <STEPControl_Writer.hxx>
#include <StlAPI_Writer.hxx>
#include <TopExp.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Wire.hxx>
#include <cmath>
#include <gp_Ax1.hxx>
#include <gp_Ax2.hxx>
#include <gp_Trsf.hxx>
#include <gp_Vec.hxx>
#include <stdexcept>
namespace cad {
namespace {
void positive(double v) {
  if (!std::isfinite(v) || v <= 0)
    throw std::invalid_argument("Dimension must be finite and positive");
}
TopoDS_Shape cut(const TopoDS_Shape &a, const TopoDS_Shape &b) {
  BRepAlgoAPI_Cut op(a, b);
  op.Build();
  if (!op.IsDone())
    throw std::runtime_error("Boolean cut failed");
  requireValid(op.Shape());
  return op.Shape();
}
TopoDS_Wire rectangle(double w, double d, double z) {
  BRepBuilderAPI_MakePolygon p;
  p.Add(gp_Pnt(0, 0, z));
  p.Add(gp_Pnt(w, 0, z));
  p.Add(gp_Pnt(w, d, z));
  p.Add(gp_Pnt(0, d, z));
  p.Close();
  if (!p.IsDone())
    throw std::runtime_error("Wire failed");
  return p.Wire();
}
} // namespace
void requireValid(const TopoDS_Shape &s) {
  if (s.IsNull())
    throw std::invalid_argument("Empty shape");
  if (!BRepCheck_Analyzer(s).IsValid())
    throw std::runtime_error("Invalid BRep");
}
TopoDS_Shape box(double x, double y, double z) {
  positive(x);
  positive(y);
  positive(z);
  BRepPrimAPI_MakeBox m(x, y, z);
  m.Build();
  if (!m.IsDone())
    throw std::runtime_error("Box failed");
  requireValid(m.Shape());
  return m.Shape();
}
TopoDS_Shape plate(double r) {
  positive(r);
  if (r >= 40)
    throw std::invalid_argument("Hole too large");
  BRepPrimAPI_MakeCylinder c(gp_Ax2(gp_Pnt(50, 40, -5), gp_Dir(0, 0, 1)), r, 40);
  c.Build();
  if (!c.IsDone())
    throw std::runtime_error("Tool failed");
  return cut(box(), c.Shape());
}
TopoDS_Shape extrusion(double h) {
  positive(h);
  BRepBuilderAPI_MakePolygon p;
  for (auto point : {gp_Pnt(0, 0, 0), gp_Pnt(100, 0, 0), gp_Pnt(100, 20, 0), gp_Pnt(30, 20, 0),
                     gp_Pnt(30, 60, 0), gp_Pnt(0, 60, 0)})
    p.Add(point);
  p.Close();
  if (!p.IsDone())
    throw std::runtime_error("Profile failed");
  BRepBuilderAPI_MakeFace f(p.Wire());
  if (!f.IsDone())
    throw std::runtime_error("Face failed");
  BRepPrimAPI_MakePrism op(f.Face(), gp_Vec(0, 0, h));
  op.Build();
  if (!op.IsDone())
    throw std::runtime_error("Extrusion failed");
  requireValid(op.Shape());
  return op.Shape();
}
TopoDS_Shape revolved() {
  BRepBuilderAPI_MakePolygon p;
  p.Add(gp_Pnt(10, 0, 0));
  p.Add(gp_Pnt(20, 0, 0));
  p.Add(gp_Pnt(20, 0, 30));
  p.Add(gp_Pnt(10, 0, 30));
  p.Close();
  BRepBuilderAPI_MakeFace f(p.Wire());
  if (!f.IsDone())
    throw std::runtime_error("Revolution profile failed");
  BRepPrimAPI_MakeRevol op(f.Face(), gp_Ax1(gp_Pnt(0, 0, 0), gp_Dir(0, 0, 1)));
  op.Build();
  if (!op.IsDone())
    throw std::runtime_error("Revolution failed");
  requireValid(op.Shape());
  return op.Shape();
}
TopoDS_Shape loft() {
  BRepOffsetAPI_ThruSections op(true, false);
  op.AddWire(rectangle(80, 60, 0));
  op.AddWire(rectangle(40, 30, 60));
  op.Build();
  if (!op.IsDone())
    throw std::runtime_error("Loft failed");
  requireValid(op.Shape());
  return op.Shape();
}
TopoDS_Shape filleted(double radius) {
  positive(radius);
  if (radius >= 15)
    throw std::invalid_argument("Radius must be less than half the smallest box size");
  auto s = box();
  TopTools_IndexedMapOfShape edges;
  TopExp::MapShapes(s, TopAbs_EDGE, edges);
  BRepFilletAPI_MakeFillet op(s);
  for (int i = 1; i <= edges.Extent(); ++i)
    op.Add(radius, TopoDS::Edge(edges(i)));
  op.Build();
  if (!op.IsDone())
    throw std::runtime_error("Fillet failed");
  requireValid(op.Shape());
  return op.Shape();
}
TopoDS_Shape translated(const TopoDS_Shape &s, double x, double y, double z) {
  requireValid(s);
  if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(z))
    throw std::invalid_argument("Nonfinite translation");
  gp_Trsf t;
  t.SetTranslation(gp_Vec(x, y, z));
  BRepBuilderAPI_Transform op(s, t, true);
  op.Build();
  if (!op.IsDone())
    throw std::runtime_error("Transform failed");
  requireValid(op.Shape());
  return op.Shape();
}
TopoDS_Shape flange(double outer, double thick, double bore, int holes, double hd, double pitch) {
  positive(outer);
  positive(thick);
  positive(bore);
  positive(hd);
  positive(pitch);
  if (holes < 1 || holes > 64 || bore >= outer || pitch + hd >= outer || pitch - hd <= bore ||
      (holes > 1 && pitch * std::sin(std::acos(-1.0) / holes) <= hd))
    throw std::invalid_argument("Flange holes overlap or exceed available material");
  BRepPrimAPI_MakeCylinder base(outer / 2, thick);
  base.Build();
  BRepPrimAPI_MakeCylinder center(gp_Ax2(gp_Pnt(0, 0, -1), gp_Dir(0, 0, 1)), bore / 2, thick + 2);
  center.Build();
  auto s = cut(base.Shape(), center.Shape());
  for (int i = 0; i < holes; ++i) {
    double a = 2 * std::acos(-1.0) * i / holes;
    BRepPrimAPI_MakeCylinder tool(
        gp_Ax2(gp_Pnt(pitch / 2 * std::cos(a), pitch / 2 * std::sin(a), -1), gp_Dir(0, 0, 1)),
        hd / 2, thick + 2);
    tool.Build();
    s = cut(s, tool.Shape());
  }
  return s;
}
Stats stats(const TopoDS_Shape &s) {
  requireValid(s);
  TopTools_IndexedMapOfShape f, e, v, sol;
  TopExp::MapShapes(s, TopAbs_FACE, f);
  TopExp::MapShapes(s, TopAbs_EDGE, e);
  TopExp::MapShapes(s, TopAbs_VERTEX, v);
  TopExp::MapShapes(s, TopAbs_SOLID, sol);
  GProp_GProps p;
  BRepGProp::VolumeProperties(s, p);
  return {f.Extent(), e.Extent(), v.Extent(), sol.Extent(), p.Mass()};
}
void writeStep(const TopoDS_Shape &s, const std::string &path) {
  requireValid(s);
  STEPControl_Writer w;
  Interface_Static::SetCVal("write.step.unit", "MM");
  if (w.Transfer(s, STEPControl_AsIs) != IFSelect_RetDone ||
      w.Write(path.c_str()) != IFSelect_RetDone)
    throw std::runtime_error("STEP export failed");
}
TopoDS_Shape readStep(const std::string &path) {
  STEPControl_Reader r;
  Interface_Static::SetCVal("xstep.cascade.unit", "MM");
  if (r.ReadFile(path.c_str()) != IFSelect_RetDone || r.TransferRoots() <= 0)
    throw std::runtime_error("STEP import failed");
  auto s = r.OneShape();
  requireValid(s);
  return s;
}
void writeBrep(const TopoDS_Shape &s, const std::string &path) {
  requireValid(s);
  if (!BRepTools::Write(s, path.c_str()))
    throw std::runtime_error("BREP export failed");
}
TopoDS_Shape readBrep(const std::string &path) {
  TopoDS_Shape s;
  BRep_Builder b;
  if (!BRepTools::Read(s, path.c_str(), b))
    throw std::runtime_error("BREP import failed");
  requireValid(s);
  return s;
}
void writeStl(const TopoDS_Shape &s, const std::string &path) {
  requireValid(s);
  BRepMesh_IncrementalMesh mesh(s, 0.1, false, 0.3, true);
  if (!mesh.IsDone())
    throw std::runtime_error("Meshing failed");
  StlAPI_Writer w;
  w.ASCIIMode() = false;
  if (!w.Write(s, path.c_str()))
    throw std::runtime_error("STL export failed");
}
} // namespace cad
