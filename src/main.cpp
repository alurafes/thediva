#include "engine.hpp"

int main() {
  thediva::engine engine {};

  engine.load_chart_by_file_path("build/dsc/pv_222_extreme.dsc");

  return 0;
}
