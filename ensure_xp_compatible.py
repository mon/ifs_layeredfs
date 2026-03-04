import argparse
import functools
import os
from collections.abc import Callable
from glob import glob
from typing import TypeAlias

import pefile
from requests.utils import CaseInsensitiveDict

parser = argparse.ArgumentParser()
parser.add_argument("dll_path", nargs="+")

args = parser.parse_args()

Imports: TypeAlias = dict[str, list[tuple[bytes | None, int | None]]]
Exports: TypeAlias = list[tuple[bytes | None, int | None]]


class MissingDLL(Exception):
    pass


class MissingFunction(Exception):
    pass


def load_imports(paths: list[str]) -> CaseInsensitiveDict[Imports]:
    dlls = CaseInsensitiveDict()
    for path in paths:
        print(f"Loading imports from {path}")
        pe = pefile.PE(path)
        imports: Imports = {}
        total = 0
        for entry in pe.DIRECTORY_ENTRY_IMPORT:
            these = []
            for imp in entry.imports:
                these.append((imp.name, imp.ordinal))

            imports[entry.dll.decode()] = these
            total += len(these)

        print(f"  ...{total} imports from {len(imports)} DLLs")

        dlls[os.path.basename(path)] = imports

    return dlls


def load_exports(path: str) -> Callable[[], Exports]:
    @functools.cache
    def load():
        print(f"Loading exports from {path}...")
        pe = pefile.PE(path)
        exports: Exports = []
        for exp in pe.DIRECTORY_ENTRY_EXPORT.symbols:
            exports.append((exp.name, exp.ordinal))

        print(f"  ...{len(exports)} exports")

        return exports

    return load


available_functions: CaseInsensitiveDict[Callable[[], Exports]] = CaseInsensitiveDict()
for dll in glob("./xp_dlls/*.dll"):
    available_functions[os.path.basename(dll)] = load_exports(dll)

loaded_dlls = load_imports(args.dll_path)

# https://www.geoffchappell.com/studies/windows/win32/kernel32/api/index.htm
# my test .dll is 32-bit and these are only in 64
ignore_functions = [
    b"RtlLookupFunctionEntry",
    b"RtlUnwindEx",
    b"RtlVirtualUnwind",
    b"RtlRestoreContext",
    b"__C_specific_handler",
]

exceptions = []
for path, needed_functions in loaded_dlls.items():
    for dll_name, imports in needed_functions.items():
        if (dll := available_functions.get(dll_name)) is None:
            if dll_name not in loaded_dlls:
                exceptions.append(
                    MissingDLL(
                        f"{path}: Need {dll_name} but it's not in the XP DLLs folder"
                    )
                )
            continue

        dll = dll()

        for name, ordinal in imports:
            if name is not None:
                if name in ignore_functions:
                    continue

                if next((fn for fn in dll if fn[0] == name), None) is None:
                    exceptions.append(
                        MissingFunction(
                            f'{path}: Function "{name}" not present in XP {dll_name}'
                        )
                    )
                    continue
            elif ordinal is not None:
                if next((fn for fn in dll if fn[1] == ordinal), None) is None:
                    exceptions.append(
                        MissingFunction(
                            f"{path}: Function with ordinal {ordinal} not present in XP {dll_name}"
                        )
                    )
                    continue
            else:
                raise RuntimeError(f"{path}: Import with no name or ordinal???")

if exceptions:
    raise ExceptionGroup("Failures", exceptions)

print("All functions present!")
