import argparse
import os
from glob import glob
from typing import TypeAlias

import pefile

parser = argparse.ArgumentParser()
parser.add_argument("dll_path")

args = parser.parse_args()

Imports: TypeAlias = dict[bytes, list[tuple[bytes | None, int | None]]]
Exports: TypeAlias = list[tuple[bytes | None, int | None]]


class MissingDLL(Exception):
    pass


class MissingFunction(Exception):
    pass


def load_imports(path: str) -> Imports:
    print(f"Loading imports from {path}")
    pe = pefile.PE(path)
    imports: Imports = {}
    total = 0
    for entry in pe.DIRECTORY_ENTRY_IMPORT:
        these = []
        for imp in entry.imports:
            these.append((imp.name, imp.ordinal))

        imports[entry.dll.lower()] = these
        total += len(these)

    print(f"  ...{total} imports from {len(imports)} DLLs")

    return imports


def load_exports(path: str) -> Exports:
    print(f"Loading exports from {path}...")
    pe = pefile.PE(path)
    exports: Exports = []
    for exp in pe.DIRECTORY_ENTRY_EXPORT.symbols:
        exports.append((exp.name, exp.ordinal))

    print(f"  ...{len(exports)} exports")

    return exports


available_functions: dict[bytes, Exports] = {}
for dll in glob("./xp_dlls/*.dll"):
    available_functions[os.path.basename(dll).lower().encode()] = load_exports(dll)

needed_functions = load_imports(args.dll_path)

# https://www.geoffchappell.com/studies/windows/win32/kernel32/api/index.htm
# my test .dll is 32-bit and these are only in 64
ignore_functions = [
    b"RtlLookupFunctionEntry",
    b"RtlUnwindEx",
    b"RtlVirtualUnwind",
    b"__C_specific_handler",
]

for dll_name, imports in needed_functions.items():
    if (dll := available_functions.get(dll_name)) is None:
        raise MissingDLL(f"Need {dll_name} but it's not in the XP DLLs folder")

    for name, ordinal in imports:
        if name is not None:
            if name in ignore_functions:
                continue

            if next((fn for fn in dll if fn[0] == name), None) is None:
                raise MissingFunction(f'Function "{name}" not present in XP {dll_name}')
        elif ordinal is not None:
            if next((fn for fn in dll if fn[1] == ordinal), None) is None:
                raise MissingFunction(
                    f"Function with ordinal {ordinal} not present in XP {dll_name}"
                )
        else:
            raise RuntimeError("Import with no name or ordinal???")

print("All functions present!")
