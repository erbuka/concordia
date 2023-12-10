import sys;
import os;
from functools import reduce

bytes_per_line = 40

def main():
  root = sys.argv[1]
  os.chdir(root)

  print("#include <array>")
  print("#include <cinttypes>")
  print("#include <cstddef>", end="\n\n\n")
  for (base, dir, files) in os.walk("."):
    ns = "cnc::" + base.replace("\\", "::").replace("/", "::").replace(".", "assets")
    print(f"namespace {ns} {{")
    for file_name in files:
      file = open(os.path.join(base, file_name), mode="rb")
      bytes = file.read()
      length = len(bytes)
      var_name = os.path.basename(file_name).replace(".", "_")
      # content = reduce(lambda a, b: f"{a}, {str(b)}", bytes, "")
      print(f"\tinline constexpr std::array<std::uint8_t, {length}> {var_name} = {{")

      for i, b in enumerate(bytes):
        new_line = (i+1) % bytes_per_line == 0
        padding = "\t\t" if i % bytes_per_line == 0 else ""
        print(f"{ padding }{str(b)}, ", end=None if new_line else "")
      print("\t};")            
      file.close()
    print("}")

if __name__ == "__main__":
  main()
