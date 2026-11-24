#include "modeling.h"
#include <BRepAlgoAPI_Common.hxx>
#include <BRepAlgoAPI_Fuse.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakePolygon.hxx>
#include <BRepGProp.hxx>
#include <BRepPrimAPI_MakeSphere.hxx>
#include <GProp_GProps.hxx>
#include <Standard_Failure.hxx>
#include <TopoDS_Wire.hxx>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>
namespace fs = std::filesystem;
void check(bool ok, const char *message) {
  if (!ok)
    throw std::runtime_error(message);
}
void expectNear(double a, double b, double tolerance = 1e-5) {
  if (std::abs(a - b) > tolerance)
    throw std::runtime_error("Unexpected numerical value: " + std::to_string(a) + " expected " +
                             std::to_string(b));
}
void rejects(const std::function<void()> &fn) {
  bool failed = false;
  try {
    fn();
  } catch (const std::exception &) {
    failed = true;
  } catch (const Standard_Failure &) {
    failed = true;
  }
  check(failed, "Invalid input unexpectedly accepted");
}
std::string escape(std::string s) {
  std::string r;
  for (char c : s) {
    if (c == '"' || c == '\\')
      r += '\\';
    if (c == '\n')
      r += "\\n";
    else if (c == '\r')
      r += "\\r";
    else
      r += c;
  }
  return r;
}
int main(int argc, char **argv) {
  const fs::path out = argc > 1 ? fs::u8path(argv[1]) : fs::path("test-artifacts");
  fs::create_directories(out);
  const double pi = std::acos(-1.0);
  struct Result {
    std::string name;
    bool ok;
    std::string error;
  };
  std::vector<Result> results;
  auto test = [&](std::string name, std::function<void()> run) {
    Result r{name, true, {}};
    try {
      run();
    } catch (const Standard_Failure &e) {
      r.ok = false;
      r.error = e.GetMessageString();
    } catch (const std::exception &e) {
      r.ok = false;
      r.error = e.what();
    }
    std::cout << (r.ok ? "PASS " : "FAIL ") << name << " " << r.error << "\n";
    results.push_back(r);
  };
  test("box_topology", [] {
    auto s = cad::stats(cad::box());
    check(s.faces == 6 && s.edges == 12 && s.vertices == 8 && s.solids == 1, "Box topology");
  });
  test("box_volume", [] { expectNear(cad::stats(cad::box()).volume, 240000); });
  test("box_center", [] {
    GProp_GProps p;
    BRepGProp::VolumeProperties(cad::box(), p);
    expectNear(p.CentreOfMass().X(), 50);
    expectNear(p.CentreOfMass().Y(), 40);
    expectNear(p.CentreOfMass().Z(), 15);
  });
  test("box_custom_dimensions", [] { expectNear(cad::stats(cad::box(2, 3, 4)).volume, 24); });
  test("plate_volume",
       [&] { expectNear(cad::stats(cad::plate()).volume, 240000 - pi * 144 * 30); });
  test("plate_single_solid", [] {
    auto s = cad::stats(cad::plate());
    check(s.solids == 1 && s.faces == 7, "Plate topology");
  });
  test("extrusion_volume", [] { expectNear(cad::stats(cad::extrusion()).volume, 64000); });
  test("revolution_volume",
       [&] { expectNear(cad::stats(cad::revolved()).volume, pi * (400 - 100) * 30); });
  test("loft_volume", [] { expectNear(cad::stats(cad::loft()).volume, 168000, 1e-3); });
  test("fillet_valid_and_removes_material", [] {
    auto s = cad::stats(cad::filleted());
    check(s.solids == 1 && s.volume > 200000 && s.volume < 240000, "Fillet volume");
  });
  test("translation_volume",
       [] { expectNear(cad::stats(cad::translated(cad::box(), 120, -30, 40)).volume, 240000); });
  test("translation_center", [] {
    GProp_GProps p;
    BRepGProp::VolumeProperties(cad::translated(cad::box(), 120, -30, 40), p);
    expectNear(p.CentreOfMass().X(), 170);
    expectNear(p.CentreOfMass().Y(), 10);
    expectNear(p.CentreOfMass().Z(), 55);
  });
  test("flange_volume", [&] {
    expectNear(cad::stats(cad::flange()).volume, pi * (2500 - 225 - 4 * 16) * 12, 1e-3);
  });
  test("flange_six_holes", [&] {
    expectNear(cad::stats(cad::flange(100, 12, 30, 6, 8, 70)).volume,
               pi * (2500 - 225 - 6 * 16) * 12, 1e-3);
  });
  test("sphere_volume", [&] {
    BRepPrimAPI_MakeSphere m(10);
    m.Build();
    expectNear(cad::stats(m.Shape()).volume, 4 * pi * 1000 / 3);
  });
  test("boolean_union_volume", [] {
    auto a = cad::box(10, 10, 10);
    auto b = cad::translated(a, 5, 0, 0);
    BRepAlgoAPI_Fuse op(a, b);
    op.Build();
    check(op.IsDone(), "Fuse failed");
    expectNear(cad::stats(op.Shape()).volume, 1500);
  });
  test("boolean_intersection_volume", [] {
    auto a = cad::box(10, 10, 10);
    auto b = cad::translated(a, 5, 0, 0);
    BRepAlgoAPI_Common op(a, b);
    op.Build();
    check(op.IsDone(), "Common failed");
    expectNear(cad::stats(op.Shape()).volume, 500);
  });
  test("reject_zero_dimension", [] { rejects([] { cad::box(0, 1, 1); }); });
  test("reject_negative_dimension", [] { rejects([] { cad::box(1, -1, 1); }); });
  test("reject_nan_dimension",
       [] { rejects([] { cad::box(std::numeric_limits<double>::quiet_NaN(), 1, 1); }); });
  test("reject_infinite_dimension",
       [] { rejects([] { cad::box(1, 1, std::numeric_limits<double>::infinity()); }); });
  test("reject_oversized_plate_hole", [] { rejects([] { cad::plate(40); }); });
  test("reject_zero_extrusion", [] { rejects([] { cad::extrusion(0); }); });
  test("reject_oversized_fillet", [] { rejects([] { cad::filleted(15); }); });
  test("reject_invalid_flange_count", [] { rejects([] { cad::flange(100, 12, 30, 0); }); });
  test("reject_overlapping_flange_holes", [] { rejects([] { cad::flange(100, 12, 30, 40); }); });
  test("reject_empty_shape", [] { rejects([] { cad::requireValid(TopoDS_Shape()); }); });
  test("reject_invalid_translation", [] {
    rejects([] { cad::translated(cad::box(), std::numeric_limits<double>::infinity(), 0, 0); });
  });
  test("step_roundtrip", [&] {
    auto p = (out / "plate.step").u8string();
    cad::writeStep(cad::plate(), p);
    auto s = cad::stats(cad::readStep(p));
    expectNear(s.volume, 240000 - pi * 144 * 30, 1e-3);
    check(s.solids == 1, "STEP solids");
  });
  test("step_unicode_filename", [&] {
    auto p = (out / fs::u8path(u8"法兰盘.step")).u8string();
    cad::writeStep(cad::flange(), p);
    expectNear(cad::stats(cad::readStep(p)).volume, cad::stats(cad::flange()).volume, 1e-3);
  });
  test("brep_roundtrip", [&] {
    auto p = (out / "flange.brep").u8string();
    cad::writeBrep(cad::flange(), p);
    expectNear(cad::stats(cad::readBrep(p)).volume, cad::stats(cad::flange()).volume);
  });
  test("binary_stl_triangle_count", [&] {
    auto p = out / "flange.stl";
    cad::writeStl(cad::flange(), p.u8string());
    std::ifstream in(p, std::ios::binary);
    in.seekg(80);
    unsigned char n[4] = {};
    in.read(reinterpret_cast<char *>(n), 4);
    unsigned count = n[0] | (unsigned(n[1]) << 8) | (unsigned(n[2]) << 16) | (unsigned(n[3]) << 24);
    check(count > 0, "No STL triangles");
    check(fs::file_size(p) == 84ULL + 50ULL * count, "STL size mismatch");
  });
  test("reject_missing_step",
       [&] { rejects([&] { cad::readStep((out / "missing.step").u8string()); }); });
  test("reject_corrupt_step", [&] {
    auto p = out / "corrupt.step";
    std::ofstream(p) << "not a STEP file\n";
    rejects([&] { cad::readStep(p.u8string()); });
  });
  test("reject_unwritable_step", [&] {
    rejects(
        [&] { cad::writeStep(cad::box(), (out / "missing-directory" / "box.step").u8string()); });
  });
  test("reject_missing_brep",
       [&] { rejects([&] { cad::readBrep((out / "missing.brep").u8string()); }); });
  std::ofstream json(out / "model-results.json");
  int failed = 0;
  json << "{\n  \"tests\": [\n";
  for (size_t i = 0; i < results.size(); ++i) {
    const auto &r = results[i];
    if (!r.ok)
      ++failed;
    json << "    {\"name\":\"" << escape(r.name) << "\",\"passed\":" << (r.ok ? "true" : "false")
         << ",\"error\":\"" << escape(r.error) << "\"}" << (i + 1 < results.size() ? "," : "")
         << "\n";
  }
  json << "  ],\n  \"total\": " << results.size() << ", \"failed\": " << failed << "\n}\n";
  std::cout << results.size() - failed << "/" << results.size() << " passed\n";
  return failed ? 1 : 0;
}
