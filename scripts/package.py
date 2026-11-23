"""Validate and produce a Swofty-importable addon ZIP from a native build."""
import hashlib
import json
import math
import shutil
import struct
import zipfile
from pathlib import Path

if not __debug__:
    raise SystemExit("Package validation requires Python without -O.")

root = Path(__file__).resolve().parent.parent
manifest = json.loads((root / 'addon/swofty.json').read_text())
assert manifest['schema_version'] == 1
assert manifest['experimental_features'] == ['overlay']
assert manifest['operations']['native'] == [{'dll': 'native/input_overlay.dll'}]
assert len({o['id'] for o in manifest['options']}) == len(manifest['options'])
for option in manifest['options']:
    if option['type'] == 'slider':
        lo, hi, step = option['min'], option['max'], option['step']
        assert all(math.isfinite(v) for v in (lo, hi, step, option["default"]))
        assert step > 0 and hi > lo
        assert abs((hi-lo)/step - round((hi-lo)/step)) < 1e-8
        assert (hi-lo)/step <= 100, f"{option['id']}: more than 100 steps"
        assert lo <= option['default'] <= hi
        assert abs((option['default']-lo)/step - round((option['default']-lo)/step)) < 1e-8
dll = root / 'build/native/input_overlay.dll'
binary = dll.read_bytes()
build_info = (root / 'build/BUILD-INFO.txt').read_text().splitlines()
expected = f"F1 25 Input Overlay {manifest['version']} experimental"
if not build_info or build_info[0] != expected or f"DLL SHA256: {hashlib.sha256(binary).hexdigest()}" not in build_info:
    raise SystemExit('The native build does not match this package. Run scripts/build.sh first.')
assert binary[:2] == b'MZ', 'Missing Windows PE header'
pe = struct.unpack_from('<I', binary, 0x3c)[0]
assert binary[pe:pe+4] == b'PE\0\0'
assert struct.unpack_from('<H', binary, pe+4)[0] == 0x8664, 'Not AMD64'
assert struct.unpack_from('<H', binary, pe+22)[0] & 0x2000, 'Not a DLL'
for symbol in (b'SwoftyAddonInit\0', b'SwoftyAddonApi\0'):
    assert symbol in binary, 'Missing addon export'
files = {
    'swofty.json': root / 'addon/swofty.json',
    'native/input_overlay.dll': dll,
    'INSTALL.md': root / 'docs/INSTALL.md',
    'MULTI_DRIVER_REVIEW.md': root / 'docs/MULTI_DRIVER_REVIEW.md',
    'LICENSE': root / 'LICENSE',
    'THIRD_PARTY_LICENSES/Swofty-MIT.txt': root / 'third_party/swofty/LICENSE',
    'BUILD-INFO.txt': root / 'build/BUILD-INFO.txt',
}
out = root / 'dist' / f"F125-Input-Overlay-{manifest['version']}.zip"
out.parent.mkdir(exist_ok=True)
with zipfile.ZipFile(out, 'w', zipfile.ZIP_DEFLATED) as archive:
    for name, path in files.items():
        archive.write(path, name)
with zipfile.ZipFile(out) as archive:
    assert archive.testzip() is None
    assert set(archive.namelist()) == set(files)
    for operation in manifest['operations']['native']:
        assert archive.read(operation['dll']) == binary
print(f'{out}\nSHA256 {hashlib.sha256(out.read_bytes()).hexdigest()}')

addon_out = out.with_suffix(".swoftyaddon")
shutil.copyfile(out, addon_out)
print(addon_out)
