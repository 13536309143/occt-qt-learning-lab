#pragma once
#include <TopoDS_Shape.hxx>
#include <string>
namespace cad {
struct Stats {
  int faces, edges, vertices, solids;
  double volume;
};
void requireValid(const TopoDS_Shape &shape);
TopoDS_Shape box(double x = 100, double y = 80, double z = 30);
TopoDS_Shape plate(double radius = 12);
TopoDS_Shape extrusion(double height = 20);
TopoDS_Shape revolved();
TopoDS_Shape loft();
TopoDS_Shape filleted(double radius = 2);
TopoDS_Shape translated(const TopoDS_Shape &shape, double x, double y, double z);
TopoDS_Shape flange(double outer = 100, double thick = 12, double bore = 30, int holes = 4,
                    double holeDiameter = 8, double pitchDiameter = 70);
Stats stats(const TopoDS_Shape &shape);
void writeStep(const TopoDS_Shape &shape, const std::string &path);
TopoDS_Shape readStep(const std::string &path);
void writeBrep(const TopoDS_Shape &shape, const std::string &path);
TopoDS_Shape readBrep(const std::string &path);
void writeStl(const TopoDS_Shape &shape, const std::string &path);
} // namespace cad
